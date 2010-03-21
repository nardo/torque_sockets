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

"""callback binding model module.

This module implements the glue functions for the callback binding model, that
is used by JS callback types. This binding works in reverse in the sense that a
C++ object is created to wrap the JS object.

In C++, objects using this binding model are passed by pointer to an object
that has a corresponding 'Run' function.
Note that every time a callback is passed to C++ a new object is created. The
function called is responsible for deleting it when appropriate.

Note: As of currently, the Callback objects cannot be returned.

For example:
void SetFrameCallback(FrameCallback *callback);
...
vod OnFrame(float elapsed_time) {
  frame_callback->Run(elapsed_time);
}

For JS bindings, they are directly represented by a JavaScript function object.
"""

import string
import sys
import cpp_utils
import java_utils
import npapi_utils
import syntax_tree


class Error(Exception):
  """Base exception for the callback_binding module."""


class InvalidCallbackUsageError(Error):
  """Raised when a callback is used incorrectly."""


def JavaMemberString(scope, type_defn):
  """Gets the representation of a member name in Java.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.

  Returns:
    a string representing the type
  """
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
  """
  return cpp_utils.GetScopedName(scope, type_defn), False


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
  return '%s*' % cpp_utils.GetScopedName(scope, type_defn), False


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
  return '%s*' % cpp_utils.GetScopedName(scope, type_defn), False


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
  return '%s*' % cpp_utils.GetScopedName(scope, type_defn), False


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
  return '%s*' % cpp_utils.GetScopedName(scope, type_defn), False


def CppMutableToNonMutable(scope, type_defn, expr):
  """Gets the string converting a mutable expression to a non-mutable one.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition for the type.
    expr: a string for the mutable expression.

  Returns:
    a string, which is the non-mutable expression.
  """
  scope, type_defn = scope, type_defn  # silence gpylint.
  return expr


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


def JSDocTypeString(type_defn):
  """Gets the representation of a type in JSDoc notation.

  Args:
    type_defn: a Definition for the type.

  Returns:
    a string that is the JSDoc notation of type_defn.
  """
  param_types = []
  for param in type_defn.params:
    param_binding_model = param.type_defn.binding_model
    param_types.append(param_binding_model.JSDocTypeString(param.type_defn))
  return_binding_model = type_defn.type_defn.binding_model
  return_type = return_binding_model.JSDocTypeString(type_defn.type_defn)
  return 'function(%s): %s' % (', '.join(param_types), return_type)


def _MakeRunFunction(scope, type_defn):
  """Creates the Run function for the callback class.

  Args:
    scope: the scope for the function.
    type_defn: the callback definition.

  Returns:
    a Function, with the same parameters as the callback.
  """
  function = syntax_tree.Function(type_defn.source, {}, 'Run', None, [])
  function.type_defn = type_defn.type_defn
  function.parent = scope
  function.params = type_defn.params
  return function


_npapi_binding_glue_header_template = string.Template("""
class ${GlueClass} : public ${BaseClass} {
 public:
  ${GlueClass}(NPP npp, NPObject *npobject);
  virtual ~${GlueClass}();
  virtual ${RunFunction};
 private:
  NPP npp_;
  NPObject *npobject_;
};
${GlueClass} *CreateObject(NPP npp, NPObject *npobject);
""")


def NpapiBindingGlueHeader(scope, type_defn):
  """Gets the NPAPI glue header for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue header.
  """
  glue_class = type_defn.name + '_glue'
  base_class = cpp_utils.GetScopedName(scope, type_defn)
  run_function, unused_check = cpp_utils.GetFunctionPrototype(
      scope, _MakeRunFunction(scope, type_defn), '')
  return _npapi_binding_glue_header_template.substitute(
      GlueClass=glue_class,
      BaseClass=base_class,
      RunFunction=run_function)


_npapi_binding_glue_cpp_template = string.Template("""
${GlueClass}::${GlueClass}(NPP npp, NPObject *npobject)
    : npp_(npp),
      npobject_(npobject) {
  NPN_RetainObject(npobject);
}

${GlueClass}::~${GlueClass}() {
  // TODO: The NPObject we should be releasing here might have already been
  // destroyed by the browser due to a Firefox bug.  The following line is
  // commented out in order to avoid a browser crash.
  //g_browser->releaseobject(npobject_);
}

${RunFunction} {
  ${CallbackGlue};
}

${GlueClass} *CreateObject(NPP npp, NPObject *npobject) {
  return npobject ? new ${GlueClass}(npp, npobject) : NULL;
}
""")


def NpapiBindingGlueCpp(scope, type_defn):
  """Gets the NPAPI glue implementation for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue implementation.
  """
  glue_class = type_defn.name + '_glue'

  run_function, unused_check = cpp_utils.GetFunctionPrototype(
      scope, _MakeRunFunction(scope, type_defn), glue_class + '::')

  if 'async' in type_defn.attributes:
    async_param = 'true'
  else:
    async_param = 'false'

  callback_glue = 'return RunCallback(npp_, npobject_, %s' % async_param
  if type_defn.params:
    callback_glue += ', '.join([''] + [t.name for t in type_defn.params])
  callback_glue += ')'

  return _npapi_binding_glue_cpp_template.substitute(
      GlueClass=glue_class,
      RunFunction=run_function,
      CallbackGlue=callback_glue)


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError

_from_npvariant_template = string.Template("""
  ${success} = NPVARIANT_IS_OBJECT(${input_expr});
  ${type_name} *${variable} = NULL;
  if (${success}) {
    ${variable} = ${namespace}::CreateObject(
       ${npp}, NPVARIANT_TO_OBJECT(${input_expr}));
  } else {
    *error_handle = "Error in " ${context}
        ": a callback must be a Javascript function.";
  }""")

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
  type_name = cpp_utils.GetScopedName(scope, type_defn)
  callback_glue_namespace = npapi_utils.GetGlueFullNamespace(type_defn)
  text = _from_npvariant_template.substitute(success=success,
                                             context=exception_context,
                                             input_expr=input_expr,
                                             type_name=type_name,
                                             variable=variable,
                                             namespace=callback_glue_namespace,
                                             npp=npp)
  return text, variable


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
    InvalidCallbackUsageError: always. This function can't be called for a
    Callback type.
  """
  raise InvalidCallbackUsageError


def main(unused_argv):
  pass

if __name__ == '__main__':
  main(sys.argv)
