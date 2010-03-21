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

"""Javascript JSCompiler and JSDocToolkit file generator.

This module generates JSCompiler and JSDocToolkit file for a
javascript documentation file from the parsed syntax tree.
"""

import gflags
import re
import cpp_utils
import js_utils
import java_utils
import naming
import log
import syntax_tree


class UndocumentedError(Exception):
  """Error raised when a member is undocumented."""

  def __init__(self, obj):
    Exception.__init__(self)
    self.object = obj


class JSHeaderGenerator(object):
  """Header generator class.

  This class takes care of the details of generating JavaScript JSDocToolkit
  format files suitable for either documenation generation or for externs for
  the JSCompiler.

  It contains a list of functions named after each of the Definition classes in
  syntax_tree, with a common signature. The appropriate function will be called
  for each definition, to generate the code for that definition.

  Attributes:
    _output_dir: output directory
    force_documentation: whether to force all members to have documentation
  """
  _start_whitespace_re = re.compile(r'^(\s+)')
  _param_re = re.compile(r'\\param (\w+) ')
  _return_re = re.compile(r'\\return ')
  _code_re = re.compile(r'\\code')
  _li_re = re.compile(r'\\li')
  _var_re = re.compile(r'\\var')
  _sa_re = re.compile(r'\\sa')
  _endcode_re = re.compile(r'\\endcode')

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
    verbatim code if it is 'js_header'.

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
      if obj.attributes['verbatim'] == 'js_header':
        section = self.GetSectionFromAttributes(parent_section, obj)
        section.EmitCode(obj.text)
    except KeyError:
      log.SourceWarning(obj.source,
                        'ignoring verbatim with no verbatim attribute')

  def Typedef(self, parent_section, scope, obj):
    """Generates the code for a Typedef definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Typedef definition.
    """
    # typedefs do not get exported for JavaScript.
    # silence lint
    parent_section, scope, obj = parent_section, scope, obj

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
    id_prefix = js_utils.GetFullyQualifiedScopePrefix(scope)
    proto = 'prototype.'
    # Note: There is no static in javascript
    field_name = naming.Normalize(obj.name, naming.Java)
    extra = ''
    if 'getter' in obj.attributes and 'setter' not in obj.attributes:
      extra = '\n\nThis property is read-only.'
    elif 'getter' not in obj.attributes and 'setter' in obj.attributes:
      extra = '\n\nThis property is write-only.'
    type_string = '\n@type {%s}' % js_utils.GetFullyQualifiedTypeName(
        obj.type_defn)
    self.Documentation(member_section, obj, extra + type_string)
    undef = ''
    if gflags.FLAGS['properties-equal-undefined'].value:
      undef = ' = undefined'
    member_section.EmitCode('%s%s%s%s;' % (id_prefix, proto, field_name, undef))
    # Note: There are no getter/setter in javascript

  def Function(self, parent_section, scope, obj):
    """Generates the code for a Function definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Function definition.
    """
    section = self.GetSectionFromAttributes(parent_section, obj)
    return_type = '**not defined**'
    if obj.type_defn:
      return_type = js_utils.GetFullyQualifiedTypeName(obj.type_defn)
    doc_info = self.Documentation(section, obj, '')
    if doc_info:
      # there was a return statement so the return type better NOT be void
      if return_type == 'void':
        log.SourceError(obj.source,
                        'return found for void function: %s' % obj.name)
    else:
      # there was no return statement so the return type better be void
      if (not return_type == 'void') and (not return_type == '**not defined**'):
        log.SourceError(obj.source, 
                        'return missing for non void function: %s' % obj.name)
    prototype = js_utils.GetFunctionPrototype(scope, obj, True)
    section.EmitCode(prototype)

  def OverloadedFunction(self, parent_section, scope, func_array):
    """Generates the code for an Overloaded Function.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      func_array: an array of function definition objects.
    """
    # merge the params.
    params = []
    min_params = len(func_array[0].params)
    for func in func_array:
      if len(func.params) < min_params:
        min_params = len(func.params)
      index = 0
      # we only take the comment from the first function that documents
      # a parameter at this position.
      comments, param_comments = js_utils.GetCommentsForParams(func)
      for param in func.params:
        if len(params) <= index:
          params.append({'orig_name': param.name,
                         'new_name': param.name,
                         'docs' : param_comments[param.name],
                         'params': [{'param': param, 'func': func}]})
        else:
          params[index]['params'].append({'param': param, 'func': func})
        index += 1

    # rename the params.
    index = 0
    opt = ''
    for param in params:
      if index >= min_params:
        opt = 'opt_'
      param['new_name'] = '%sparam%d' % (opt, index + 1)
      index += 1

    # generate param comments.
    param_comments = []
    for param in params:
      if len(param['params']) == 1:
        param_string = js_utils.GetFunctionParamType(
            param['params'][0]['func'],
            param['params'][0]['param'].name)
      else:
        union_strings = set()
        for option in param['params']:
          union_strings.add(js_utils.GetFunctionParamType(option['func'],
                                                          option['param'].name))
        param_string = '|'.join(union_strings)
        if len(union_strings) > 1:
          param_string = '(' + param_string + ')'
      param_comments += ['@param {%s} %s %s' % (param_string, param['new_name'],
          param['docs'])]

    first_func = func_array[0]

    # use just the first function's comments.
    func_comments = (js_utils.GetCommentsForParams(first_func)[0] +
        '\n'.join(param_comments))

    first_func.attributes['__docs'] = func_comments
    section = self.GetSectionFromAttributes(parent_section, first_func)
    self.Documentation(section, first_func, '')

    param_strings = []
    for param in params:
      param_strings += [param['new_name']]

    param_string = ', '.join(param_strings)
    id_prefix = js_utils.GetFullyQualifiedScopePrefix(scope)
    proto = 'prototype.'
    prototype = '%s%s%s = function(%s) { };' % (
        id_prefix, proto, naming.Normalize(first_func.name, naming.Java),
        param_string)
    section.EmitCode(prototype)

  def Callback(self, parent_section, scope, obj):
    """Generates the code for a Callback definition.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Class definition.
    """
    parent_section, scope, obj = parent_section, scope, obj

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
    section = self.GetSectionFromAttributes(parent_section, obj).CreateSection(
        obj.name)
    id_prefix = js_utils.GetFullyQualifiedScopePrefix(scope)
    if 'marshaled' in obj.attributes:
      found = False
      for member_defn in obj.defn_list:
        if member_defn.name == 'marshaled':
          if isinstance(member_defn, syntax_tree.Variable):
            type_name = js_utils.GetFullyQualifiedTypeName(
                member_defn.type_defn)
            self.Documentation(section, obj, '\n@type {%s}' % type_name)
            section.EmitCode('%s%s = goog.typedef;' % (id_prefix, obj.name))
            found = True
            break
      if not found:
        log.SourceError(obj.source,
                        ('no marshaled function found for %s' % obj.name))
    else:
      extends = ''
      if obj.base_type:
        base = js_utils.GetFullyQualifiedTypeName(obj.base_type)
        if base[0] == '!':
          base = base[1:]
        extends = '\n@extends {%s}' % base
      self.Documentation(section, obj, '\n@constructor' + extends)
      section.EmitCode('%s%s = function() { };' % (id_prefix, obj.name))

    public_section = section.CreateSection('public:')
    protected_section = section.CreateSection('protected:')
    private_section = section.CreateSection('private:')
    self.DefinitionList(section, obj, obj.defn_list)
    # TODO(gman): Delete protected and private sections. Those docs
    # are not needed for javascript.

  def Namespace(self, parent_section, scope, obj):
    """Generates the code for a Namespace definition.

    This function will recursively generate the code for all the definitions
    inside the namespace.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      obj: the Namespace definition.
    """
    self.DefinitionList(parent_section, obj, obj.defn_list)

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
    section = self.GetSectionFromAttributes(parent_section, obj)
    type_string = 'number'
    id_prefix = js_utils.GetFullyQualifiedScopePrefix(scope)
    self.Documentation(parent_section, obj, '\n@enum {%s}' % type_string)
    section.EmitCode('%s%s = {' % (id_prefix, obj.name))
    count = 0
    for ii in range(0, len(obj.values)):
      value = obj.values[ii]
      comma = ','
      if ii == len(obj.values) - 1:
        comma = ''
      if value.value is None:
        section.EmitCode('%s: %d%s' % (value.name, count, comma))
      else:
        section.EmitCode('%s: %s%s' % (value.name, value.value, comma))
        count = int(value.value)
      count += 1
    section.EmitCode('};')

  def DefinitionList(self, parent_section, scope, defn_list):
    """Generates the code for all the definitions in a list.

    Args:
      parent_section: the main section of the parent scope.
      scope: the parent scope.
      defn_list: the list of definitions.
    Returns:
      True if there was a '\returns' tag.
    """
    # extract functions and merge functions of the same name and process all
    # non-functions.
    function_by_name = {}
    for obj in defn_list:
      if 'nojs' in obj.attributes or 'nodocs' in obj.attributes:
        continue
      if obj.defn_type == 'Function':
        if obj.name in function_by_name:
          function_by_name[obj.name].append(obj)
        else:
          function_by_name[obj.name] = [obj]
      else:
        func = getattr(self, obj.defn_type)
        func(parent_section, scope, obj)

    # process functions.
    for func_array in function_by_name.values():
      if len(func_array) == 1:
        self.Function(parent_section, scope, func_array[0])
      else:
        self.OverloadedFunction(parent_section, scope, func_array)

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
      comment_lines = (obj.attributes['__docs'] + extra_doc).splitlines()
      # Break up text and insert comment formatting
      section.EmitCode('/**')
      # move all blank lines at start of docs.
      while comment_lines and comment_lines[0].strip() == '':
        comment_lines = comment_lines[1:]
      # figure out how much whitespace is on first line and remove
      # the same amount from all lines
      start_index = 0
      if comment_lines:
        match = self._start_whitespace_re.search(comment_lines[0])
        if match:
          start_index = len(match.group(1))
      flags = {'eat_lines': False};
      found_returns = False
      for line in comment_lines:
        if line[0:start_index].strip() == '':
          line = line[start_index:]
        if line.startswith('\\'):
          flags['eat_lines'] = False
        if self._return_re.match(line):
          found_returns = True
        line = self._param_re.sub(
            lambda match: js_utils.GetParamSpec(obj, match.group(1)),
            line)
        line = self._return_re.sub(
            lambda match: js_utils.GetReturnSpec(obj, flags) + ' ',
            line)
        line = self._code_re.sub('<pre>', line)
        line = self._li_re.sub('<li>', line)
        line = self._var_re.sub('', line)
        line = self._sa_re.sub('@see', line)
        line = self._endcode_re.sub('</pre>', line)
        if not flags['eat_lines']:
          section.EmitCode(' * %s' % (line))
      section.EmitCode(' */')
      return found_returns

    except KeyError:
      log.SourceError(obj.source, 'Documentation not found')

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
      a js_utils.JavascriptFileWriter that contains the javascript header file
      code.

    Raises:
      CircularDefinition: circular definitions were found in the file.
    """
    filename = idl_file.basename + '.js'
    writer = js_utils.JavascriptFileWriter('%s/%s' % (self._output_dir,
                                                      filename), True)
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
    a list of js_utils.JavascriptFileWriter, one for each output header file.
  """
  generator = JSHeaderGenerator(output_dir)
  writer_list = []
  for (f, defn) in pairs:
    writer_list.append(generator.Generate(f, namespace, defn))
  return writer_list


def main():
  pass

if __name__ == '__main__':
  main()
