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

This module generates C++ header file for a javascript
documentation file from the parsed syntax tree.
"""

import cpp_utils
import gflags
import java_utils
import naming


class UndocumentedError(Exception):
  """Error raised when a member is undocumented."""

  def __init__(self, obj):
    Exception.__init__(self)
    self.object = obj


class CPPHeaderGenerator(object):
  """Header generator class.

  This class takes care of the details of generating a C++ header file
  containing all the definitions from a syntax tree. This particular
  header generator does so with a slant on javascript. This means no
  virtual, static, void, etc.

  It contains a list of functions named after each of the Definition classes in
  syntax_tree, with a common signature. The appropriate function will be called
  for each definition, to generate the code for that definition.

  Attributes:
    _output_dir: output directory
    force_documentation: whether to force all members to have documentation
  """

  def __init__(self, output_dir):
    self._output_dir = output_dir
    self.force_documentation = gflags.FLAGS['force-docs'].value

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
    except KeyError:
      source = obj.source
      print ('%s:%d ignoring verbatim with no verbatim attribute' %
             (source.file.source, source.line))

  def Typedef(self, parent_section, scope, obj):
    """Generates the code for a Typedef definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Typedef definition.
    """
    section = self.GetSectionFromAttributes(parent_section, obj)
    bm = obj.type_defn.binding_model
    type_string = bm.JavaMemberString(scope, obj.type_defn)
    section.EmitCode('typedef %s %s;' % (type_string, obj.name))

  def Variable(self, parent_section, scope, obj):
    """Generates the code for a Variable definition.

    This function will generate the member/global variable declaration, as well
    as the setter/getter functions if specified in the attributes.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Variable definition.
    """
    member_section = self.GetSectionFromAttributes(parent_section, obj)

    bm = obj.type_defn.binding_model
    type_string = bm.JavaMemberString(scope, obj.type_defn)
    # Note: There is no static in javascript
    field_name = naming.Normalize(obj.name, naming.Java)
    if 'getter' in obj.attributes and 'setter' not in obj.attributes:
      self.Documentation(member_section, obj,
                         'This property is read-only.')
    elif 'getter' not in obj.attributes and 'setter' in obj.attributes:
      self.Documentation(member_section, obj,
                         'This property is write-only.')
    else:
      self.Documentation(member_section, obj, '')
    member_section.EmitCode('%s %s;' % (type_string, field_name))
    # Note: There are no getter/setter in javascript

  def Function(self, parent_section, scope, obj):
    """Generates the code for a Function definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Function definition.
    """
    section = self.GetSectionFromAttributes(parent_section, obj)
    self.Documentation(section, obj, '')
    prototype = java_utils.GetFunctionPrototype(scope, obj)
    section.EmitCode(prototype + ';')

  def Callback(self, parent_section, scope, obj):
    """Generates the code for a Callback definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Class definition.
    """
    parent_section, scope, obj = parent_section, scope, obj  # silence gpylint
    # TODO: implement this. Do we want to generate the C++ callback object
    # (either through a CallbackN<A0, A1, ...> typedef, or explicitly), or
    # something else that could be more useful to generate CPP docs ?

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
    """
    self.Documentation(parent_section, obj, '')
    section = self.GetSectionFromAttributes(parent_section, obj).CreateSection(
        obj.name)
    if obj.base_type:
      bm = obj.base_type.binding_model
      type_string = bm.JavaMemberString(scope, obj.base_type)
      section.EmitCode('class %s : public %s {' % (obj.name, type_string))
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

  def Namespace(self, parent_section, scope, obj):
    """Generates the code for a Namespace definition.

    This function will recursively generate the code for all the definitions
    inside the namespace.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Namespace definition.
    """
    scope = scope  # silence gpylint.
    self.Documentation(parent_section, obj, '')
    parent_section.PushNamespace(obj.name)
    self.DefinitionList(parent_section, obj, obj.defn_list)
    parent_section.PopNamespace()

  def Typename(self, parent_section, scope, obj):
    """Generates the code for a Typename definition.

    Since typenames (undefined types) cannot be expressed in C++, this function
    will not output code. The definition may be output with a verbatim section.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Typename definition.
    """
    # silence gpylint.
    parent_section, scope, obj = parent_section, scope, obj

  def Enum(self, parent_section, scope, obj):
    """Generates the code for an Enum definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Enum definition.
    """
    scope = scope  # silence gpylint.
    section = self.GetSectionFromAttributes(parent_section, obj)
    self.Documentation(parent_section, obj, '')
    section.EmitCode('enum %s {' % obj.name)
    for value in obj.values:
      if value.value is None:
        section.EmitCode('%s,' % value.name)
      else:
        section.EmitCode('%s = %s,' % (value.name, value.value))
    section.EmitCode('};')

  def DefinitionList(self, parent_section, scope, defn_list):
    """Generates the code for all the definitions in a list.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      defn_list: the list of definitions.
    """
    for obj in defn_list:
      if 'nojs' in obj.attributes:
        continue
      func = getattr(self, obj.defn_type)
      func(parent_section, scope, obj)

  def Documentation(self, parent_section, obj, extra_doc):
    """Generates the documentation code.

    Args:
      parent_section: the main section of the parent scope.
      obj: the object to be documented; may be class, function, enum or field.
      extra_doc: extra documenation information to be put in comments
    Raises:
      UndocumentedError: an error if there is no documentation
    """
    try:
      section = self.GetSectionFromAttributes(parent_section, obj)
      comment_lines = obj.attributes['__docs'].splitlines()
      # Break up text and insert comment formatting
      section.EmitCode('/*! ')
      for line in comment_lines:
        section.EmitCode('%s' % (line))
      section.EmitCode('%s' % (extra_doc))
      section.EmitCode('*/')

    except KeyError:
      # catch the error when __docs does not exist
      if self.force_documentation:
        source = obj.source
        print ('%s:%d Documentation not found' % (source.file.source,
                                                  source.line))
        raise UndocumentedError('Documentation not found.')

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
    writer = cpp_utils.CppFileWriter('%s/%s' % (self._output_dir,
                                                idl_file.header), True)
    code_section = writer.CreateSection('defns')
    self.DefinitionList(code_section, namespace, defn_list)
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
  generator = CPPHeaderGenerator(output_dir)
  writer_list = []
  for (f, defn) in pairs:
    writer_list.append(generator.Generate(f, namespace, defn))
  return writer_list


def main():
  pass

if __name__ == '__main__':
  main()
