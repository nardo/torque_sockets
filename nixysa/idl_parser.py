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

"""Parser module for IdlGlue-NG code generator.

This is the parser module for the IdlGlue-NG code generator. It is written using
ply (http://www.dabeaz.com/ply/).
"""

import sys
import os.path
from ply import lex
from ply import yacc

import syntax_tree


class File(object):
  """Simple class that stores filenames for each IDL source file.

  Various code generation back-ends can add fields, such as 'glue header' or
  'glue cpp'.

  Attributes:
    source: the source IDL filename.
    header: C++ header file containing definitions of types declared in this
      IDL file.
  """

  def __init__(self, filename):
    """Inits a File instance.

    Args:
      filename: the IDL filename.
    """
    self.source = filename
    self.basename = os.path.basename(filename).split('.')[0]
    self.header = self.basename + '.h'
    self.documentation = self.basename + '.doc'

  def __str__(self):
    return self.source


class SourceLocation(object):
  """Simple class that stores the source location of IDL definitions.

  Attributes:
    file: the source IDL File object containing the definition.
    line: the source line of the definition.
  """

  def __init__(self, source_file, line):
    """Inits a SourceLocation instance.

    Args:
      source_file: the source IDL File object containing the definition.
      line: the source line of the definition.
    """
    self.file = source_file
    self.line = line

  def __str__(self):
    return '%s:%s' % (self.file, self.line)


class Parser(object):
  """IDL parser class.

  This class implements the IDL parser, that parses IDL files to produce syntax
  tree objects. It is written using PLY. t_* methods and members are used for
  the ply lexer to define the tokens and their regular expressions, p_* methods
  are used for the ply parser to define the grammar rules.
  """

  def __init__(self, output_dir):
    self.output_dir = output_dir

  # remove gpylint warnings regarding docstrings and naming.
  # pylint: disable-msg=C6409,C6102,C6108,C6104,C6111,C6105,C6310

  tokens = ('NAMESPACE',
            'CLASS',
            'ENUM',
            'TYPEDEF',
            'TYPENAME',
            'CALLBACK',
            'QUALIFIER',
            'SIGNED',
            'TEXT',
            'VERBATIM_OPEN',
            'VERBATIM_CLOSE',
            'DOCUMENTATION_OPEN',
            'DOCUMENTATION_CLOSE',
            'COMMENT_OPEN',
            'COMMENT_CLOSE',
            'STRING_OPEN',
            'STRING_CLOSE',
            'ID',
            'NUMBER')

  _reserved = {'namespace': 'NAMESPACE',
               'class': 'CLASS',
               'struct': 'CLASS',
               'enum': 'ENUM',
               'typename': 'TYPENAME',
               'typedef': 'TYPEDEF',
               'callback': 'CALLBACK',
               'unsigned': 'SIGNED',
               'signed': 'SIGNED',
               'const': 'QUALIFIER',
               'volatile': 'QUALIFIER',
               'restrict': 'QUALIFIER'}

  states = (('verbatim', 'exclusive'),
            ('documentation', 'exclusive'),
            ('cppcomment', 'exclusive'),
            ('ccomment', 'exclusive'),
            ('string', 'exclusive'))

  literals = ['{', '}', '(', ')', '[', ']', ';', ':', ',', '=', '?']

  # Tokens
  t_ignore = ' \t\r'
  t_verbatim_ignore = '\r'
  t_documentation_ignore = '\r'
  t_ccomment_ignore = '\r'
  t_cppcomment_ignore = '\r'
  t_string_ignore = '\r'
  t_NUMBER = r'0x[0-9A-Fa-f]+|0[0-7]*|[1-9][0-9]*'

  def t_COMMENT_OPEN_C(self, t):
    r'/\*'
    t.lexer.begin('ccomment')
    t.type = 'COMMENT_OPEN'
    #return t

  def t_ccomment_COMMENT_CLOSE(self, t):
    r'\*/'
    t.lexer.begin('INITIAL')
    #return t

  def t_COMMENT_OPEN_CPP(self, t):
    r'//'
    t.lexer.begin('cppcomment')
    t.type = 'COMMENT_OPEN'
    #return t

  def t_cppcomment_COMMENT_CLOSE(self, t):
    r'\n'
    t.lexer.lineno += 1
    t.lexer.begin('INITIAL')
    #return t

  def t_STRING_OPEN(self, t):
    r'"'
    t.lexer.begin('string')
    return t

  def t_string_STRING_CLOSE(self, t):
    r'"'
    t.lexer.begin('INITIAL')
    return t

  def t_string_TEXT(self, t):
    r'[^\\\r"]+'
    t.lexer.lineno += t.value.count('\n')
    return t

  def t_string_TEXT_ESCAPE(self, t):
    r'\\.'
    t.type = 'TEXT'
    escape_map = {'n': '\n', 'r': '\r', 't': '\t'}
    char = t.value[1]
    t.value = escape_map.get(char, char)
    return t

  def t_INITIAL_newline(self, t):
    r'\n+'
    t.lexer.lineno += t.value.count('\n')

  def t_ANY_error(self, t):
    location = self._GetLocation()
    print ("Illegal character '%s' at file %s line %d" %
           (t.value[0], location.file.source, location.line))
    t.lexer.skip(1)

  def t_ID(self, t):
    r'~?[a-zA-Z_][a-zA-Z_0-9]*'
    t.type = self._reserved.get(t.value, 'ID')  # Check for reserved words
    return t

  def t_VERBATIM_OPEN(self, t):
    r'%{'
    t.lexer.begin('verbatim')
    return t

  def t_verbatim_VERBATIM_CLOSE(self, t):
    r'%}'
    t.lexer.begin('INITIAL')
    return t

  def t_DOCUMENTATION_OPEN(self, t):
    r'%\['
    t.lexer.begin('documentation')
    return t

  def t_documentation_DOCUMENTATION_CLOSE(self, t):
    r'%\]'
    t.lexer.begin('INITIAL')
    return t

  def t_cppcomment_TEXT(self, t):
    r'[^\n\r]+'
    #return t

  # group characters for faster parsing, but escape at '%' and '*' to let the
  # rules above to take precedence
  def t_ccomment_TEXT(self, t):
    r'[^*\r]+|\*'
    t.lexer.lineno += t.value.count('\n')
    #return t

  # group characters for faster parsing, but escape at '%' and '*' to let the
  # rules above to take precedence
  def t_verbatim_ccomment_TEXT(self, t):
    r'[^%\r]+|%'
    t.lexer.lineno += t.value.count('\n')
    return t

  # group characters for faster parsing, but escape at '%' and '*' to let the
  # rules above to take precedence
  def t_documentation_ccomment_TEXT(self, t):
    r'[^%\r]+|%'
    t.lexer.lineno += t.value.count('\n')
    return t

  # Grammar rules

  def p_start(self, p):
    'start : definition_list'
    p[0] = p[1]

  def p_class_definition(self, p):
    # This line is long, but it should not be split, PLY needs individual rules
    # to be on a single line.
    "class_definition : attributes_opt CLASS ID base_class '{' member_definition_list '}' ';'"
    p[0] = syntax_tree.Class(self._GetLocation(), attributes=p[1], name=p[3],
                             base_type_ref=p[4], defn_list=p[6])

  def p_base_class_1(self, p):
    'base_class : empty'
    p[0] = None

  def p_base_class_2(self, p):
    "base_class : ':' type"
    p[0] = p[2]

  def p_attributes_opt_1(self, p):
    'attributes_opt : empty'
    p[0] = {}

  # If we want to force documentation, this rule and the one above should not
  # exist.
  def p_attributes_opt_2(self, p):
    "attributes_opt : '[' attribute_list_opt ']'"
    p[0] = dict(p[2])

  def p_attributes_opt_3(self, p):
    "attributes_opt : documentation_value '[' attribute_list_opt ']'"
    p[0] = dict(p[1] + p[3])

  def p_attributes_opt_4(self, p):
    'attributes_opt : documentation_value'
    p[0] = dict(p[1])

  def p_attribute_list_opt(self, p):
    """attribute_list_opt : empty
                          | attribute_list"""
    if len(p) == 1:
      p[0] = []
    else:
      p[0] = p[1]

  def p_attribute_list_1(self, p):
    'attribute_list : attribute'
    p[0] = [p[1]]

  def p_attribute_list_2(self, p):
    "attribute_list : attribute_list ',' attribute"
    p[0] = p[1] + [p[3]]

  def p_attribute(self, p):
    'attribute : attrid attribute_value'
    p[0] = (p[1], p[2])

  def p_attribute_value_1(self, p):
    'attribute_value : empty'
    p[0] = None

  def p_attribute_value_2(self, p):
    "attribute_value : '=' attrid"
    p[0] = p[2]

  def p_attrid_1(self, p):
    """attrid : ID
              | QUALIFIER"""
    p[0] = p[1]

  def p_attrid_2(self, p):
    'attrid : STRING_OPEN text STRING_CLOSE'
    p[0] = p[2]

  def p_documentation_value_1(self, p):
    'documentation_value : DOCUMENTATION_OPEN text DOCUMENTATION_CLOSE'
    p[0] = [('__docs', p[2])]

  def p_documentation_value_2(self, p):
    'documentation_value : DOCUMENTATION_OPEN DOCUMENTATION_CLOSE'
    p[0] = [('__docs', '')]

  def p_definition_list_1(self, p):
    'definition_list : comments'
    p[0] = []

  def p_definition_list_2(self, p):
    'definition_list : definition_list definition comments'
    p[0] = p[1] + [p[2]]

  def p_definition(self, p):
    """definition : function_definition
                  | variable_definition
                  | typename_definition
                  | typedef_definition
                  | class_definition
                  | enum_definition
                  | callback_definition
                  | namespace_definition
                  | verbatim_block"""
    p[0] = p[1]

  def p_member_definition_list_1(self, p):
    'member_definition_list : comments'
    p[0] = []

  def p_member_definition_list_2(self, p):
    'member_definition_list : member_definition_list member_definition comments'
    p[0] = p[1] + [p[2]]

  def p_member_definition(self, p):
    """member_definition : function_definition
                         | constructor_definition
                         | variable_definition
                         | typename_definition
                         | typedef_definition
                         | class_definition
                         | enum_definition
                         | callback_definition
                         | verbatim_block"""
    p[0] = p[1]

  def p_comment_opt(self, p):
    """comments : empty
                | comments comment"""
    pass

  def p_comment(self, p):
    'comment : COMMENT_OPEN text COMMENT_CLOSE'
    p[0] = p[2]

  def p_verbatim_block(self, p):
    'verbatim_block : attributes_opt VERBATIM_OPEN text VERBATIM_CLOSE'
    p[0] = syntax_tree.Verbatim(self._GetLocation(), attributes=p[1], text=p[3])

  def p_text(self, p):
    'text : text_list'
    p[0] = ''.join(p[1])

  def p_text_list(self, p):
    """text_list : empty
                 | text_list TEXT"""
    if len(p) == 3:
      p[0] = p[1] + [p[2]]
    else:
      p[0] = []

  def p_typedef_definition(self, p):
    "typedef_definition : attributes_opt TYPEDEF type ID ';'"
    p[0] = syntax_tree.Typedef(self._GetLocation(), attributes=p[1],
                               type_ref=p[3], name=p[4])

  def p_typename_definition(self, p):
    "typename_definition : attributes_opt TYPENAME ID ';'"
    p[0] = syntax_tree.Typename(self._GetLocation(), attributes=p[1], name=p[3])

  def p_variable_definition(self, p):
    "variable_definition : attributes_opt type ID ';'"
    p[0] = syntax_tree.Variable(self._GetLocation(), attributes=p[1],
                                type_ref=p[2], name=p[3])

  def p_function_definition(self, p):
    "function_definition : attributes_opt type ID '(' param_list_opt ')' ';'"
    p[0] = syntax_tree.Function(self._GetLocation(), attributes=p[1],
                                type_ref=p[2], name=p[3], params=p[5])

  def p_callback_definition(self, p):
    "callback_definition : attributes_opt CALLBACK type ID '(' param_list_opt ')' ';'"
    p[0] = syntax_tree.Callback(self._GetLocation(), attributes=p[1],
                                type_ref=p[3], name=p[4], params=p[6])

  def p_constructor_definition(self, p):
    "constructor_definition : attributes_opt ID '(' param_list_opt ')' ';'"
    p[0] = syntax_tree.Function(self._GetLocation(), attributes=p[1],
                                type_ref=None, name=p[2], params=p[4])

  def p_param_list_opt_1(self, p):
    """param_list_opt : empty"""
    p[0] = []

  def p_param_list_opt_2(self, p):
    """param_list_opt : param_list"""
    p[0] = p[1]

  def p_param_list_1(self, p):
    """param_list : param
                  | param_list ',' param"""
    if len(p) == 2:
      p[0] = [p[1]]
    else:
      p[0] = p[1] + [p[3]]

  def p_param(self, p):
    'param : type ID'
    p[0] = (p[1], p[2])

  def p_enum_definition(self, p):
    "enum_definition : attributes_opt ENUM ID '{' enum_values '}' ';'"
    p[0] = syntax_tree.Enum(self._GetLocation(), attributes=p[1], name=p[3],
                            values=p[5])

  def p_enum_values(self, p):
    """enum_values : enum_value
                   | enum_values ',' enum_value"""
    if len(p) == 4:
      p[0] = p[1] + [p[3]]
    else:
      p[0] = [p[1]]

  def p_enum_value(self, p):
    """enum_value : ID
                  | ID '=' NUMBER"""
    if len(p) == 4:
      value = p[3]
    else:
      value = None
    p[0] = syntax_tree.Enum.Value(p[1], value)

  def p_namespace_definition(self, p):
    "namespace_definition : attributes_opt NAMESPACE ID '{' definition_list '}'"
    p[0] = syntax_tree.Namespace(self._GetLocation(), attributes=p[1],
                                 name=p[3], defn_list=p[5])

  def p_type(self, p):
    """type : type_reference
            | QUALIFIER type"""
    if len(p) == 3:
      p[0] = syntax_tree.QualifiedTypeReference(self._GetLocation(), p[1], p[2])
    else:
      p[0] = p[1]

  def p_type_reference(self, p):
    """ type_reference : type_name
                       | scoped_type_reference
                       | unsized_array_type_reference
                       | sized_array_type_reference
                       | nullable_type_reference"""
    p[0] = p[1]

  def p_type_name(self, p):
    """type_name : ID
                 | SIGNED ID"""
    if len(p) == 3:
      p[0] = syntax_tree.NameTypeReference(self._GetLocation(), '%s %s' %
                                           (p[1], p[2]))
    else:
      p[0] = syntax_tree.NameTypeReference(self._GetLocation(), p[1])

  def p_scoped_type_reference(self, p):
    "scoped_type_reference : ID ':' ':' type_reference"
    p[0] = syntax_tree.ScopedTypeReference(self._GetLocation(), p[1], p[4])

  def p_unsized_array_type_reference(self, p):
    "unsized_array_type_reference : type_reference '[' ']'"
    p[0] = syntax_tree.ArrayTypeReference(self._GetLocation(), p[1], None)

  def p_sized_array_type_reference(self, p):
    "sized_array_type_reference : type_reference '[' NUMBER ']'"
    p[0] = syntax_tree.ArrayTypeReference(self._GetLocation(), p[1], p[3])

  def p_nullable_type_reference(self, p):
    "nullable_type_reference : type_reference '?'"
    p[0] = syntax_tree.QualifiedTypeReference(self._GetLocation(),
                                              'nullable', p[1])

  def p_empty(self, p):
    'empty : '
    pass

  def p_error(self, p):
    location = self._GetLocation()
    if p is None:
      print ('%s:%d: Syntax error - unexpected end of file' %
             (location.file.source, location.line))
    else:
      print ('%s:%d: Syntax error - unexpected token %s(%s)' %
             (location.file.source, location.line, p.type, p.value))

  def Parse(self, idl_file):
    """Parses an IDL file.

    Args:
      idl_file: the file to parse, as a File object.

    Returns:
      A list of syntax_tree.Definition objects which represent all the
      top-level definitions in the IDL file. These definitions are not
      'finalized', some post-processing has to be executed (see
      syntax_tree.FinalizeObjects).
    """
    self._lexer = lex.lex(module=self)
    # Add the output dir to the system path so that yacc finds the generated
    # parsetab in there.
    sys.path.insert(0, self.output_dir)
    self._parser = yacc.yacc(module=self, outputdir=self.output_dir)
    del sys.path[0]
    self.file = idl_file
    input_data = open(idl_file.source).read()
    return self._parser.parse(input=input_data, lexer=self._lexer)

  def _GetLocation(self):
    return SourceLocation(self.file, self._lexer.lineno)


def main(filename):
  parser = Parser('.')
  print parser.Parse(File(filename))


if __name__ == '__main__':
  if len(sys.argv) != 2:
    print 'usage : idl_parser.py  inputfile'
    raise SystemExit
  main(sys.argv[1])
