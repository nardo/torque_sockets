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

"""Utilities for C++ code generation.

This module contains a few utilities for C++ code generation.
"""

import re
import naming
import writer


def GetCommonPrefixLength(list1, list2):
  """Returns the length of the common prefix between two lists.

  Args:
    list1: the first list.
    list2: the second list.

  Returns:
    the highest n such as list1[:n] == list2[:n]
  """
  l = min(len(list1), len(list2))
  for i in range(0, l):
    if list1[i] != list2[i]: return i
  return l


def GetScopePrefixWithScopeOperator(scope, type_defn, scope_operator):
  """Gets the prefix string to reference a type from a given scope.

  This function returns a concatenation of scope operators such as, in the
  context of the given scope, when prefixing the name of the given type, it
  will reference exactly that type.

  For example, given:
  namespace A {
    namespace B {
      class C;
    }
    namespace D {
      void F();
    }
  }
  To access C from F, one needs to refer to it by B::C. This function will
  return the 'B::' part.

  Args:
    scope: the Definition for the scope from which the type must be accessed.
    type_defn: the Definition for the type which must be accessed.
    scope_operator: the scope operator for your language, ie '.' or '::'

  Returns:
    the prefix string.
  """
  type_stack = type_defn.GetParentScopeStack()
  scope_stack = scope.GetParentScopeStack() + [scope]
  common_prefix = GetCommonPrefixLength([scope.name for scope in scope_stack],
                                        [scope.name for scope in type_stack])
  return scope_operator.join(
      [s.name for s in type_stack[common_prefix:]] + [''])


def GetScopePrefix(scope, type_defn):
  """Gets the prefix string to reference a type from a given scope.

  This function returns a concatenation of C++ scope operators such as, in the
  context of the given scope, when prefixing the name of the given type, it
  will reference exactly that type.

  For example, given:
  namespace A {
    namespace B {
      class C;
    }
    namespace D {
      void F();
    }
  }
  To access C from F, one needs to refer to it by B::C. This function will
  return the 'B::' part.

  Args:
    scope: the Definition for the scope from which the type must be accessed.
    type_defn: the Definition for the type which must be accessed.

  Returns:
    the prefix string.
  """
  return GetScopePrefixWithScopeOperator(scope, type_defn, '::')


def GetScopedName(scope, type_defn):
  """Gets the prefix string to reference a type from a given scope.

  This function returns a concatenation of C++ scope operators such as, in the
  context of the given scope, when prefixing the name of the given type, it
  will reference exactly that type.

  For example, given:
  namespace A {
    namespace B {
      class C;
    }
    namespace D {
      void F();
    }
  }
  To access C from F, one needs to refer to it by B::C. This function will
  return exactly that.

  Args:
    scope: the Definition for the scope from which the type must be accessed.
    type_defn: the Definition for the type which must be accessed.

  Returns:
    the scoped reference string.
  """
  return GetScopePrefix(scope, type_defn) + type_defn.name


def GetGetterName(field):
  """Gets the name of the getter function for a member field.

  Unless overridden by the 'getter' attribute in IDL, the default name for the
  getter function is the name of the field, normalized to the lower-case
  convention.

  Args:
    field: the Definition for the field.

  Returns:
    the name of the getter function.
  """
  return (field.attributes['getter'] or naming.Normalize(field.name,
                                                         naming.Lower))


def GetSetterName(field):
  """Gets the name of the setter function for a member field.

  Unless overridden by the 'setter' attribute in IDL, the default name for the
  setter function is 'set_' concatenated with the name of the field, normalized
  to the lower-case convention.

  Args:
    field: the Definition for the field.

  Returns:
    the name of the setter function.
  """
  return (field.attributes['setter'] or
          'set_%s' % naming.Normalize(field.name, naming.Lower))


def GetFunctionParamPrototype(scope, param):
  """Gets the string needed to declare a parameter in a function prototype.

  Args:
    scope: the scope of the prototype.
    param: the Function.Param to declare

  Returns:
    a (string, list) pair. The string is the declaration of the parameter in
    the prototype. The list contains (bool, Definition) pairs, describing the
    types that need to be forward-declared (bool is false) or defined (bool is
    true).
  """
  bm = param.type_defn.binding_model
  if param.mutable:
    text, need_defn = bm.CppMutableParameterString(scope, param.type_defn)
  else:
    text, need_defn = bm.CppParameterString(scope, param.type_defn)
  return '%s %s' % (text, param.name), [(need_defn, param.type_defn)]


def GetFunctionPrototype(scope, obj, id_prefix):
  """Gets the string needed to declare a function prototype.

  Args:
    scope: the scope of the prototype.
    obj: the function to declare.
    id_prefix: a string that is used as a prefix to the identifier, to be able
               to declare a member function for example (id_prefix would be
               'Class::').

  Returns:
    a (string, list) pair. The string is the prototype. The list contains
    (bool, Definition) pairs, describing the types that need to be
    forward-declared (bool is false) or defined (bool is true).
  """
  check_types = []
  param_strings = []
  for p in obj.params:
    param_string, check_type = GetFunctionParamPrototype(scope, p)
    check_types += check_type
    param_strings += [param_string]
  param_string = ', '.join(param_strings)
  prefix_strings = []
  suffix_strings = []
  for attrib in ['static', 'virtual', 'inline']:
    if attrib in obj.attributes:
      prefix_strings.append(attrib)
  if prefix_strings:
    prefix_strings.append('')
  if 'const' in obj.attributes:
    suffix_strings.append('const')
  if 'pure' in obj.attributes:
    suffix_strings.append('= 0')
  if suffix_strings:
    suffix_strings.insert(0, '')
  prefix = ' '.join(prefix_strings)
  suffix = ' '.join(suffix_strings)
  if obj.type_defn:
    bm = obj.type_defn.binding_model
    text, need_defn = bm.CppReturnValueString(scope, obj.type_defn)
    check_types += [(need_defn, obj.type_defn)]
    prototype = '%s%s %s%s(%s)%s' % (prefix, text, id_prefix, obj.name,
                                     param_string, suffix)
  else:
    prototype = '%s%s%s(%s)%s' % (prefix, id_prefix, obj.name, param_string,
                                  suffix)
  return prototype, check_types


def MakeHeaderToken(filename):
  """Generates a header guard token.

  Args:
    filename: the name of the header file

  Returns:
    the generated header guard token.
  """
  return re.sub('[^A-Z0-9_]', '_', filename.upper()) + '__'


class CppFileWriter(object):
  """C++ file writer class.

  This class helps with generating a C++ file by parts, by allowing delayed
  construction of 'sections' inside the code, that can be filled later. For
  example one can create a section for forward declarations, and add code to
  that section as the rest of the file gets written.

  It also provides facility to add #include lines, and header guards for header
  files, as well as simplifies namespace openning and closing.

  It helps 'beautifying' the code in simple cases.
  """

  class Section(object):
    """C++ writer section class."""
    # this regexp is used for EmitTemplate. It maps {#SomeText} into 'section'
    # groups and the rest of the text into 'text' groups in the match objects.
    _template_re = re.compile(
        r"""
        ^\s*                                # skip whitespaces
        (?:                                 # non grouping ( )
        \$\{\#(?P<section>[_A-Za-z0-9]*)\}  #   matches a '${#AnyText}' section
                                            #   tag, puts the 'AnyText' in a
                                            #   'section' group
        |                                   # ... or ...
        (?P<text>.*)                        #   matches any text, puts it into
                                            #   a 'text' group
        )                                   # close non-grouping ( )
        \s*$                                # skip whitespaces
        """, re.MULTILINE | re.VERBOSE)

    def __init__(self, indent_string, indent):
      """Inits a CppFileWriter.Section.

      Args:
        indent_string: the string for one indentation.
        indent: the number of indentations for code inside the section.
      """
      self._indent_string = indent_string
      self._code = []
      self._fe_namespaces = []
      self._be_namespaces = []
      self._section_map = {}
      self._indent = indent
      self._need_validate = False

    def EmitSection(self, section):
      """Emits a section at the current position.

      When calling GetLines, the code for the section passed in will be output
      at this position.

      Args:
        section: the section to add.
      """
      self._ValidateNamespace()
      self._code.append(section)

    def CreateUnlinkedSection(self, name, indent=None):
      """Creates a section, but without emitting it.

      When calling GetLines, the code for the created section will not be
      output unless EmitSection is called.

      Args:
        name: the name of the section.
        indent: (optional) the number of indentations for the new section.

      Returns:
        the created section.
      """
      if not indent:
        indent = self._indent
      section = CppFileWriter.Section(self._indent_string, indent)
      self._section_map[name] = section
      return section

    def CreateSection(self, name):
      """Creates a section, and emits it at the current position.

      When calling GetLines, the code for the created section will be output
      at this position.

      Args:
        name: the name of the section.

      Returns:
        the created section.
      """
      self._ValidateNamespace()
      section = self.CreateUnlinkedSection(name, indent=self._indent)
      self.EmitSection(section)
      return section

    def GetSection(self, name):
      """Gets a section by name.

      Args:
        name: the name of the section.

      Returns:
        the section if found, None otherwise.
      """
      try:
        return self._section_map[name]
      except KeyError:
        return None

    def PushNamespace(self, name):
      """Opens a namespace.

      This function opens a namespace by emitting code at the current position.
      This is done lazily so that openning, closing, then openning the same
      namespace again doesn't add extra code.

      Args:
        name: the name of the namespace.
      """
      self._need_validate = True
      self._fe_namespaces.append(name)

    def PopNamespace(self):
      """Closes the current namespace.

      This function closes the current namespace by emitting code at the
      current position.  This is done lazily so that openning, closing, then
      openning the same namespace again doesn't add extra code.

      Returns:
        the name of the namespace that was closed.
      """
      self._need_validate = True
      return self._fe_namespaces.pop()

    def _ValidateNamespace(self):
      """Validates the current namespace by emitting all the necessary code."""
      if not self._need_validate:
        return
      self._need_validate = False
      l = GetCommonPrefixLength(self._fe_namespaces, self._be_namespaces)
      while len(self._be_namespaces) > l:
        name = self._be_namespaces.pop()
        self._code.append('}  // namespace %s' % name)
      for name in self._fe_namespaces[l:]:
        self._be_namespaces.append(name)
        self._code.append('namespace %s {' % name)

    def EmitCode(self, code):
      """Emits code at the current position.

      The code passed in will be output at the current position when GetLines
      is called. The code is split into lines, and re-indented to match the
      section indentation.

      Args:
        code: a string containing the code to emit.
      """
      self._ValidateNamespace()
      for line in code.split('\n'):
        line = line.strip('\t\r ')
        if not line:
          self._code.append('')
        else:
          adjust_indent = 0
          adjust_chars = ''
          if line[0] == '}':
            adjust_indent -= 1
          if line[-1] == ':':
            adjust_indent -= 1
            adjust_chars = ' '
          self._code.append(self._indent_string * (self._indent + adjust_indent)
                            + adjust_chars + line)
        if not re.search(r'\bnamespace\b', line):
          self._indent += line.count('{') - line.count('}')

    def EmitTemplate(self, template):
      """Emits a template at the current position.

      Somewhat similarly to string.template.substitute, this function takes a
      string containing code and escape sequences. Every time an escape
      sequence, of the form '${#SectionName}', is found, a section is created
      (or re-used) and emitted at the current position. Otherwise the text is
      treated as code and simply emitted as-is. For example take the following
      string:

        void MyFunction() {
          ${#MyFunctionBody}
        }

      Calling EmitTemplate with that string is equivalent to:

        section.EmitCode('void MyFunction() {')
        section.CreateSection('MyFunctionBody')
        section.EmitCode('}')

      If a section of that particular name already exists, it is reused.

      Args:
        template: a string containing the template to emit.
      """

      def _Match(mo):
        """Function called for template regexp matches.

        Args:
          mo: match object.

        Returns:
          empty string.
        """
        section_group = mo.group('section')
        if section_group:
          if section_group in self._section_map:
            section = self._section_map[section_group]
            self.EmitSection(section)
          else:
            self.CreateSection(section_group)
        else:
          self.EmitCode(mo.group('text'))
        return ''
      self._template_re.sub(_Match, template)

    def IsEmpty(self):
      """Queries whether the section is empty or not.

      Returns:
        True if the section is empty, False otherwise.
      """
      return not self._code

    def AddPrefix(self, code):
      """Adds code at the beginning of the section.

      Args:
        code: a single code line.
      """
      self._code = [code] + self._code

    def GetLines(self):
      """Retrieves the full contents of the section.

      This function gathers all the code that was emitted, including in
      children sections.

      Returns:
        a list of code lines.
      """
      # close open namespaces
      self._fe_namespaces = []
      self._need_validate = True
      self._ValidateNamespace()
      lines = []
      for line in self._code:
        if isinstance(line, CppFileWriter.Section):
          lines.extend(line.GetLines())
        else:
          lines.append(line)
      return lines

  def __init__(self, filename, is_header, header_token=None,
               indent_string='  '):
    """Inits a CppFileWriter.

    The file writer has a 'main section' where all the code will go. See
    CreateSection, EmitCode.

    Args:
      filename: the name of the file.
      is_header: a boolean, True if this is a header file. In that case, the
        header guard will be generated.
      header_token: (optional) a string for the header guard token. Defaults to
        a generated one based on the filename.
      indent_string: (optional) the string to be used for indentations.
        Defaults to two spaces.
    """
    self._filename = filename
    self._is_header = is_header
    self._header_token = header_token or MakeHeaderToken(filename)
    self._includes = []
    self._include_section = self.Section(indent_string, 0)
    self._main_section = self.Section(indent_string, 0)

  def AddInclude(self, name, system=False):
    """Adds an include to the file.

    Args:
      name: the name of the include.
      system: (optional) True if it is a 'system' include (uses the <file.h>
        syntax). Defaults to False.
    """
    if system:
      self._include_section.EmitCode('#include <%s>' % name)
    else:
      self._include_section.EmitCode('#include "%s"' % name)

  def CreateSection(self, name):
    """Creates a section within the main section.

    Args:
      name: the name of the section to be created.

    Returns:
      the created section.
    """
    return self._main_section.CreateSection(name)

  def GetSection(self, name):
    """Gets a section by name from the main section.

    Args:
      name: the name of the section to look for.

    Returns:
      the created section if found, None otherwise.
    """
    return self._main_section.GetSection(name)

  def PushNamespace(self, name):
    """Opens a namespace in the main section.

    This function opens a namespace by emitting code at the current position.
    This is done lazily so that openning, closing, then openning the same
    namespace again doesn't add extra code.

    Args:
      name: the name of the namespace.
    """
    self._main_section.PushNamespace(name)

  def PopNamespace(self):
    """Closes the current namespace in the main section.

    This function closes the current namespace by emitting code at the
    current position.  This is done lazily so that openning, closing, then
    openning the same namespace again doesn't add extra code.

    Returns:
      the name of the namespace that was closed.
    """
    return self._main_section.PopNamespace()

  def EmitCode(self, code):
    """Emits code at the current position in the main section.

    Args:
      code: a string containing the code to emit.
    """
    self._main_section.EmitCode(code)

  def GetLines(self):
    """Retrieves the full contents of the file writer.

    This function gathers all the code that was emitted, including the
    header guard (if this is a header file), and the includes.

    Returns:
      a list of code lines.
    """
    lines = []
    if self._is_header:
      lines.extend(['#ifndef %s' % self._header_token,
                    '#define %s' % self._header_token])
    include_lines = self._include_section.GetLines()
    if include_lines:
      lines.append('')
      lines.extend(include_lines)
    main_lines = self._main_section.GetLines()
    if main_lines:
      lines.append('')
      lines.extend(main_lines)
    if self._is_header:
      lines.extend(['', '#endif  // %s' % self._header_token])
    return lines

  def Write(self):
    """Writes the full contents to the file.

    This function writes the full contents to the file specified by the
    'filename' parameter at creation time.
    """
    writer.WriteIfContentDifferent(self._filename,
                                   '\n'.join(self.GetLines()) + '\n')


def main():
  pass


if __name__ == '__main__':
  main()
