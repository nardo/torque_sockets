#!/usr/bin/python2.4
#
# Copyright 2008 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""C++ header file generator.

This module generates C++ header files from the parsed syntax tree.
"""

import cpp_utils
import naming
import syntax_tree

# TODO: have these exceptions derive from a common Error.


class CircularDefinition(Exception):
  """Raised when a circular type definition is found."""

  def __init__(self, type_defn):
    Exception.__init__(self)
    self.type_defn = type_defn


class BadForwardDeclaration(Exception):
  """Raised when an impossible forward declaration is required."""


def ForwardDecl(section, type_defn):
  """Emits the forward declaration of a type, if possible.

  Inner types (declared inside a class) cannot be forward-declared.
  Only classes can be forward-declared.

  Args:
    section: the section to emit to.
    type_defn: the Definition for the type to forward-declare.

  Raises:
    BadForwardDeclaration: an inner type or a non-class was passed as an
      argument.
  """
  # inner types cannot be forward-declared
  if type_defn.parent.defn_type != 'Namespace':
    raise BadForwardDeclaration
  stack = type_defn.GetParentScopeStack()
  if type_defn.defn_type == 'Class':
    for scope in stack:
      if scope.name:
        section.PushNamespace(scope.name)
    section.EmitCode('class %s;' % type_defn.name)
    for scope in stack[::-1]:
      if scope.name:
        section.PopNamespace()
  else:
    raise BadForwardDeclaration


class HeaderGenerator(object):
  """Header generator class.

  This class takes care of the details of generating a C++ header file
  containing all the definitions from a syntax tree.

  It contains a list of functions named after each of the Definition classes in
  syntax_tree, with a common signature. The appropriate function will be called
  for each definition, to generate the code for that definition.
  """

  def __init__(self, output_dir):
    self._output_dir = output_dir

  def GetSectionFromAttributes(self, parent_section, defn):
    """Gets the code section appropriate for a given definition.

    Classes have 3 definition sections: private, protected and public. This
    function will pick one of the sections, based on the attributes of the
    definition, if its parent is a class. For other scopes (namespaces) it will
    return the parent scope main section.

    Args:
      parent_section: the main section for the parent scope.
      defn: the definition.

    Returns:
      the appropriate section.
    """
    if defn.parent and defn.parent.defn_type == 'Class':
      if 'private' in defn.attributes:
        return parent_section.GetSection('private:') or parent_section
      elif 'protected' in defn.attributes:
        return parent_section.GetSection('protected:') or parent_section
      else:
        return parent_section.GetSection('public:') or parent_section
    else:
      return parent_section

  def Verbatim(self, parent_section, scope, obj):
    """Generates the code for a Verbatim definition.

    Verbatim definitions being written for a particular type of output file,
    this function will check the 'verbatim' attribute, and only emit the
    verbatim code if it is 'cpp_header'.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Verbatim definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    scope = scope  # silence gpylint.
    try:
      if obj.attributes['verbatim'] == 'cpp_header':
        section = self.GetSectionFromAttributes(parent_section, obj)
        section.EmitCode(obj.text)
        return []
      else:
        return []
    except KeyError:
      source = obj.source
      print ('%s:%d ignoring verbatim with no verbatim attribute' %
             (source.file.source, source.line))
      return []

  def Typedef(self, parent_section, scope, obj):
    """Generates the code for a Typedef definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Typedef definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    section = self.GetSectionFromAttributes(parent_section, obj)
    bm = obj.type_defn.binding_model
    type_string, unused_need_defn = bm.CppTypedefString(scope, obj.type_defn)
    check_types = [(True, obj.type_defn)]
    section.EmitCode('typedef %s %s;' % (type_string, obj.name))
    return check_types

  def Variable(self, parent_section, scope, obj):
    """Generates the code for a Variable definition.

    This function will generate the member/global variable declaration, as well
    as the setter/getter functions if specified in the attributes.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Variable definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    if obj.parent.defn_type == 'Class':
      if 'field_access' in obj.attributes:
        member_section = parent_section.GetSection(
            obj.attributes['field_access'] + ':')
      else:
        member_section = parent_section.GetSection('private:')
    else:
      member_section = parent_section
    getter_section = self.GetSectionFromAttributes(parent_section, obj)

    bm = obj.type_defn.binding_model
    type_string, need_defn = bm.CppMemberString(scope, obj.type_defn)
    check_types = [(need_defn, obj.type_defn)]
    if 'static' in obj.attributes:
      static = 'static '
    else:
      static = ''
    field_name = naming.Normalize(obj.name, naming.Java)
    self.Documentation(member_section, scope, obj)
    member_section.EmitCode('%s%s %s;' % (static, type_string, field_name))
    if 'getter' in obj.attributes:
      return_type, need_defn = bm.CppReturnValueString(scope, obj.type_defn)
      check_types += [(need_defn, obj.type_defn)]
      getter_name = cpp_utils.GetGetterName(obj)
      self.FieldFunctionDocumentation(getter_section, 'Accessor', type_string,
                                      field_name)
      getter_section.EmitCode('%s%s %s() const { return %s; }' %
                              (static, return_type, getter_name, field_name))
    if 'setter' in obj.attributes:
      param_type, need_defn = bm.CppParameterString(scope, obj.type_defn)
      check_types += [(need_defn, obj.type_defn)]
      setter_name = cpp_utils.GetSetterName(obj)
      self.FieldFunctionDocumentation(getter_section, 'Mutator', type_string,
                                      field_name)
      getter_section.EmitCode('%svoid %s(%s %s) { %s = %s; }' %
                              (static, setter_name, param_type, obj.name,
                               field_name, obj.name))
    return check_types

  def Function(self, parent_section, scope, obj):
    """Generates the code for a Function definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Function definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    section = self.GetSectionFromAttributes(parent_section, obj)
    self.Documentation(section, scope, obj)

    # create temporary function for generating Java syntax
    func_name = naming.Normalize(obj.name, naming.Java)
    function = syntax_tree.Function(obj.source, obj.attributes, func_name,
                                    None, [])
    function.type_defn = obj.type_defn
    function.parent = obj.parent
    function.params = obj.params
    prototype, check_types = cpp_utils.GetFunctionPrototype(scope, function, '')
    section.EmitCode(prototype + ';')
    return check_types

  def Callback(self, parent_section, scope, obj):
    """Generates the code for a Callback definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Class definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    parent_section, scope, obj = parent_section, scope, obj  # silence gpylint
    # TODO: implement this. Do we want to generate the C++ callback object
    # (either through a CallbackN<A0, A1, ...> typedef, or explicitly), or
    # something else that could be more useful to generate JS docs ?
    return []

  def Class(self, parent_section, scope, obj):
    """Generates the code for a Class definition.

    This function will recursively generate the code for all the definitions
    inside the class. These definitions will be output into one of 3 sections
    (private, protected, public), depending on their attributes. These
    individual sections will only be output if they are not empty.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Class definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    self.Documentation(parent_section, scope, obj)
    section = self.GetSectionFromAttributes(parent_section, obj).CreateSection(
        obj.name)
    check_types = []
    if obj.base_type:
      bm = obj.base_type.binding_model
      section.EmitCode('class %s : public %s {' %
                       (obj.name, bm.CppBaseClassString(scope, obj.base_type)))
      check_types += [(True, obj.base_type)]
    else:
      section.EmitCode('class %s {' % obj.name)
    public_section = section.CreateSection('public:')
    protected_section = section.CreateSection('protected:')
    private_section = section.CreateSection('private:')
    self.DefinitionList(section, obj, obj.defn_list)
    if not public_section.IsEmpty():
      public_section.AddPrefix('public:')
    if not protected_section.IsEmpty():
      protected_section.AddPrefix('protected:')
    if not private_section.IsEmpty():
      private_section.AddPrefix('private:')
    section.EmitCode('};')
    return check_types

  def Namespace(self, parent_section, scope, obj):
    """Generates the code for a Namespace definition.

    This function will recursively generate the code for all the definitions
    inside the namespace.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Namespace definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    scope = scope  # silence gpylint.
    self.Documentation(parent_section, scope, obj)
    parent_section.PushNamespace(obj.name)
    self.DefinitionList(parent_section, obj, obj.defn_list)
    parent_section.PopNamespace()
    return []

  def Typename(self, parent_section, scope, obj):
    """Generates the code for a Typename definition.

    Since typenames (undefined types) cannot be expressed in C++, this function
    will not output code. The definition may be output with a verbatim section.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Typename definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    # silence gpylint.
    (parent_section, scope, obj) = (parent_section, scope, obj)
    return []

  def Enum(self, parent_section, scope, obj):
    """Generates the code for an Enum definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Enum definition.

    Returns:
      a list of (boolean, Definition) pairs, of all the types that need
      to be declared (boolean is False) or defined (boolean is True) before
      this definition.
    """
    scope = scope  # silence gpylint.
    section = self.GetSectionFromAttributes(parent_section, obj)
    self.Documentation(parent_section, scope, obj)
    section.EmitCode('enum %s {' % obj.name)
    for value in obj.values:
      if value.value is None:
        section.EmitCode('%s,' % value.name)
      else:
        section.EmitCode('%s = %s,' % (value.name, value.value))
    section.EmitCode('};')
    return []

  def DefinitionList(self, parent_section, scope, defn_list):
    """Generates the code for all the definitions in a list.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      defn_list: the list of definitions.
    """
    for obj in defn_list:
      self.emitted_defn.add(obj)
      # array types are implicitly defined
      for k in obj.array_defns:
        self.emitted_defn.add(obj.array_defns[k])
      func = getattr(self, obj.defn_type)
      check_types = func(parent_section, scope, obj)
      for need_defn, type_defn in check_types:
        self.CheckType(need_defn, type_defn)

  def Documentation(self, parent_section, scope, obj):
    """Generates the documentation code.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the object to be documented; may be class, function, enum or field.
    """
    for scoped_obj in scope.defn_list:
      # TODO: make documentation defn_type more generalized
      if scoped_obj.defn_type == 'Verbatim':
        if ('verbatim' in scoped_obj.attributes and
            scoped_obj.attributes['verbatim'] == 'docs'):
          try:
            # Should skip documentation if object does not have matching name
            # If id exists in documentation block, but not in object or does
            #   not match object's id, skip this documentation block
            # If id does not exist as an attribute, look only for matching type
            found_documentation = False
            if scoped_obj.attributes['name'] == obj.name:
              if 'id' in scoped_obj.attributes:
                if ('id' in obj.attributes and
                    scoped_obj.attributes['id'] == obj.attributes['id']):
                  found_documentation = True
              elif scoped_obj.attributes['type'] == obj.defn_type:
                found_documentation = True
            if found_documentation:
              section = self.GetSectionFromAttributes(parent_section,
                                                      scoped_obj)
              # Break up text and insert comment formatting
              comment_lines = scoped_obj.text.splitlines(False)
              section.EmitCode('/*! ')
              for line in comment_lines:
                section.EmitCode('* %s' % (line.strip()))
              section.EmitCode('*/')
          except KeyError:
            source = obj.source
            print ('%s:%d ignoring documentation with incorrect attributes' %
                   (source.file.source, source.line))

  def FieldFunctionDocumentation(self, member_section, description,
                                 type_string, field_name):
    """Automatically generate the get/set function documentation code.

    Args:
      member_section: the main section of the getter/setter scope.
      description: describes the field function.
      type_string: string defining field type.
      field_name: getter/setter field name.
    """
    member_section.EmitCode('/*!')
    member_section.EmitCode('* %s for %s %s' %
                            (description, type_string, field_name))
    member_section.EmitCode('*/')

  def CheckType(self, need_defn, type_defn):
    """Checks for the definition or declaration of a type.

    This function helps keeping track of which types are needed to be defined
    or declared in the C++ file before other definitions can happen. If the
    definition is needed (and is not in this C++ header file), the proper
    include will be generated. If the type only needs to be forward-declared,
    the forward declaration will be output (if the type is not otherwise
    defined).

    Args:
      need_defn: a boolean, True if the C++ definition of the type is needed,
        False if only the declaration is needed.
      type_defn: the Definition of the type to check.
    """
    while type_defn.defn_type == 'Array':
      # arrays are implicitly defined with their data type
      type_defn = type_defn.data_type
    if need_defn:
      if type_defn not in self.emitted_defn:
        self.needed_defn.add(type_defn)
    else:
      if type_defn in self.emitted_defn:
        return
      if type_defn.parent and type_defn.parent.defn_type != 'Namespace':
        # inner type: need the definition of the parent.
        self.CheckType(True, type_defn.parent)
      else:
        # only forward-declare classes.
        # typedefs could be forward-declared by emitting the definition again,
        # but this necessitates the source type to be forward-declared before.
        # TODO: see if it is possible to find a proper ordering that let us
        # forward-declare typedefs instead of needing to include the definition.
        if type_defn.defn_type == 'Class':
          self.needed_decl.add(type_defn)
        else:
          self.needed_defn.add(type_defn)

  def Generate(self, idl_file, namespace, defn_list):
    """Generates the header file.

    Args:
      idl_file: the source IDL file containing the definitions, as a
        idl_parser.File instance.
      namespace: a Definition for the global namespace.
      defn_list: the list of top-level definitions.

    Returns:
      a cpp_utils.CppFileWriter that contains the C++ header file code.

    Raises:
      CircularDefinition: circular definitions were found in the file.
    """
    self.needed_decl = set()
    self.needed_defn = set()
    self.emitted_defn = set()
    writer = cpp_utils.CppFileWriter('%s/%s' % (self._output_dir,
                                                idl_file.header), True)

    decl_section = writer.CreateSection('decls')
    code_section = writer.CreateSection('defns')

    self.DefinitionList(code_section, namespace, defn_list)

    self.needed_decl -= self.needed_defn
    if self.needed_decl:
      for type_defn in self.needed_decl:
        # TODO: sort by namespace so that we don't open and close them more
        # than necessary.
        ForwardDecl(decl_section, type_defn)
      decl_section.EmitCode('')

    # TODO: disabling temporarily because of problems
    # for type_defn in self.needed_defn:
    #   if type_defn.source.file == idl_file:
    #     raise CircularDefinition(type_defn)
    includes = set(type_defn.GetDefinitionInclude()
                   for type_defn in self.needed_defn)
    for include_file in includes:
      if include_file is not None:
        writer.AddInclude(include_file)
    return writer


def ProcessFiles(output_dir, pairs, namespace):
  """Generates the headers for all input files.

  Args:
    output_dir: the output directory.
    pairs: a list of (idl_parser.File, syntax_tree.Definition list) describing
      the list of top-level definitions in each source file.
    namespace: a syntax_tree.Namespace for the global namespace.

  Returns:
    a list of cpp_utils.CppFileWriter, one for each output header file.
  """
  generator = HeaderGenerator(output_dir)
  writer_list = []
  for (f, defn) in pairs:
    writer_list.append(generator.Generate(f, namespace, defn))
  return writer_list


def main():
  pass

if __name__ == '__main__':
  main()
