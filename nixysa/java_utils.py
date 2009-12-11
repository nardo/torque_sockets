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

"""Utilities for Java code generation.

This module contains a few utilities for Javascripty-C++ code generation.
"""

import cpp_utils
import naming


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
  To access C from F, one needs to refer to it by B.C. This function will
  return the 'B.' part.

  Args:
    scope: the Definition for the scope from which the type must be accessed.
    type_defn: the Definition for the type which must be accessed.

  Returns:
    the prefix string.
  """
  type_stack = type_defn.GetParentScopeStack()
  scope_stack = scope.GetParentScopeStack() + [scope]
  common_prefix = cpp_utils.GetCommonPrefixLength(
      [scope.name for scope in scope_stack],
      [scope.name for scope in type_stack])
  return '.'.join([s.name for s in type_stack[common_prefix:]] + [''])


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
  To access C from F, one needs to refer to it by B.C. This function will
  return exactly that.

  Args:
    scope: the Definition for the scope from which the type must be accessed.
    type_defn: the Definition for the type which must be accessed.

  Returns:
    the scoped reference string.
  """
  return GetScopePrefix(scope, type_defn) + type_defn.name


def GetFunctionParamPrototype(scope, param):
  """Gets the string needed to declare a parameter in a function prototype.

  Args:
    scope: the scope of the prototype.
    param: the Function.Param to declare

  Returns:
    a string that is the declaration of the parameter in the prototype.
  """
  bm = param.type_defn.binding_model
  if param.mutable:
    text = bm.JavaMemberString(scope, param.type_defn)
  else:
    text = bm.JavaMemberString(scope, param.type_defn)
  return '%s %s' % (text, param.name)


def GetFunctionPrototype(scope, obj):
  """Gets the string needed to declare a function prototype.

  Args:
    scope: the scope of the prototype.
    obj: the function to declare.

  Returns:
    a string that is the prototype.
  """
  # For creator functions matching class name, should be capitalized
  if not obj.type_defn:
    func_name = naming.Normalize(obj.name, naming.Capitalized)
  else:
    func_name = naming.Normalize(obj.name, naming.Java)
  param_strings = []
  for p in obj.params:
    param_string = GetFunctionParamPrototype(scope, p)
    param_strings += [param_string]
  param_string = ', '.join(param_strings)

  static = ''
  if 'static' in obj.attributes:
    static = 'static'

  if obj.type_defn:
    bm = obj.type_defn.binding_model
    return_value = bm.JavaMemberString(scope, obj.type_defn)
    prototype = '%s %s %s(%s)' % (static, return_value, func_name, param_string)
  else:
    prototype = '%s %s(%s)' % (static, func_name, param_string)

  return prototype
