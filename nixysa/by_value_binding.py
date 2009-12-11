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

"""by_value binding model module.

This module implements the glue functions for the by_value binding model.

In C++, objects using this binding model are passed by const reference (or by
pointer if mutable), and returned by copy. For example:
void SetValue(const Class &value);
Class GetValue();

For JS bindings, the browser object holds a copy of the C++ object.
"""

import string
import cpp_utils
import java_utils
import npapi_utils
import syntax_tree


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
  return cpp_utils.GetScopedName(scope, type_defn), False


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
  return 'const %s&' % cpp_utils.GetScopedName(scope, type_defn), False


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
  """
  return cpp_utils.GetScopedName(scope, type_defn)


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
  """
  (scope, type_defn) = (scope, type_defn)  # silence gpylint.
  if mutable:
    return '%s->%s(%s)' % (object_expr, method.name, ', '.join(param_exprs))
  else:
    return '%s.%s(%s)' % (object_expr, method.name, ', '.join(param_exprs))


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
  """
  return '%s::%s(%s)' % (cpp_utils.GetScopedName(scope, type_defn),
                         method.name, ', '.join(param_exprs))


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
  """
  method = method  # silence gpylint.
  return '%s(%s)' % (cpp_utils.GetScopedName(scope, type_defn),
                     ', '.join(param_exprs))


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
  """
  (scope, type_defn) = (scope, type_defn)  # silence gpylint.
  return '%s->%s(%s)' % (object_expr, cpp_utils.GetSetterName(field),
                         param_expr)


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
  """
  (scope, type_defn) = (scope, type_defn)  # silence gpylint.
  return '%s.%s()' % (object_expr, cpp_utils.GetGetterName(field))


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
  """
  return '%s::%s(%s)' % (cpp_utils.GetScopedName(scope, type_defn),
                         cpp_utils.GetSetterName(field), param_expr)


def CppGetStatic(scope, type_defn, field):
  """Gets the representation of an expression getting a static field.

  Args:
    scope: a Definition for the scope in which the expression will be written.
    type_defn: a Definition, representing the type of the object containing the
      field being retrieved.
    field: a string, the name of the field to be retrieved.

  Returns:
    a string, which is the expression for getting the field.
  """
  return '%s::%s()' % (cpp_utils.GetScopedName(scope, type_defn),
                       cpp_utils.GetGetterName(field))


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
  return '!' + '.'.join([s.name for s in type_stack[1:]] + [name])


_binding_glue_header_template = string.Template("""
class NPAPIObject: public NPObject {
  NPP npp_;
  ${Class} value_;
 public:
  NPAPIObject(NPP npp): npp_(npp), value_() { }
  NPP npp() {return npp_;}
  const ${Class} &value() {return value_;}
  ${Class} *value_mutable() {return &value_;}
  void set_value(const ${Class} &value) {value_ = value;}
};
NPAPIObject *CreateNPObject(NPP npp, const ${Class} &object);""")


def NpapiBindingGlueHeader(scope, type_defn):
  """Gets the NPAPI glue header for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue header.
  """
  class_name = cpp_utils.GetScopedName(scope, type_defn)
  return _binding_glue_header_template.substitute(Class=class_name)


_binding_glue_cpp_template = string.Template("""
void InitializeGlue(NPP npp) {
  InitializeIds(npp);
}

static NPObject *Allocate(NPP npp, NPClass *theClass) {
  return new NPAPIObject(npp);
}

static void Deallocate(NPObject *header) {
  delete static_cast<NPAPIObject *>(header);
}

NPAPIObject *CreateNPObject(NPP npp, const ${Class} &object) {
  GLUE_PROFILE_START(npp, "createobject");
  NPAPIObject *npobject = static_cast<NPAPIObject *>(
      NPN_CreateObject(npp, &npclass));
  GLUE_PROFILE_STOP(npp, "createobject");
  npobject->set_value(object);
  return npobject;
}""")


def NpapiBindingGlueCpp(scope, type_defn):
  """Gets the NPAPI glue implementation for a given type.

  Args:
    scope: a Definition for the scope in which the glue will be written.
    type_defn: a Definition, representing the type.

  Returns:
    a string, the glue implementation.
  """
  class_name = cpp_utils.GetScopedName(scope, type_defn)
  return _binding_glue_cpp_template.substitute(Class=class_name)


_dispatch_function_header_template = string.Template("""
NPAPIObject *${variable_npobject} = static_cast<NPAPIObject *>(header);
NPP ${npp} = ${variable_npobject}->npp();
""")


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
  """
  (scope, type_defn, success) = (scope, type_defn, success)  # silence gpylint.
  variable_npobject = '%s_npobject' % variable
  text = _dispatch_function_header_template.substitute(
      variable_npobject=variable_npobject, npp=npp)
  return (text, '%s->value_mutable()' % variable_npobject)


def GetMarshalingAttributes(type_defn):
  if 'marshaled' not in type_defn.attributes:
    return {}
  for member_defn in type_defn.defn_list:
    if member_defn.name == 'marshaled':
      if isinstance(member_defn, syntax_tree.Variable):
        return member_defn.attributes
  return {}


_from_npvariant_template = string.Template("""
${Class} ${variable};
if (NPVARIANT_IS_OBJECT(${input})) {
  NPObject *npobject = NPVARIANT_TO_OBJECT(${input});
  if (npobject->_class == ${ClassGlueNS}::GetNPClass()) {
    ${variable} = static_cast<${ClassGlueNS}::NPAPIObject *>(npobject)->value();
    ${success} = true;
  } else {
    *error_handle = "Error in " ${context}
        ": type mismatch.";
    ${success} = false;
  }
} else {
  *error_handle = "Error in " ${context}
      ": was expecting an object.";
  ${success} = false;
}
""")


# A property with the name "marshaled" is interpreted specially to allow value
# types to get converted between differing C++ and JavaScript representations.
# To make a value type behave this way, add the "marshaled" attribute to the
# idl for that type, and implement a single property with userglue, also
# called "marshaled".  The type of the "marshaled" property dictates what type
# JavaScript will see.  The userglue for the property is responsible for
# converting between C++ and JavaScript representations.

_from_npvariant_template_marshaled = string.Template("""
${Class} ${variable};
${success} = ${ClassGlueNS}::SetProperty(&${variable},
                                         ${npp},
                                         NPN_GetStringIdentifier("marshaled"),
                                         &${input},
                                         error_handle);
""")


# This template handles both unmarshaled and marshaled representations. It is
# used for marshaled value types that do not have a getter on their marshaled
# property.
_from_npvariant_template_dual_interface = string.Template("""
${Class} ${variable};
if (NPVARIANT_IS_OBJECT(${input})) {
  NPObject *npobject = NPVARIANT_TO_OBJECT(${input});
  if (npobject->_class == ${ClassGlueNS}::GetNPClass()) {
    ${variable} = static_cast<${ClassGlueNS}::NPAPIObject *>(npobject)->value();
    ${success} = true;
  } else {
    // Note that this error will not be reported. SetProperty will overwrite
    // it. I think it's safer to leave it here though, in case of future
    // modifications.
    *error_handle = "Error in " ${context}
        ": type mismatch.";
    ${success} = false;
  }
} else {
  *error_handle = "Error in " ${context}
      ": was expecting an object.";
  ${success} = false;
}
if (!${success}) {
  // TODO: This code path is not tested at the time of writing.
  ${success} = ${ClassGlueNS}::SetProperty(&${variable},
                                           ${npp},
                                           NPN_GetStringIdentifier("marshaled"),
                                           &${input},
                                           error_handle);
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
  npp = npp  # silence gpylint.
  class_name = cpp_utils.GetScopedName(scope, type_defn)
  glue_namespace = npapi_utils.GetGlueFullNamespace(type_defn)
  marshaling_attributes = GetMarshalingAttributes(type_defn)

  # If the value type has marshaling getter then the C++ type is never exposed
  # to JavaScript and we don't need to check. Otherwise, assume that we might
  # see a wrapped C++ type or an alternative JavaScript representation.
  if 'getter' in marshaling_attributes:
    template = _from_npvariant_template_marshaled
  else:
    if 'setter' in marshaling_attributes:
      template = _from_npvariant_template_dual_interface
    else:
      template = _from_npvariant_template

  text = template.substitute(npp=npp,
                             Class=class_name,
                             ClassGlueNS=glue_namespace,
                             variable=variable,
                             input=input_expr,
                             success=success,
                             context=exception_context)
  return (text, variable)


_expr_to_npvariant_template = string.Template("""
${ClassGlueNS}::NPAPIObject *${variable} =
    ${ClassGlueNS}::CreateNPObject(${npp}, ${expr});
OBJECT_TO_NPVARIANT(${variable}, *${output});
${success} = ${variable} != NULL;
""")

_expr_to_npvariant_template_marshaled = string.Template("""
${success} = ${ClassGlueNS}::GetProperty(${expr},
                                         ${npp},
                                         NPN_GetStringIdentifier("marshaled"),
                                         ${output},
                                         error_handle);
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
  class_name = cpp_utils.GetScopedName(scope, type_defn)
  glue_namespace = npapi_utils.GetGlueFullNamespace(type_defn)
  marshaling_attributes = GetMarshalingAttributes(type_defn)
  if 'getter' in marshaling_attributes:
    template = _expr_to_npvariant_template_marshaled
  else:
    template = _expr_to_npvariant_template
  text = template.substitute(Class=class_name,
                             ClassGlueNS=glue_namespace,
                             variable=variable,
                             output=output,
                             npp=npp,
                             expr=expression,
                             success=success)
  return (text, '')


def main():
  pass

if __name__ == '__main__':
  main()
