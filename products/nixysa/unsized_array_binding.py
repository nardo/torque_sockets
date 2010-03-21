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

"""unsized_array binding model module.

This module implements the glue functions for the unsized_array binding model,
used for unsized arrays.

In C++, objects using this binding model are represented by a std::vector of
the C++ member representation of the base type (for example, an array of
by_pointer data will be a std::vector of a pointer to that data type).  The
std::vector itself is passed by const reference (or by pointer if mutable), and
returned by copy. For example:

  void SetValue(const std::vector<Class> &value);
  std::vector<Class> GetValue();

For JS bindings, the array is represented by a JavaScript array.
"""

import string


class InvalidArrayUsage(Exception):
  """Raised when an array is used incorrectly."""
  pass


def _CppTypeString(scope, type_defn):
  """Gets the C++ type of the array.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the array type.

  Returns:
    the C++ type describing the array.
  """
  data_type = type_defn.GetFinalType().data_type
  data_type_bm = data_type.binding_model
  data_type_string, need_defn = data_type_bm.CppMemberString(scope, data_type)
  return 'std::vector<%s >' % data_type_string, need_defn


def JavaMemberString(scope, type_defn):
  """Gets the representation of a member name in Java.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a string representing the type
  """
  # Silence gpylint
  scope, type_defn = scope, type_defn
  return 'Array'


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
  return _CppTypeString(scope, type_defn)


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
  return _CppTypeString(scope, type_defn)


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
  return _CppTypeString(scope, type_defn)


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
  type_string, need_defn = _CppTypeString(scope, type_defn)
  return 'const %s&' % type_string, need_defn


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
  type_string, need_defn = _CppTypeString(scope, type_defn)
  return '%s*' % type_string, need_defn


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


def JSDocTypeString(type_defn):
  """Gets the representation of a type in JSDoc notation.

  Args:
    type_defn: a Definition for the type.

  Returns:
    a string that is the JSDoc notation of type_defn.
  """
  type_defn = type_defn.GetFinalType()
  element_type_defn = type_defn.data_type.GetFinalType()
  return ('!Array.<%s>' %
          element_type_defn.binding_model.JSDocTypeString(element_type_defn))


def NpapiBindingGlueHeader(scope, type_defn):
  """Gets the NPAPI glue header for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue header.

  Raises:
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


def NpapiBindingGlueCpp(scope, type_defn):
  """Gets the NPAPI glue implementation for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue implementation.

  Raises:
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage


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
    InvalidArrayUsage: always. This function should not be called on an array.
  """
  raise InvalidArrayUsage

_from_npvariant_template = string.Template("""
${Type} ${variable};
do {
  if (!NPVARIANT_IS_OBJECT(${input})) {
    *error_handle = "Error in " ${context}
        ": was expecting an array but got a non-object.";
    ${success} = false;
    break;
  }
  NPObject *npobject = NPVARIANT_TO_OBJECT(${input});
  NPVariant value;
  if (!GetNPObjectProperty(${npp}, npobject, "length", &value)) {
    ${success} = false;
    *error_handle = "Error in " ${context}
        ": input had no valid length property.";
  }
  if (!NPVARIANT_IS_NUMBER(value)) {
    NPN_ReleaseVariantValue(&value);
    ${success} = false;
    *error_handle = "Error in " ${context}
        ": input had no valid numeric length property.";
    break;
  }
  int size = static_cast<int>(NPVARIANT_TO_NUMBER(value));
  ${variable}.resize(size);
  for (int i = 0; i < size; i++) {
    if (!GetNPArrayProperty(${npp}, npobject, i, &value)) {
      ${success} = false;
      *error_handle = "Exception while validating " ${context}
          ": array had no value at an index less than "
          "or equal to the index requested.";
      break;
    }
    ${GetValue}
    NPN_ReleaseVariantValue(&value);
    if (!${success}) {
      *error_handle = "Exception while validating " ${context}
          ": a value at an index less than or equal to the "
          "index requested was missing or of invalid type.";
      break;
    }
    ${variable}[i] = ${expr};
  }
} while (false);
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
  text, expr = data_type_bm.NpapiFromNPVariant(scope, data_type, 'value',
                                               '%s_i' % variable, success,
                                               exception_context, npp)
  type_name, unused_need_defn = _CppTypeString(scope, type_defn)
  text = _from_npvariant_template.substitute(Type=type_name,
                                             variable=variable,
                                             input=input_expr,
                                             GetValue=text,
                                             expr=expr,
                                             npp=npp,
                                             success=success,
                                             context=exception_context)
  return (text, variable)


_expr_to_npvariant_template = string.Template("""
${Type} ${variable} = ${expr};
NPObject *${variable}_npobject = CreateArray(${npp});
static const char *array_push_identifier = "push";
NPIdentifier identifier = NPN_GetStringIdentifier(array_push_identifier);
for (${Type}::size_type i = 0; i < ${variable}.size(); ++i) {
  NPVariant value;
  ${SetValuePre}
  if (!${success}) break;
  ${SetValuePost}
  NPVariant result;
  NPN_Invoke(${npp}, ${variable}_npobject, identifier, &value, 1, &result);
  NPN_ReleaseVariantValue(&value);
  NPN_ReleaseVariantValue(&result);
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
  pre, post = data_type_bm.NpapiExprToNPVariant(scope, data_type,
                                                '%s_value' % variable,
                                                '%s[i]' % variable, '&value',
                                                success, npp)
  type_name, unused_need_defn = _CppTypeString(scope, type_defn)
  text = _expr_to_npvariant_template.substitute(Type=type_name,
                                                variable=variable,
                                                expr=expression,
                                                npp=npp,
                                                SetValuePre=pre,
                                                SetValuePost=post,
                                                success=success)
  return (text, 'OBJECT_TO_NPVARIANT(%s_npobject, *%s);' % (variable, output))


def main():
  pass

if __name__ == '__main__':
  main()
