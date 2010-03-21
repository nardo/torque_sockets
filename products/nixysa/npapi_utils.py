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

"""Utilities for NPAPI glue.

This module contains a few utilities to help with NPAPI glue generation.
"""

import string
import naming


_id_table_template = string.Template("""
enum {
  ${IDS}NUM_${TABLE}_IDS
};

static NPIdentifier ${table}_ids[NUM_${TABLE}_IDS];
static const NPUTF8 *${table}_names[NUM_${TABLE}_IDS] = {
  ${NAMES}
};""")

_id_init_template = string.Template("""
NPN_GetStringIdentifiers(${table}_names, NUM_${TABLE}_IDS,
                                ${table}_ids);""")

_id_check_template = string.Template("""
for (int i = 0; i < NUM_${TABLE}_IDS; i++)
    if (name == ${table}_ids[i])
        return true;""")

def MakeIdTableDict(id_list, table_name):
  """Generate a substitution dictionary for NPAPI identifiers management.

  This function generates C++ code snippets that are used for NPAPI identifier
  management, and puts them into a dictionary that can be used for template
  substitution during glue generation.

  The substitution dictionary contains 3 keys that are generated based on the
  given table name, one for the declaration of the table, one for the
  initialization of the table, and one to check whether an identifier is in the
  table or not.

  Args:
    id_list: a list of pairs of string. Each element is composed of the name of
      the C++ enum value representing the identifier, and of the quoted name of
      the identifier in JS - e.g. ('METHOD_DO_SOMETHING', '"doSomething"')
    table_name: the name of the identifier table.

  Returns:
    the substitution dictionary.
  """
  id_set = set(id_list)
  words = naming.SplitWords(table_name)
  name_cap = naming.Capitalized(words)
  if id_set:
    ids = ''.join(id + ',\n' for (id, id_name) in id_set)
    names = ',\n  '.join(id_name for (id, id_name) in id_set)
    table_dict = {'TABLE': naming.Upper(words),
                  'table': naming.Lower(words),
                  'Table': name_cap,
                  'IDS': ids,
                  'NAMES': names}
    return {'%sTable' % name_cap: _id_table_template.substitute(table_dict),
            '%sInit' % name_cap: _id_init_template.substitute(table_dict),
            '%sCheck' % name_cap: _id_check_template.substitute(table_dict)}
  else:
    return {'%sTable' % name_cap: '',
            '%sInit' % name_cap: '',
            '%sCheck' % name_cap: ''}


class InvalidScopeType(Exception):
  """Raised when a scope was expected but the Definition is not a scope."""

  def __init__(self, type_defn):
    Exception.__init__(self)
    self.type_defn = type_defn


def GetGlueNamespace(scope):
  """Gets the name of the glue namespace for a scope.

  When generating NPAPI glue for elements within a scope, the glue is scoped by
  a namespace with a generated name to avoid naming conflicts. This function
  generates the name of that glue namespace. This is just the local namespace,
  to get the fully qualified namespace, use GetGlueFullNamespace.

  Args:
    scope: the Definition for the scope.

  Returns:
    the name of the glue namespace.

  Raises:
    InvalidScopeType: the scope parameter is not a Class or a Namespace.
  """
  if scope.defn_type == 'Class':
    return 'class_%s' % scope.name
  elif scope.defn_type == 'Namespace':
    if scope.name:
      return 'namespace_%s' % scope.name
    else:
      # global namespace
      return 'glue'
  elif scope.defn_type == 'Callback':
    return 'callback_%s' % scope.name
  else:
    raise InvalidScopeType(scope)


def GetGlueFullNamespace(scope):
  """Gets the fully qualified name of the glue namespace for a scope.

  When generating NPAPI glue for elements within a scope, the glue is scoped by
  a namespace with a generated name to avoid naming conflicts. This function
  generates the name of that glue namespace. This is the fully qualified
  namespace, to get the local namespace use GetGlueNamespace.

  Args:
    scope: the Definition for the scope.

  Returns:
    the fully qualified name of the glue namespace.

  Raises:
    InvalidScopeType: the scope parameter is not a scope Definition.
  """
  scope = scope.GetFinalType()
  return '::'.join(map(GetGlueNamespace, scope.GetParentScopeStack() + [scope]))


def main():
  pass


if __name__ == '__main__':
  main()
