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

"""nullable binding model module.

This module implements the glue functions for the nullable binding model,
used for nullable types.

In C++, objects using this binding model are represented by a pointer.
For JS bindings, the nullable type is represented by a JavaScript reference.
"""

import by_pointer_binding
import string


class InvalidNullableUsage(Exception):
  """Raised when a nullable is used in conjuction with a type that is not a
  pointer pointer binding."""
  pass


def JavaMemberString(scope, type_defn):
  """Gets the representation of a member name in Java.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a string representing the type
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.JavaMemberString(scope, data_type)


def CppTypedefString(scope, type_defn):
  """Gets the representation of a type when used in a C++ typedef.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.CppTypedefString(scope, data_type)


def CppMemberString(scope, type_defn):
  """Gets the representation of a type when used as a C++ class member.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.CppMemberString(scope, data_type)


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
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.CppReturnValueString(scope, data_type)


def CppParameterString(scope, type_defn):
  """Gets the representation of a type when used for a function parameter.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the representation of
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.CppParameterString(scope, data_type)


def CppMutableParameterString(scope, type_defn):
  """Gets the representation of a type for a mutable function parameter.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a (string, boolean) pair, the first element being the string representing
    the type, the second element indicating whether or not the definition of
    the type is needed for the expression to be valid.
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.CppMutableParameterString(scope, data_type)


def CppMutableToNonMutable(scope, type_defn, expr):
  """Gets the string converting a mutable expression to a non-mutable one.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.
    expr: a string for the mutable expression.

  Returns:
    a string, which is the non-mutable expression.
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  return data_type_bm.CppMutableToNonMutable(scope, data_type, expr)


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  raise InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


def JSDocTypeString(type_defn):
  """Gets the representation of a type in JSDoc notation.

  Args:
    type_defn: a Definition for the type.

  Returns:
    a string that is the JSDoc notation of type_defn.
  """
  type_defn = type_defn.GetFinalType()
  element_type_defn = type_defn.data_type.GetFinalType()
  type = element_type_defn.binding_model.JSDocTypeString(element_type_defn)
  if type[0] == '!':
    type = type[1:]
  return type


def NpapiBindingGlueHeader(scope, type_defn):
  """Gets the NPAPI glue header for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue header.

  Raises:
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


def NpapiBindingGlueCpp(scope, type_defn):
  """Gets the NPAPI glue implementation for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue implementation.

  Raises:
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage


def NpapiDispatchFunctionHeader(scope, type_defn, variable, npp, success):
  """Gets a header for NPAPI glue dispatch functions.

  This function creates a string containing a C++ code snippet that should be
  included at the beginning of NPAPI glue dispatch functions like Invoke or
  GetProperty. This code snippet will declare and initialize certain variables
  that will be used in the dispatch functions, like the NPObject representing
  the object, or a pointer to the NPP instance.

  First it checks whether the NPVariant is null. If so it simply sets the value
  to null. It relies on the later compilation of the glue to detect when it is
  used with a binding model that cannot be used with nthe value null. It is
  binding model independent.

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
    InvalidNullableUsage: always. This function should not be called on a
      nullable.
  """
  return InvalidNullableUsage

_from_npvariant_template = string.Template("""
${Type} ${variable};
if (NPVARIANT_IS_NULL(${input})) {
  ${variable} = NULL;
} else {
  ${text}
  ${variable} = ${value};
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
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  text, value = data_type_bm.NpapiFromNPVariant(
      scope,
      data_type,
      input_expr,
      variable + '_nullable',
      success,
      exception_context,
      npp);

  data_type_name, dummy = data_type_bm.CppMemberString(scope, data_type)
  nullable_text = _from_npvariant_template.substitute(
      Type=data_type_name,
      variable=variable,
      text=text,
      input=input_expr,
      value=value)

  return (nullable_text, variable)


_to_npvariant_pre_template = string.Template("""
${pre_text}
if (!${variable}) {
  success = true;
}
""")


_to_npvariant_post_template = string.Template("""
if (${variable}) {
  ${post_text}
} else {
  NULL_TO_NPVARIANT(*${output});
}
""")


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
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  pre_text, post_text = data_type_bm.NpapiExprToNPVariant(
      scope,
      data_type,
      variable,
      expression,
      output,
      success,
      npp)

  nullable_pre_text = _to_npvariant_pre_template.substitute(
      variable=variable,
      npp=npp,
      pre_text=pre_text,
      success=success)

  nullable_post_text = _to_npvariant_post_template.substitute(
      variable=variable,
      output=output,
      npp=npp,
      post_text=post_text,
      success=success)

  return nullable_pre_text, nullable_post_text

def main():
  pass

if __name__ == '__main__':
  main()
