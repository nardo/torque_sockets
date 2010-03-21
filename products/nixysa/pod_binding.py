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

"""pod binding model module.

This module implements the glue functions for the pod binding model, that is
used by POD types, and strings (which are POD in JavaScript). 'void' is also
included here, although it is only used for return values (and raises an
exception otherwise).

In C++, objects using this binding model are passed and returned by value (or
by pointer when mutable), except strings which are passed by const reference
(and returned by copy).
For example:
void SetValues(int value, const string &name);
float GetValue();
string GetString();

For JS bindings, they are directly represented by variants.
"""

import string
import sys
import cpp_utils
import java_utils


CPP_POD_TO_JSDOC_TYPES = {
  'int': 'number',
  'std.string' : 'string',
  'bool' : 'boolean',
  'float' : 'number',
  'double' : 'number',
  'unsigned int' : 'number',
  'size_t' : 'number',
  'void' : 'void'};  # void is a special case. It's used for callbacks


class InvalidPODUsage(Exception):
  """Raised when POD type is used incorrectly."""
  pass


class BadVoidUsage(Exception):
  """Raised when 'void' is used outside of a return value."""
  pass


class UnknownPODType(Exception):
  """Raised when an unknown POD type is used."""

  def __init__(self, name):
    Exception.__init__(self)
    self.name = name


def JavaMemberString(scope, type_defn):
  """Gets the representation of a member name in Java.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a string representing the type
  """
  # TODO: Check if we need the check below for Java
  #final_type = type_defn.GetFinalType()
  #if final_type.podtype == 'void':
  #  raise BadVoidUsage
  return java_utils.GetScopedName(scope, type_defn)


def CppTypedefString(scope, type_defn):
  """Gets the representation of a type when used in a C++ typedef.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.

  Raises:
    BadVoidUsage: type_defn is a 'void' POD type.
  """
  if type_defn.GetFinalType().podtype == 'void':
    raise BadVoidUsage
  return cpp_utils.GetScopedName(scope, type_defn), True


def CppMemberString(scope, type_defn):
  """Gets the representation of a type when used as a C++ class member.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.

  Raises:
    BadVoidUsage: type_defn is a 'void' POD type.
  """
  if type_defn.GetFinalType().podtype == 'void':
    raise BadVoidUsage
  return cpp_utils.GetScopedName(scope, type_defn), True


def CppReturnValueString(scope, type_defn):
  """Gets the representation of a type when used as a C++ function return value.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.
  """
  return cpp_utils.GetScopedName(scope, type_defn), True


def CppParameterString(scope, type_defn):
  """Gets the representation of a type when used for a function parameter.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.

  Raises:
    BadVoidUsage: type_defn is a 'void' POD type.
  """
  final_type = type_defn.GetFinalType()
  if final_type.podtype == 'void':
    raise BadVoidUsage
  elif final_type.podtype == 'string' or final_type.podtype == 'wstring':
    return 'const %s&' % cpp_utils.GetScopedName(scope, type_defn), True
  else:
    return cpp_utils.GetScopedName(scope, type_defn), True


def CppMutableParameterString(scope, type_defn):
  """Gets the representation of a type for a mutable function parameter.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the string representing
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.

  Raises:
    BadVoidUsage: type_defn is a 'void' POD type.
  """
  if type_defn.GetFinalType().podtype == 'void':
    raise BadVoidUsage
  return '%s*' % cpp_utils.GetScopedName(scope, type_defn), True


def CppMutableToNonMutable(scope, type_defn, expr):
  """Gets the string converting a mutable expression to a non-mutable one.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.
    expr: a string for the mutable expression.

  Returns:
    a string, which is the non-mutable expression.
  """
  (scope, type_defn) = (scope, type_defn)  # silence gpylint.
  return '*(%s)' % expr


def CppBaseClassString(scope, type_defn):
  """Gets the representation of a type for a base class.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the string representing
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppCallMethod(scope, type_defn, object_expr, mutable, method, param_exprs):
  """Gets the representation of a member function call.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object being called.
    object_expr: a string, which is the expression for the object being called.
    mutable: a boolean, whether or not the 'object_expr' expression is mutable
      or not
    method: a Function, representing the function to call.
    param_exprs: a list of strings, each being the expression for the value of
      each parameter.

  Returns:
    a string, which is the expression for the function call.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppCallStaticMethod(scope, type_defn, method, param_exprs):
  """Gets the representation of a static function call.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object being called.
    method: a Function, representing the function to call.
    param_exprs: a list of strings, each being the expression for the value of
      each parameter.

  Returns:
    a string, which is the expression for the function call.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppCallConstructor(scope, type_defn, method, param_exprs):
  """Gets the representation of a constructor call.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object being called.
    method: a Function, representing the constructor to call.
    param_exprs: a list of strings, each being the expression for the value of
      each parameter.

  Returns:
    a string, which is the expression for the constructor call.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppSetField(scope, type_defn, object_expr, field, param_expr):
  """Gets the representation of an expression setting a field in an object.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object containing the
      field being set.
    object_expr: a string, which is the expression for the object containing
      the field being set.
    field: a string, the name of the field to be set.
    param_expr: a strings, being the expression for the value to be set.

  Returns:
    a string, which is the expression for setting the field.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppGetField(scope, type_defn, object_expr, field):
  """Gets the representation of an expression getting a field in an object.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object containing the
      field being retrieved.
    object_expr: a string, which is the expression for the object containing
      the field being retrieved.
    field: a string, the name of the field to be retrieved.

  Returns:
    a string, which is the expression for getting the field.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppSetStatic(scope, type_defn, field, param_expr):
  """Gets the representation of an expression setting a static field.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object containing the
      field being set.
    field: a string, the name of the field to be set.
    param_expr: a strings, being the expression for the value to be set.

  Returns:
    a string, which is the expression for setting the field.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def CppGetStatic(scope, type_defn, field):
  """Gets the representation of an expression getting a static field.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object containing the
      field being retrieved.
    field: a string, the name of the field to be retrieved.

  Returns:
    a string, which is the expression for getting the field.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def JSDocTypeString(type_defn):
  """Gets the representation of a type in JSDoc notation.

  Args:
    type_defn: a Definition for the type.

  Returns:
    a string that is the JSDoc notation of type_defn.
  """
  type_defn = type_defn.GetFinalType()
  type_stack = type_defn.GetParentScopeStack()
  name = type_defn.name
  type_string = '.'.join([s.name for s in type_stack[1:]] + [name])
  if type_string in CPP_POD_TO_JSDOC_TYPES:
    return CPP_POD_TO_JSDOC_TYPES[type_string]
  print >> sys.stderr, (
      'ERROR: %s : Unknown C++ Pod to JSDoc type conversion for C++ type: %s' %
      (type_defn.source, type_string))
  return '*'


def NpapiBindingGlueHeader(scope, type_defn):
  """Gets the NPAPI glue header for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue header.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def NpapiBindingGlueCpp(scope, type_defn):
  """Gets the NPAPI glue implementation for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue implementation.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


def NpapiDispatchFunctionHeader(scope, type_defn, variable, npp, success):
  """Gets a header for NPAPI glue dispatch functions.

  This function creates a string containing a C++ code snippet that should be
  included at the beginning of NPAPI glue dispatch functions like Invoke or
  GetProperty. This code snippet will declare and initialize certain variables
  that will be used in the dispatch functions, like the NPObject representing
  the object, or a pointer to the NPP instance.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.
    variable: a string, representing a name of a variable that can be used to
      store a reference to the object.
    npp: a string, representing the name of the variable that holds the pointer
      to the NPP instance. Will be declared by the code snippet.
    success: the name of a bool variable containing the current success status.
      (is not declared by the code snippet).

  Returns:
    a (string, string) pair, the first string being the code snippet, and the
    second string being an expression to access the object.

  Raises:
    InvalidPODUsage: always. This function can't be called for a POD type.
  """
  raise InvalidPODUsage


_wstring_from_npvariant_template = string.Template("""
${type} ${variable};
if (!NPVARIANT_IS_STRING(${input}) {
  ${success} = false;
  *error_handle = "Error in " ${context}
      ": was expecting a string.";
} else if (!UTF8ToString16(NPVARIANT_TO_STRING(${input}).utf8characters,
                    NPVARIANT_TO_STRING(${input}).utf8length,
                    &${variable})) {
  ${success} = false;
  *error_handle = "Error in " ${context}
      ": hit an unexpected unicode conversion problem.";
}
""")

_string_from_npvariant_template = string.Template("""
${type} ${variable};
if (NPVARIANT_IS_STRING(${input})) {
  ${variable} = ${type}(NPVARIANT_TO_STRING(${input}).utf8characters,
                        NPVARIANT_TO_STRING(${input}).utf8length);
} else {
  ${success} = false;
  *error_handle = "Error in " ${context}
      ": was expecting a string.";
}
""")

_float_from_npvariant_template = string.Template("""
    ${type} ${variable} = 0.f;
    if (NPVARIANT_IS_NUMBER(${input})) {
      ${variable} = static_cast<${type}>(NPVARIANT_TO_NUMBER(${input}));
    } else {
      *error_handle = "Error in " ${context}
          ": was expecting a number.";
      ${success} = false;
    }
""")

_int_from_npvariant_template = string.Template("""
    ${type} ${variable} = 0;
    if (NPVARIANT_IS_NUMBER(${input})) {
      ${variable} = static_cast<${type}>(NPVARIANT_TO_NUMBER(${input}));
    } else {
      *error_handle = "Error in " ${context}
          ": was expecting an int.";
      ${success} = false;
    }
""")

_bool_from_npvariant_template = string.Template("""
    ${type} ${variable} = false;
    if (NPVARIANT_IS_BOOLEAN(${input})) {
      ${variable} = NPVARIANT_TO_BOOLEAN(${input});
    } else {
      *error_handle = "Error in " ${context}
          ": was expecting a boolean.";
      ${success} = false;
    }
""")

def NpapiFromNPVariant(scope, type_defn, input_expr, variable, success,
    exception_context, npp):
  """Gets the string to get a value from a NPVariant.

  This function creates a string containing a C++ code snippet that is used to
  retrieve a value from a NPVariant. If an error occurs, like if the NPVariant
  is not of the correct type, the snippet will set the success status variable
  to false.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type of the value.
    input_expr: an expression representing the NPVariant to get the value from.
    variable: a string, representing a name of a variable that can be used to
      store a reference to the value.
    success: the name of a bool variable containing the current success status.
    exception_context: the name of a string containing context information, for
      use in exception reporting.
    npp: a string, representing the name of the variable that holds the pointer
      to the NPP instance.

  Returns:
    a (string, string) pair, the first string being the code snippet and the
    second one being the expression to access that value.

  Raises:
    BadVoidUsage: type_defn is a 'void' POD type.
    UnknownPODType: type_defn is not a known POD type.
  """
  npp = npp  # silence gpylint.
  type_name = cpp_utils.GetScopedName(scope, type_defn)
  final_type = type_defn.GetFinalType()
  if final_type.podtype == 'void':
    return '', 'void(0)'
  elif final_type.podtype == 'int':
    text = _int_from_npvariant_template.substitute(type=type_name,
                                                   input=input_expr,
                                                   variable=variable,
                                                   success=success,
                                                   context=exception_context)
    return text, variable
  elif final_type.podtype == 'bool':
    text = _bool_from_npvariant_template.substitute(type=type_name,
                                                    input=input_expr,
                                                    variable=variable,
                                                    success=success,
                                                    context=exception_context)
    return text, variable
  elif final_type.podtype == 'float':
    text = _float_from_npvariant_template.substitute(type=type_name,
                                                     input=input_expr,
                                                     variable=variable,
                                                     success=success,
                                                     context=exception_context)
    return text, variable
  elif final_type.podtype == 'variant':
    return '%s %s(npp, %s);' % (type_name, variable, input_expr), variable
  elif final_type.podtype == 'string':
    text = _string_from_npvariant_template.substitute(type=type_name,
                                                      input=input_expr,
                                                      variable=variable,
                                                      success=success,
                                                      context=exception_context)
    return text, variable
  elif final_type.podtype == 'wstring':
    text = _wstring_from_npvariant_template.substitute(type=type_name,
                                                       input=input_expr,
                                                       variable=variable,
                                                       success=success,
                                                       context=exception_context)
    return text, variable
  else:
    raise UnknownPODType(final_type.podtype)


def NpapiExprToNPVariant(scope, type_defn, variable, expression, output,
                         success, npp):
  """Gets the string to store a value into a NPVariant.

  This function creates a string containing a C++ code snippet that is used to
  store a value into a NPVariant. That operation takes two phases, one that
  allocates necessary NPAPI resources, and that can fail, and one that actually
  sets the NPVariant (that can't fail). If an error occurs, the snippet will
  set the success status variable to false.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type of the value.
    variable: a string, representing a name of a variable that can be used to
      store a reference to the value.
    expression: a string representing the expression that yields the value to
      be stored.
    output: an expression representing a pointer to the NPVariant to store the
      value into.
    success: the name of a bool variable containing the current success status.
    npp: a string, representing the name of the variable that holds the pointer
      to the NPP instance.

  Returns:
    a (string, string) pair, the first string being the code snippet for the
    first phase, and the second one being the code snippet for the second phase.

  Raises:
    UnknownPODType: type_defn is not a known POD type.
  """
  npp = npp  # silence gpylint.
  type_name = cpp_utils.GetScopedName(scope, type_defn)
  final_type = type_defn.GetFinalType()
  if final_type.podtype == 'void':
    return ('%s;' % expression,
            'VOID_TO_NPVARIANT(*%s);' % output)
  elif final_type.podtype == 'int':
    return ('%s %s = %s;' % (type_name, variable, expression),
            'INT32_TO_NPVARIANT(%s, *%s);' % (variable, output))
  elif final_type.podtype == 'bool':
    return ('%s %s = %s;' % (type_name, variable, expression),
            'BOOLEAN_TO_NPVARIANT(%s, *%s);' % (variable, output))
  elif final_type.podtype == 'float':
    return ('%s %s = %s;' % (type_name, variable, expression),
            'DOUBLE_TO_NPVARIANT(static_cast<double>(%s), *%s);' %
            (variable, output))
  elif final_type.podtype == 'variant':
    return ('%s %s = %s' % (type_name, variable, expression),
            '*%s = %s.NPVariant(npp);' % (output, variable))
  elif final_type.podtype == 'string':
    return ('GLUE_PROFILE_START(npp, "StringToNPVariant");\n'
            '%s = StringToNPVariant(%s, %s);\n'
            'GLUE_PROFILE_STOP(npp, "StringToNPVariant");'
            % (success, expression, output),
            '')
  elif final_type.podtype == 'wstring':
    return ('GLUE_PROFILE_START(npp, "String16ToNPVariant");\n'
            '%s = String16ToNPVariant(%s, %s);\n'
            'GLUE_PROFILE_STOP(npp, "String16ToNPVariant");'
            % (success, expression, output),
            '')
  else:
    raise UnknownPODType(final_type.podtype)


def main(unused_argv):
  pass

if __name__ == '__main__':
  main(sys.argv)
