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

"""Test for syntax_tree."""

import unittest
import idl_parser
import syntax_tree

_location = idl_parser.SourceLocation(idl_parser.File('test.idl'), 0)


def MakeType(name):
  return syntax_tree.Typename(_location, {}, name)


def MakeScope(name):
  return ContextMock([], [], name, None)


class TypeReferenceMock(syntax_tree.TypeReference):
  def __init__(self, return_type):
    syntax_tree.TypeReference.__init__(self, _location)
    self.return_type = return_type
    self.context = None
    self.scoped = None

  def GetTypeInternal(self, context, scoped):
    self.context = context
    self.scoped = scoped
    return self.return_type


class ContextMock(syntax_tree.Definition):
  defn_type = 'ContextMock'

  def __init__(self, types_list, scopes_list, name, parent):
    syntax_tree.Definition.__init__(self, _location, [], name)
    self.is_scope = True
    self.types_dict = dict([(type_defn.name, type_defn) for type_defn
                            in types_list])
    self.scopes_list = scopes_list
    for o in scopes_list + types_list:
      o.parent = self
    self.parent = parent

  def LookUpType(self, name):
    if name in self.types_dict:
      return self.types_dict[name]
    else:
      return None

  def FindScopes(self, name):
    return [scope for scope in self.scopes_list if scope.name == name]


class TypeReferenceTest(unittest.TestCase):
  def setUp(self):
    self.type_defn = MakeType('Type')
    self.context = ContextMock([], [], None, None)

  def testGetTypeSuccess(self):
    mock = TypeReferenceMock(self.type_defn)
    return_type = mock.GetType(self.context)
    self.assertEquals(return_type, self.type_defn)
    self.assertEquals(mock.context, self.context)
    self.assertEquals(mock.scoped, False)

  def testGetTypeFailure(self):
    mock = TypeReferenceMock(None)
    self.assertRaises(syntax_tree.TypeNotFoundError, mock.GetType, self.context)
    self.assertEquals(mock.context, self.context)
    self.assertEquals(mock.scoped, False)


class NameTypeReferenceTest(unittest.TestCase):
  def setUp(self):
    # Context2 {
    #   Type1;
    #   Type3;
    #   Context1 {
    #     Type1;
    #     Type2;
    #   }
    # }
    self.type1_c1 = MakeType('Type1')
    self.type2_c1 = MakeType('Type2')
    self.context1 = ContextMock([self.type1_c1, self.type2_c1], [], 'Context1',
                                None)
    self.type1_c2 = MakeType('Type1')
    self.type3_c2 = MakeType('Type3')
    self.context2 = ContextMock([self.type1_c2, self.type3_c2], [self.context1],
                                'Context2', None)
    self.type1_ref = syntax_tree.NameTypeReference(_location, 'Type1')
    self.type2_ref = syntax_tree.NameTypeReference(_location, 'Type2')
    self.type3_ref = syntax_tree.NameTypeReference(_location, 'Type3')

  def testGetTypeInScope(self):
    self.assertEquals(self.type1_c1, self.type1_ref.GetType(self.context1))
    self.assertEquals(self.type2_c1, self.type2_ref.GetType(self.context1))
    self.assertEquals(self.type3_c2, self.type3_ref.GetType(self.context2))

  def testGetTypeFromOuterScope(self):
    self.assertEquals(self.type1_c2, self.type1_ref.GetType(self.context2))
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.type2_ref.GetType, self.context2)

  def testGetTypeFromInnerScope(self):
    self.assertEquals(self.type3_c2, self.type3_ref.GetType(self.context1))


class ScopedTypeReferenceTest(unittest.TestCase):
  def setUp(self):
    # Context3 {
    #   Type1;
    #   Type2;
    #   Context1 {
    #     Type1;
    #   }
    #   Context2 {
    #     Type1;
    #   }
    # }
    self.type1_c1 = MakeType('Type1')
    self.context1 = ContextMock([self.type1_c1], [], 'Context1', None)
    self.type1_c2 = MakeType('Type1')
    self.context2 = ContextMock([self.type1_c2], [], 'Context2', None)
    self.type1_c3 = MakeType('Type1')
    self.type2_c3 = MakeType('Type2')
    self.context3 = ContextMock([self.type1_c3, self.type2_c3],
                                [self.context1, self.context2], 'Context3',
                                None)
    self.type1_ref = syntax_tree.NameTypeReference(_location, 'Type1')
    self.type2_ref = syntax_tree.NameTypeReference(_location, 'Type2')
    self.c1_t1_ref = syntax_tree.ScopedTypeReference(_location, 'Context1',
                                                     self.type1_ref)
    self.c2_t1_ref = syntax_tree.ScopedTypeReference(_location, 'Context2',
                                                     self.type1_ref)
    self.c1_t2_ref = syntax_tree.ScopedTypeReference(_location, 'Context1',
                                                     self.type2_ref)

  def testGetTypeFromOuterScope(self):
    self.assertEquals(self.type1_c1, self.c1_t1_ref.GetType(self.context3))
    self.assertEquals(self.type1_c2, self.c2_t1_ref.GetType(self.context3))

  def testGetTypeFromInnerScope(self):
    self.assertEquals(self.type1_c1, self.c1_t1_ref.GetType(self.context1))
    self.assertEquals(self.type1_c2, self.c2_t1_ref.GetType(self.context1))
    self.assertEquals(self.type1_c1, self.c1_t1_ref.GetType(self.context2))
    self.assertEquals(self.type1_c2, self.c2_t1_ref.GetType(self.context2))

  def testGetInexistentType(self):
    self.assertRaises(syntax_tree.TypeNotFoundError, self.c1_t2_ref.GetType,
                      self.context1)
    self.assertRaises(syntax_tree.TypeNotFoundError, self.c1_t2_ref.GetType,
                      self.context2)
    self.assertRaises(syntax_tree.TypeNotFoundError, self.c1_t2_ref.GetType,
                      self.context3)


class ArrayTypeReferenceTest(unittest.TestCase):
  def setUp(self):
    self.type_defn = MakeType('Type')
    self.context = ContextMock([self.type_defn], [], 'Context', None)
    self.type_ref = syntax_tree.NameTypeReference(_location, 'Type')
    self.nonexist_type_ref = syntax_tree.NameTypeReference(_location,
                                                           'NonexistentType')

  def testGetType(self):
    unsized_ref = syntax_tree.ArrayTypeReference(_location, self.type_ref, None)
    unsized_array = self.type_defn.GetArrayType(None)
    self.assertEquals(unsized_ref.GetType(self.context), unsized_array)
    sized_ref = syntax_tree.ArrayTypeReference(_location, self.type_ref, 3)
    sized_array = self.type_defn.GetArrayType(3)
    self.assertEquals(sized_ref.GetType(self.context), sized_array)

  def testGetInexistentType(self):
    unsized_ref = syntax_tree.ArrayTypeReference(_location,
                                                 self.nonexist_type_ref, None)
    self.assertRaises(syntax_tree.TypeNotFoundError, unsized_ref.GetType,
                      self.context)
    sized_ref = syntax_tree.ArrayTypeReference(_location,
                                               self.nonexist_type_ref, 5)
    self.assertRaises(syntax_tree.TypeNotFoundError, sized_ref.GetType,
                      self.context)


class QualifiedTypeReferenceTest(unittest.TestCase):
  def setUp(self):
    self.type_defn = MakeType('Type')
    self.context = ContextMock([self.type_defn], [], 'Context', None)
    self.type_ref = syntax_tree.NameTypeReference(_location, 'Type')
    self.nonexist_type_ref = syntax_tree.NameTypeReference(_location,
                                                           'NonexistentType')

  def testGetType(self):
    qualified_ref = syntax_tree.QualifiedTypeReference(_location, 'const',
                                                       self.type_ref)
    self.assertEquals(qualified_ref.GetType(self.context), self.type_defn)

  def testGetInexistentType(self):
    qualified_ref = syntax_tree.QualifiedTypeReference(_location, 'const',
                                                       self.nonexist_type_ref)
    self.assertRaises(syntax_tree.TypeNotFoundError, qualified_ref.GetType,
                      self.context)


class DefinitionTest(unittest.TestCase):
  def setUp(self):
    pass

  def testGetParentScopeStack(self):
    definition1 = syntax_tree.Definition(_location, [], 'Definition1')
    definition1.is_scope = True
    definition2 = syntax_tree.Definition(_location, [], 'Definition2')
    definition2.parent = definition1
    definition2.is_scope = True
    definition3 = syntax_tree.Definition(_location, [], 'Definition3')
    definition3.parent = definition2
    self.assertEquals(definition1.GetParentScopeStack(), [])
    self.assertEquals(definition2.GetParentScopeStack(), [definition1])
    self.assertEquals(definition3.GetParentScopeStack(), [definition1,
                                                          definition2])

  def testGetDefinitionInclude(self):
    definition1 = syntax_tree.Definition(_location, [], 'Definition1')
    self.assertEquals(definition1.GetDefinitionInclude(), _location.file.header)
    include = '/path/to/header.h'
    definition2 = syntax_tree.Definition(_location, {'include': include},
                                         'Definition2')
    self.assertEquals(definition2.GetDefinitionInclude(), include)

  def testGetArrayTypeFail(self):
    definition = syntax_tree.Definition(_location, [], 'Definition')
    definition.is_type = False
    self.assertRaises(syntax_tree.ArrayOfNonTypeError, definition.GetArrayType,
                      None)
    self.assertRaises(syntax_tree.ArrayOfNonTypeError, definition.GetArrayType,
                      5)

  def testGetArrayType(self):
    definition = syntax_tree.Definition(_location, [], 'Definition')
    definition.is_type = True
    unsized = definition.GetArrayType(None)
    self.assertEquals(unsized.data_type, definition)
    self.assertEquals(unsized.size, None)
    self.assertEquals(unsized, definition.GetArrayType(None))
    sized = definition.GetArrayType(3)
    self.assertEquals(sized.data_type, definition)
    self.assertEquals(sized.size, 3)
    self.assertEquals(sized, definition.GetArrayType(3))

  def testLookUpTypeRecursive(self):
    type1_c1 = MakeType('Type1')
    type2_c1 = MakeType('Type2')
    context1 = ContextMock([type1_c1, type2_c1], [], 'Context1', None)
    type1_c2 = MakeType('Type1')
    context2 = ContextMock([type1_c2], [], 'Context2', context1)
    self.assertEquals(context1.LookUpTypeRecursive('Type1'), type1_c1)
    self.assertEquals(context1.LookUpTypeRecursive('Type2'), type2_c1)
    self.assertEquals(context1.LookUpTypeRecursive('Type3'), None)
    self.assertEquals(context2.LookUpTypeRecursive('Type1'), type1_c2)
    self.assertEquals(context2.LookUpTypeRecursive('Type2'), type2_c1)
    self.assertEquals(context2.LookUpTypeRecursive('Type3'), None)

  def testFindScopesRecursive(self):
    scope1_c1 = MakeScope('Scope1')
    scope2_c1 = MakeScope('Scope2')
    context1 = ContextMock([], [scope1_c1, scope2_c1], 'Context1', None)
    scope1_c2 = MakeScope('Scope1')
    context2 = ContextMock([], [scope1_c2], 'Context2', context1)
    self.assertEquals(context1.FindScopesRecursive('Scope1'), [scope1_c1])
    self.assertEquals(context1.FindScopesRecursive('Scope2'), [scope2_c1])
    self.assertEquals(context2.FindScopesRecursive('Scope1'), [scope1_c2,
                                                               scope1_c1])
    self.assertEquals(context2.FindScopesRecursive('Scope2'), [scope2_c1])
    context3 = ContextMock([], [context1], 'Context3', None)
    self.assertEquals(context3.FindScopesRecursive('Scope1'), [])
    self.assertEquals(context3.FindScopesRecursive('Scope2'), [])

  def testSetBindingModel(self):
    class DefinitionMock(syntax_tree.Definition):
      defn_type = 'DefinitionMock'

      def __init__(self, name, binding_model_name):
        syntax_tree.Definition.__init__(self, _location, [], name)
        self.binding_model_name = binding_model_name
        self.is_type = True

      def LookUpBindingModel(self):
        return self.binding_model_name

    bm_binding_model = object()
    unsized_array_binding_model = object()
    sized_array_binding_model = object()
    binding_models = {'bm': bm_binding_model,
                      'unsized_array': unsized_array_binding_model,
                      'sized_array': sized_array_binding_model}
    definition1 = DefinitionMock('Definition1', 'bm')
    definition1.SetBindingModel(binding_models)
    self.assertEquals(definition1.binding_model, bm_binding_model)
    definition2 = DefinitionMock('Definition2', 'non_bm')
    self.assertRaises(syntax_tree.UnknownBindingModelError,
                      definition2.SetBindingModel, binding_models)
    definition3 = DefinitionMock('Definition3', 'bm')
    unsized_array = definition3.GetArrayType(None)
    sized_array = definition3.GetArrayType(21)
    definition3.SetBindingModel(binding_models)
    self.assertEquals(unsized_array.binding_model, unsized_array_binding_model)
    self.assertEquals(sized_array.binding_model, sized_array_binding_model)


class ClassTest(unittest.TestCase):
  def setUp(self):
    self.type1_c1 = MakeType('Type1')
    self.type2 = MakeType('Type2')
    self.scope1_c1 = MakeScope('Scope1')
    self.scope2 = MakeScope('Scope2')
    self.class1 = syntax_tree.Class(_location, {'binding_model': 'bm1'},
                                    'Class1', None,
                                    [self.type1_c1, self.type2, self.scope1_c1,
                                     self.scope2])
    self.type1_c2 = MakeType('Type1')
    self.type3 = MakeType('Type3')
    self.scope1_c2 = MakeScope('Scope1')
    self.scope3 = MakeScope('Scope3')
    self.class1_ref = TypeReferenceMock(self.class1)
    self.class2 = syntax_tree.Class(_location, {}, 'Class2', self.class1_ref,
                                    [self.type1_c2, self.type3, self.scope1_c2,
                                     self.scope3])
    self.class3 = syntax_tree.Class(_location, {},
                                    'Class3', None, [self.class1, self.class2])
    invalid_base = MakeType('Type5')
    self.class4 = syntax_tree.Class(_location, {}, 'Class4',
                                    TypeReferenceMock(invalid_base), [])
    self.type1_global = MakeType('Type1')
    self.type4 = MakeType('Type4')
    self.context = ContextMock([self.class3, self.type1_global, self.type4],
                               [self.class3], 'Context', None)

  def testTypeScope(self):
    for c in [self.class1, self.class2, self.class3, self.class4]:
      self.assertTrue(c.is_type)
      self.assertTrue(c.is_scope)

  def testParent(self):
    self.assertEquals(self.type1_c1.parent, self.class1)
    self.assertEquals(self.type2.parent, self.class1)
    self.assertEquals(self.scope1_c1.parent, self.class1)
    self.assertEquals(self.scope2.parent, self.class1)
    self.assertEquals(self.type1_c2.parent, self.class2)
    self.assertEquals(self.type3.parent, self.class2)
    self.assertEquals(self.scope1_c2.parent, self.class2)
    self.assertEquals(self.scope3.parent, self.class2)
    self.assertEquals(self.class1.parent, self.class3)
    self.assertEquals(self.class2.parent, self.class3)

  def testResolveTypeReferences(self):
    self.assertEquals(self.class1._types_resolved, False)
    self.class1.ResolveTypeReferences()
    self.assertEquals(self.class1.base_type, None)
    self.assertEquals(self.class1._types_resolved, True)
    self.class2.ResolveTypeReferences()
    self.assertEquals(self.class2._types_resolved, True)
    self.assertEquals(self.class2.base_type, self.class1)
    # check that the type resolution for class2 happened in the correct scope
    self.assertEquals(self.class1_ref.context, self.class3)
    self.assertEquals(self.class1_ref.scoped, False)
    self.assertRaises(syntax_tree.DerivingFromNonClassError,
                      self.class4.ResolveTypeReferences)

  def testGetBaseSafe(self):
    self.assertEquals(self.class1.GetBaseSafe(), None)
    self.assertEquals(self.class2.GetBaseSafe(), self.class1)
    self.assertEquals(self.class3.GetBaseSafe(), None)
    self.assertRaises(syntax_tree.DerivingFromNonClassError,
                      self.class4.GetBaseSafe)

  def testGetObjectsRecursive(self):
    class1_list = self.class1.GetObjectsRecursive()
    self.assertEquals(class1_list[0], self.class1)
    class1_list.sort()
    class1_list_expected = [self.class1, self.type1_c1, self.type2,
                            self.scope1_c1, self.scope2]
    class1_list_expected.sort()
    self.assertEquals(class1_list, class1_list_expected)
    class2_list = self.class2.GetObjectsRecursive()
    self.assertEquals(class2_list[0], self.class2)
    class2_list.sort()
    class2_list_expected = [self.class2, self.type1_c2, self.type3,
                            self.scope1_c2, self.scope3]
    class2_list_expected.sort()
    self.assertEquals(class2_list, class2_list_expected)
    class3_list = self.class3.GetObjectsRecursive()
    self.assertEquals(class3_list[0], self.class3)
    class3_list.sort()
    class3_list_expected = [self.class3] + class1_list + class2_list
    class3_list_expected.sort()
    self.assertEquals(class3_list, class3_list_expected)

  def testLookUpType(self):
    self.assertEquals(self.class1.LookUpType('Type1'), self.type1_c1)
    self.assertEquals(self.class1.LookUpType('Type2'), self.type2)
    self.assertEquals(self.class1.LookUpType('Type3'), None)
    self.assertEquals(self.class1.LookUpType('Type4'), None)
    self.assertEquals(self.class1.LookUpType('Class1'), None)
    self.assertEquals(self.class1.LookUpType('Class2'), None)
    self.assertEquals(self.class1.LookUpType('Class3'), None)
    self.assertEquals(self.class1.LookUpType('Scope1'), None)
    self.assertEquals(self.class1.LookUpType('Scope2'), None)
    self.assertEquals(self.class1.LookUpType('Scope3'), None)

    self.assertEquals(self.class2.LookUpType('Type1'), self.type1_c2)
    self.assertEquals(self.class2.LookUpType('Type2'), self.type2)
    self.assertEquals(self.class2.LookUpType('Type3'), self.type3)
    self.assertEquals(self.class2.LookUpType('Type4'), None)
    self.assertEquals(self.class2.LookUpType('Class1'), None)
    self.assertEquals(self.class2.LookUpType('Class2'), None)
    self.assertEquals(self.class2.LookUpType('Class3'), None)
    self.assertEquals(self.class2.LookUpType('Scope1'), None)
    self.assertEquals(self.class2.LookUpType('Scope2'), None)
    self.assertEquals(self.class2.LookUpType('Scope3'), None)

    self.assertEquals(self.class3.LookUpType('Type1'), None)
    self.assertEquals(self.class3.LookUpType('Type2'), None)
    self.assertEquals(self.class3.LookUpType('Type3'), None)
    self.assertEquals(self.class3.LookUpType('Type4'), None)
    self.assertEquals(self.class3.LookUpType('Class1'), self.class1)
    self.assertEquals(self.class3.LookUpType('Class2'), self.class2)
    self.assertEquals(self.class3.LookUpType('Class3'), None)
    self.assertEquals(self.class3.LookUpType('Scope1'), None)
    self.assertEquals(self.class3.LookUpType('Scope2'), None)
    self.assertEquals(self.class3.LookUpType('Scope3'), None)

  def testFindScopes(self):
    self.assertEquals(self.class1.FindScopes('Class1'), [])
    self.assertEquals(self.class1.FindScopes('Class2'), [])
    self.assertEquals(self.class1.FindScopes('Type1'), [])
    self.assertEquals(self.class1.FindScopes('Type2'), [])
    self.assertEquals(self.class1.FindScopes('Scope1'), [self.scope1_c1])
    self.assertEquals(self.class1.FindScopes('Scope2'), [self.scope2])
    self.assertEquals(self.class1.FindScopes('Scope3'), [])

    self.assertEquals(self.class2.FindScopes('Class1'), [])
    self.assertEquals(self.class2.FindScopes('Class2'), [])
    self.assertEquals(self.class2.FindScopes('Type1'), [])
    self.assertEquals(self.class2.FindScopes('Type2'), [])
    self.assertEquals(self.class2.FindScopes('Scope1'), [self.scope1_c2,
                                                         self.scope1_c1])
    self.assertEquals(self.class2.FindScopes('Scope2'), [self.scope2])
    self.assertEquals(self.class2.FindScopes('Scope3'), [self.scope3])

    self.assertEquals(self.class3.FindScopes('Class1'), [self.class1])
    self.assertEquals(self.class3.FindScopes('Class2'), [self.class2])
    self.assertEquals(self.class3.FindScopes('Type1'), [])
    self.assertEquals(self.class3.FindScopes('Type2'), [])
    self.assertEquals(self.class3.FindScopes('Scope1'), [])
    self.assertEquals(self.class3.FindScopes('Scope2'), [])
    self.assertEquals(self.class3.FindScopes('Scope3'), [])

  def testLookUpBindingModel(self):
    self.assertEquals(self.class1.LookUpBindingModel(), 'bm1')
    self.assertEquals(self.class2.LookUpBindingModel(), 'bm1')
    self.assertEquals(self.class3.LookUpBindingModel(), None)
    self.assertRaises(syntax_tree.DerivingFromNonClassError,
                      self.class4.LookUpBindingModel)


class NamespaceTest(unittest.TestCase):
  def setUp(self):
    self.type1_n1 = MakeType('Type1')
    self.type2 = MakeType('Type2')
    self.scope1_n1 = MakeScope('Scope1')
    self.scope2 = MakeScope('Scope2')
    self.ns1 = syntax_tree.Namespace(_location, {}, 'ns1',
                                     [self.type1_n1, self.type2,
                                      self.scope1_n1, self.scope2])
    self.type1_n2 = MakeType('Type1')
    self.type3 = MakeType('Type3')
    self.scope1_n2 = MakeScope('Scope1')
    self.scope3 = MakeScope('Scope3')
    self.ns2 = syntax_tree.Namespace(_location, {}, 'ns2',
                                     [self.type1_n2, self.type3,
                                      self.scope1_n2, self.scope3])
    self.type_ns1 = MakeType('ns1')
    self.ns3 = syntax_tree.Namespace(_location, {}, 'ns3', [self.ns1, self.ns2,
                                                            self.type_ns1])

  def testTypeScope(self):
    for ns in [self.ns1, self.ns2, self.ns3]:
      self.assertFalse(ns.is_type)
      self.assertTrue(ns.is_scope)

  def testParents(self):
    self.assertEquals(self.type1_n1.parent, self.ns1)
    self.assertEquals(self.type2.parent, self.ns1)
    self.assertEquals(self.scope1_n1.parent, self.ns1)
    self.assertEquals(self.scope2.parent, self.ns1)
    self.assertEquals(self.type1_n2.parent, self.ns2)
    self.assertEquals(self.type3.parent, self.ns2)
    self.assertEquals(self.scope1_n2.parent, self.ns2)
    self.assertEquals(self.scope3.parent, self.ns2)
    self.assertEquals(self.ns1.parent, self.ns3)
    self.assertEquals(self.ns2.parent, self.ns3)
    self.assertEquals(self.type_ns1.parent, self.ns3)

  def testGetObjectsRecursive(self):
    ns1_list = self.ns1.GetObjectsRecursive()
    self.assertEquals(ns1_list[0], self.ns1)
    ns1_list.sort()
    ns1_list_expected = [self.ns1, self.type1_n1, self.type2,
                         self.scope1_n1, self.scope2]
    ns1_list_expected.sort()
    self.assertEquals(ns1_list, ns1_list_expected)
    ns2_list = self.ns2.GetObjectsRecursive()
    self.assertEquals(ns2_list[0], self.ns2)
    ns2_list.sort()
    ns2_list_expected = [self.ns2, self.type1_n2, self.type3,
                         self.scope1_n2, self.scope3]
    ns2_list_expected.sort()
    self.assertEquals(ns2_list, ns2_list_expected)
    ns3_list = self.ns3.GetObjectsRecursive()
    self.assertEquals(ns3_list[0], self.ns3)
    ns3_list.sort()
    ns3_list_expected = [self.ns3, self.type_ns1] + ns1_list + ns2_list
    ns3_list_expected.sort()
    self.assertEquals(ns3_list, ns3_list_expected)

  def testLookUpType(self):
    self.assertEquals(self.ns1.LookUpType('Type1'), self.type1_n1)
    self.assertEquals(self.ns1.LookUpType('Type2'), self.type2)
    self.assertEquals(self.ns1.LookUpType('Type3'), None)
    self.assertEquals(self.ns1.LookUpType('ns1'), None)
    self.assertEquals(self.ns1.LookUpType('ns2'), None)
    self.assertEquals(self.ns1.LookUpType('ns3'), None)
    self.assertEquals(self.ns1.LookUpType('Scope1'), None)
    self.assertEquals(self.ns1.LookUpType('Scope2'), None)
    self.assertEquals(self.ns1.LookUpType('Scope3'), None)

    self.assertEquals(self.ns2.LookUpType('Type1'), self.type1_n2)
    self.assertEquals(self.ns2.LookUpType('Type2'), None)
    self.assertEquals(self.ns2.LookUpType('Type3'), self.type3)
    self.assertEquals(self.ns2.LookUpType('ns1'), None)
    self.assertEquals(self.ns2.LookUpType('ns2'), None)
    self.assertEquals(self.ns2.LookUpType('ns3'), None)
    self.assertEquals(self.ns2.LookUpType('Scope1'), None)
    self.assertEquals(self.ns2.LookUpType('Scope2'), None)
    self.assertEquals(self.ns2.LookUpType('Scope3'), None)

    self.assertEquals(self.ns3.LookUpType('Type1'), None)
    self.assertEquals(self.ns3.LookUpType('Type2'), None)
    self.assertEquals(self.ns3.LookUpType('Type3'), None)
    self.assertEquals(self.ns3.LookUpType('ns1'), self.type_ns1)
    self.assertEquals(self.ns3.LookUpType('ns2'), None)
    self.assertEquals(self.ns3.LookUpType('ns3'), None)
    self.assertEquals(self.ns3.LookUpType('Scope1'), None)
    self.assertEquals(self.ns3.LookUpType('Scope2'), None)
    self.assertEquals(self.ns3.LookUpType('Scope3'), None)

  def testFindScopes(self):
    self.assertEquals(self.ns1.FindScopes('ns1'), [])
    self.assertEquals(self.ns1.FindScopes('ns2'), [])
    self.assertEquals(self.ns1.FindScopes('Type1'), [])
    self.assertEquals(self.ns1.FindScopes('Type2'), [])
    self.assertEquals(self.ns1.FindScopes('Scope1'), [self.scope1_n1])
    self.assertEquals(self.ns1.FindScopes('Scope2'), [self.scope2])
    self.assertEquals(self.ns1.FindScopes('Scope3'), [])

    self.assertEquals(self.ns2.FindScopes('ns1'), [])
    self.assertEquals(self.ns2.FindScopes('ns2'), [])
    self.assertEquals(self.ns2.FindScopes('Type1'), [])
    self.assertEquals(self.ns2.FindScopes('Type2'), [])
    self.assertEquals(self.ns2.FindScopes('Scope1'), [self.scope1_n2])
    self.assertEquals(self.ns2.FindScopes('Scope2'), [])
    self.assertEquals(self.ns2.FindScopes('Scope3'), [self.scope3])

    self.assertEquals(self.ns3.FindScopes('ns1'), [self.ns1])
    self.assertEquals(self.ns3.FindScopes('ns2'), [self.ns2])
    self.assertEquals(self.ns3.FindScopes('Type1'), [])
    self.assertEquals(self.ns3.FindScopes('Type2'), [])
    self.assertEquals(self.ns3.FindScopes('Scope1'), [])
    self.assertEquals(self.ns3.FindScopes('Scope2'), [])
    self.assertEquals(self.ns3.FindScopes('Scope3'), [])

  def testMergeLookUpScope(self):
    type1 = MakeType('Type1')
    type2 = MakeType('Type2')
    scope1 = MakeScope('Scope1')
    scope2 = MakeScope('Scope2')
    ns1 = syntax_tree.Namespace(_location, {}, 'ns', [type1, type2, scope1,
                                                      scope2])
    ns1_list_copy = ns1.defn_list[:]
    type3 = MakeType('Type3')
    type4 = MakeType('Type4')
    scope3 = MakeScope('Scope3')
    scope4 = MakeScope('Scope4')
    ns2 = syntax_tree.Namespace(_location, {}, 'ns', [type3, type4, scope3,
                                                      scope4])
    ns2_list_copy = ns2.defn_list[:]
    ns1.MergeLookUpScope(ns2)
    self.assertEquals(ns1.scope, ns2.scope)
    for ns in [ns1, ns2]:
      self.assertEquals(ns.LookUpType('Type1'), type1)
      self.assertEquals(ns.LookUpType('Type2'), type2)
      self.assertEquals(ns.LookUpType('Type3'), type3)
      self.assertEquals(ns.LookUpType('Type4'), type4)
      self.assertEquals(ns.FindScopes('Scope1'), [scope1])
      self.assertEquals(ns.FindScopes('Scope2'), [scope2])
      self.assertEquals(ns.FindScopes('Scope3'), [scope3])
      self.assertEquals(ns.FindScopes('Scope4'), [scope4])
    self.assertEquals(ns1.defn_list, ns1_list_copy)
    self.assertEquals(ns2.defn_list, ns2_list_copy)


class EnumTest(unittest.TestCase):
  def setUp(self):
    value1 = syntax_tree.Enum.Value('VALUE1', 1)
    value2 = syntax_tree.Enum.Value('VALUE2', None)
    self.enum1 = syntax_tree.Enum(_location, {}, 'Enum1', [value1, value2])
    self.enum2 = syntax_tree.Enum(_location, {'binding_model': 'ignored'},
                                  'Enum1', [value1, value2])

  def testTypeScope(self):
    for e in [self.enum1, self.enum2]:
      self.assertTrue(e.is_type)
      self.assertFalse(e.is_scope)

  def testGetObjectsRecursive(self):
    for e in [self.enum1, self.enum2]:
      self.assertEquals(e.GetObjectsRecursive(), [e])

  def testLookUpBindingModel(self):
    for e in [self.enum1, self.enum2]:
      self.assertEquals(e.LookUpBindingModel(), 'enum')


class FunctionTest(unittest.TestCase):
  def setUp(self):
    self.type1 = MakeType('Type1')
    self.type2 = MakeType('Type2')
    self.f1_p1_ref = TypeReferenceMock(self.type1)
    self.f1_p2_ref = TypeReferenceMock(self.type2)
    self.function1 = syntax_tree.Function(_location, {}, 'Function1', None,
                                          [(self.f1_p1_ref, 'p1'),
                                           (self.f1_p2_ref, 'p2')])
    self.f2_ret_ref = TypeReferenceMock(self.type1)
    self.function2 = syntax_tree.Function(_location, {}, 'Function2',
                                          self.f2_ret_ref, [])
    self.f3_p1_ref = TypeReferenceMock(None)
    self.function3 = syntax_tree.Function(_location, {}, 'Function3', None,
                                          [(self.f3_p1_ref, 'p1')])
    self.f4_ret_ref = TypeReferenceMock(None)
    self.function4 = syntax_tree.Function(_location, {}, 'Function4',
                                          self.f4_ret_ref, [])
    self.scope = ContextMock([self.type1, self.type2, self.function1,
                              self.function2, self.function3, self.function4],
                             [], 'globals', None)

  def testTypeScope(self):
    for f in [self.function1, self.function2, self.function3, self.function4]:
      self.assertFalse(f.is_type)
      self.assertFalse(f.is_scope)

  def testParams(self):
    self.assertEquals(self.function1.params[0].name, 'p1')
    self.assertEquals(self.function1.params[0].mutable, False)
    self.assertEquals(self.function1.params[1].name, 'p2')
    self.assertEquals(self.function1.params[1].mutable, False)
    self.assertEquals(self.function2.params, [])
    self.assertEquals(self.function3.params[0].name, 'p1')
    self.assertEquals(self.function3.params[0].mutable, False)
    self.assertEquals(self.function4.params, [])

  def testResolveTypeReferences(self):
    self.function1.ResolveTypeReferences()
    self.assertEquals(self.function1.type_defn, None)
    self.assertEquals(self.function1.params[0].type_defn, self.type1)
    self.assertEquals(self.function1.params[1].type_defn, self.type2)
    self.function2.ResolveTypeReferences()
    self.assertEquals(self.function2.type_defn, self.type1)
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.function3.ResolveTypeReferences)
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.function4.ResolveTypeReferences)
    for ref in [self.f1_p1_ref, self.f1_p2_ref, self.f2_ret_ref,
                self.f3_p1_ref, self.f4_ret_ref]:
      self.assertEquals(ref.context, self.scope)
      self.assertEquals(ref.scoped, False)


class CallbackTest(unittest.TestCase):
  def setUp(self):
    self.type1 = MakeType('Type1')
    self.type2 = MakeType('Type2')
    self.c1_p1_ref = TypeReferenceMock(self.type1)
    self.c1_p2_ref = TypeReferenceMock(self.type2)
    self.callback1 = syntax_tree.Callback(_location, {}, 'Callback1', None,
                                          [(self.c1_p1_ref, 'p1'),
                                           (self.c1_p2_ref, 'p2')])
    self.c2_ret_ref = TypeReferenceMock(self.type1)
    self.callback2 = syntax_tree.Callback(_location, {}, 'Callback2',
                                          self.c2_ret_ref, [])
    self.c3_p1_ref = TypeReferenceMock(None)
    self.callback3 = syntax_tree.Callback(_location, {}, 'Callback3', None,
                                          [(self.c3_p1_ref, 'p1')])
    self.c4_ret_ref = TypeReferenceMock(None)
    self.callback4 = syntax_tree.Callback(_location,
                                          {'binding_model': 'test_bm'},
                                          'Callback4', self.c4_ret_ref, [])
    self.scope = ContextMock([self.type1, self.type2, self.callback1,
                              self.callback2, self.callback3, self.callback4],
                             [], 'globals', None)

  def testTypeScope(self):
    for c in [self.callback1, self.callback2, self.callback3, self.callback4]:
      self.assertTrue(c.is_type)
      self.assertFalse(c.is_scope)

  def testParams(self):
    self.assertEquals(self.callback1.params[0].name, 'p1')
    self.assertEquals(self.callback1.params[0].mutable, False)
    self.assertEquals(self.callback1.params[1].name, 'p2')
    self.assertEquals(self.callback1.params[1].mutable, False)
    self.assertEquals(self.callback2.params, [])
    self.assertEquals(self.callback3.params[0].name, 'p1')
    self.assertEquals(self.callback3.params[0].mutable, False)
    self.assertEquals(self.callback4.params, [])

  def testResolveTypeReferences(self):
    self.callback1.ResolveTypeReferences()
    self.assertEquals(self.callback1.type_defn, None)
    self.assertEquals(self.callback1.params[0].type_defn, self.type1)
    self.assertEquals(self.callback1.params[1].type_defn, self.type2)
    self.callback2.ResolveTypeReferences()
    self.assertEquals(self.callback2.type_defn, self.type1)
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.callback3.ResolveTypeReferences)
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.callback4.ResolveTypeReferences)
    for ref in [self.c1_p1_ref, self.c1_p2_ref, self.c2_ret_ref,
                self.c3_p1_ref, self.c4_ret_ref]:
      self.assertEquals(ref.context, self.scope)
      self.assertEquals(ref.scoped, False)

  def testLookUpBindingModel(self):
    self.assertEquals(self.callback1.LookUpBindingModel(), 'callback')
    self.assertEquals(self.callback2.LookUpBindingModel(), 'callback')
    self.assertEquals(self.callback3.LookUpBindingModel(), 'callback')
    self.assertEquals(self.callback4.LookUpBindingModel(), 'test_bm')


class VariableTest(unittest.TestCase):
  def setUp(self):
    self.type_defn = MakeType('Type')
    self.v1_type_ref = TypeReferenceMock(self.type_defn)
    self.v2_type_ref = TypeReferenceMock(None)
    self.variable1 = syntax_tree.Variable(_location, {}, 'Variable1',
                                          self.v1_type_ref)
    self.variable2 = syntax_tree.Variable(_location, {}, 'Variable2',
                                          self.v2_type_ref)
    self.scope = ContextMock([self.type_defn, self.variable1, self.variable2],
                             [], 'globals', None)

  def testTypeScope(self):
    for v in [self.variable1, self.variable2]:
      self.assertFalse(v.is_type)
      self.assertFalse(v.is_scope)

  def testResolveTypeReferences(self):
    self.variable1.ResolveTypeReferences()
    self.assertEquals(self.variable1.type_defn, self.type_defn)
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.variable2.ResolveTypeReferences)
    for ref in [self.v1_type_ref, self.v2_type_ref]:
      self.assertEquals(ref.context, self.scope)
      self.assertEquals(ref.scoped, False)


class TypedefTest(unittest.TestCase):
  def setUp(self):
    self.base_type1 = syntax_tree.Typename(_location, {'binding_model': 'bm1'},
                                           'BaseType1')
    self.base_type2_t = MakeType('Type')
    self.base_type2_s = MakeScope('Scope')
    self.base_type2 = syntax_tree.Class(_location, {'binding_model': 'bm2'},
                                        'BaseType2', None, [self.base_type2_t,
                                                            self.base_type2_s])
    self.t1_ref = TypeReferenceMock(self.base_type1)
    self.typedef1 = syntax_tree.Typedef(_location, {}, 'Typedef1', self.t1_ref)
    self.t2_ref = TypeReferenceMock(self.typedef1)
    self.typedef2 = syntax_tree.Typedef(_location, {}, 'Typedef2', self.t2_ref)
    self.t3_ref = TypeReferenceMock(None)
    self.typedef3 = syntax_tree.Typedef(_location, {}, 'Typedef3', self.t3_ref)
    self.t4_ref = TypeReferenceMock(self.base_type2)
    self.typedef4 = syntax_tree.Typedef(_location, {}, 'Typedef4', self.t4_ref)
    self.t5_ref = TypeReferenceMock(self.base_type2)
    self.typedef5 = syntax_tree.Typedef(_location, {'binding_model': 'bm3'},
                                        'Typedef5', self.t5_ref)
    self.scope = ContextMock([self.base_type1, self.base_type2, self.typedef1,
                              self.typedef2, self.typedef3, self.typedef4,
                              self.typedef5], [], 'globals', None)

  def testTypeScope(self):
    for t in [self.typedef1, self.typedef2, self.typedef3, self.typedef4,
              self.typedef5]:
      self.assertTrue(t.is_type)
      self.assertTrue(t.is_scope)

  def testResolveTypeReferences(self):
    self.typedef1.ResolveTypeReferences()
    self.assertEquals(self.typedef1.type_defn, self.base_type1)
    self.typedef2.ResolveTypeReferences()
    self.assertEquals(self.typedef2.type_defn, self.typedef1)
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.typedef3.ResolveTypeReferences)
    self.typedef4.ResolveTypeReferences()
    self.assertEquals(self.typedef4.type_defn, self.base_type2)
    self.typedef5.ResolveTypeReferences()
    self.assertEquals(self.typedef5.type_defn, self.base_type2)
    for typedef in [self.typedef1, self.typedef2, self.typedef3, self.typedef4,
                    self.typedef5]:
      self.assertTrue(typedef._types_resolved)
    for ref in [self.t1_ref, self.t2_ref, self.t3_ref, self.t4_ref,
                self.t5_ref]:
      self.assertEquals(ref.context, self.scope)
      self.assertEquals(ref.scoped, False)

  def testGetTypeSafe(self):
    self.assertEquals(self.typedef1.GetTypeSafe(), self.base_type1)
    self.assertEquals(self.typedef2.GetTypeSafe(), self.typedef1)
    self.assertRaises(syntax_tree.TypeNotFoundError, self.typedef3.GetTypeSafe)
    self.assertEquals(self.typedef4.GetTypeSafe(), self.base_type2)
    self.assertEquals(self.typedef5.GetTypeSafe(), self.base_type2)

  def testLookUpBindingModel(self):
    self.assertEquals(self.typedef1.LookUpBindingModel(), 'bm1')
    self.assertEquals(self.typedef2.LookUpBindingModel(), 'bm1')
    self.assertRaises(syntax_tree.TypeNotFoundError,
                      self.typedef3.LookUpBindingModel)
    self.assertEquals(self.typedef4.LookUpBindingModel(), 'bm2')
    self.assertEquals(self.typedef5.LookUpBindingModel(), 'bm3')

  def testGetFinalType(self):
    self.assertEquals(self.typedef1.GetFinalType(), self.base_type1)
    self.assertEquals(self.typedef2.GetFinalType(), self.base_type1)
    self.assertRaises(syntax_tree.TypeNotFoundError, self.typedef3.GetFinalType)
    self.assertEquals(self.typedef4.GetFinalType(), self.base_type2)
    self.assertEquals(self.typedef5.GetFinalType(), self.base_type2)

  def testLookUpType(self):
    for t in [self.typedef1, self.typedef2]:
      self.assertEquals(t.LookUpType('Type'), None)
      self.assertEquals(t.LookUpType('NonType'), None)
    self.assertRaises(syntax_tree.TypeNotFoundError, self.typedef3.LookUpType,
                      'Type')
    for t in [self.typedef4, self.typedef5]:
      self.assertEquals(t.LookUpType('Type'), self.base_type2_t)
      self.assertEquals(t.LookUpType('NonType'), None)

  def testFindScopes(self):
    for t in [self.typedef1, self.typedef2]:
      self.assertEquals(t.FindScopes('Scope'), [])
      self.assertEquals(t.FindScopes('NonScope'), [])
    self.assertRaises(syntax_tree.TypeNotFoundError, self.typedef3.FindScopes,
                      'Scope')
    for t in [self.typedef4, self.typedef5]:
      self.assertEquals(t.FindScopes('Scope'), [self.base_type2_s])
      self.assertEquals(t.FindScopes('NonScope'), [])


class TypenameTest(unittest.TestCase):
  def testTypename(self):
    typename = syntax_tree.Typename(_location, {'binding_model': 'bm'}, 'Type')
    self.assertTrue(typename.is_type)
    self.assertFalse(typename.is_scope)
    self.assertEquals(typename.LookUpBindingModel(), 'bm')


class VerbatimTest(unittest.TestCase):
  def testVerbatim(self):
    verbatim = syntax_tree.Verbatim(_location, {}, 'verbatim')
    self.assertFalse(verbatim.is_type)
    self.assertFalse(verbatim.is_scope)


class ArrayTest(unittest.TestCase):
  def testArray(self):
    data_type = syntax_tree.Typename(_location, {'binding_model': 'bm1'},
                                     'BaseType')
    unsized_array = syntax_tree.Array(data_type, None)
    sized_array = syntax_tree.Array(data_type, 42)
    for a in [unsized_array, sized_array]:
      self.assertTrue(a.is_type)
      self.assertFalse(a.is_scope)
      self.assertEquals(a.GetObjectsRecursive(), [a])
    self.assertEquals(sized_array.LookUpBindingModel(), 'sized_array')
    self.assertEquals(unsized_array.LookUpBindingModel(), 'unsized_array')


class CheckTypeInChainTest(unittest.TestCase):
  def testTypedef2(self):
    # typedef Typedef1 Typedef2;
    # typedef Typedef2 Typedef1;
    type1_ref = syntax_tree.NameTypeReference(_location, 'Typedef2')
    type1 = syntax_tree.Typedef(_location, {}, 'Typedef1', type1_ref)
    type2_ref = syntax_tree.NameTypeReference(_location, 'Typedef1')
    type2 = syntax_tree.Typedef(_location, {}, 'Typedef2', type2_ref)
    unused_scope1 = ContextMock([type1, type2], [], 'globals', None)
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type1)
    type1.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      syntax_tree.CheckTypeInChain, type2, type1)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      type2.ResolveTypeReferences)

  def testTypedef3(self):
    # typedef Typedef1 Typedef2;
    # typedef Typedef2 Typedef3;
    # typedef Typedef3 Typedef1;
    type1_ref = syntax_tree.NameTypeReference(_location, 'Typedef2')
    type1 = syntax_tree.Typedef(_location, {}, 'Typedef1', type1_ref)
    type2_ref = syntax_tree.NameTypeReference(_location, 'Typedef3')
    type2 = syntax_tree.Typedef(_location, {}, 'Typedef2', type2_ref)
    type3_ref = syntax_tree.NameTypeReference(_location, 'Typedef1')
    type3 = syntax_tree.Typedef(_location, {}, 'Typedef3', type3_ref)
    unused_scope1 = ContextMock([type1, type2, type3], [], 'globals', None)
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type3)
    syntax_tree.CheckTypeInChain(type3, type1)
    type1.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type3)
    syntax_tree.CheckTypeInChain(type3, type1)
    type2.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type3)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      syntax_tree.CheckTypeInChain, type3, type1)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      type3.ResolveTypeReferences)

  def testClass2(self):
    # class Class1: Class2 {};
    # class Class2: Class1 {};
    type1_ref = syntax_tree.NameTypeReference(_location, 'Class2')
    type1 = syntax_tree.Class(_location, {}, 'Class1', type1_ref, [])
    type2_ref = syntax_tree.NameTypeReference(_location, 'Class1')
    type2 = syntax_tree.Class(_location, {}, 'Class2', type2_ref, [])
    unused_scope1 = ContextMock([type1, type2], [], 'globals', None)
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type1)
    type1.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      syntax_tree.CheckTypeInChain, type2, type1)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      type2.ResolveTypeReferences)

  def testClass3(self):
    # class Class1: Class2 {};
    # class Class2: Class3 {};
    # class Class3: Class1 {};
    type1_ref = syntax_tree.NameTypeReference(_location, 'Class2')
    type1 = syntax_tree.Class(_location, {}, 'Class1', type1_ref, [])
    type2_ref = syntax_tree.NameTypeReference(_location, 'Class3')
    type2 = syntax_tree.Class(_location, {}, 'Class2', type2_ref, [])
    type3_ref = syntax_tree.NameTypeReference(_location, 'Class1')
    type3 = syntax_tree.Class(_location, {}, 'Class3', type3_ref, [])
    unused_scope1 = ContextMock([type1, type2, type3], [], 'globals', None)
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type3)
    syntax_tree.CheckTypeInChain(type3, type1)
    type1.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type3)
    syntax_tree.CheckTypeInChain(type3, type1)
    type2.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type3)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      syntax_tree.CheckTypeInChain, type3, type1)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      type3.ResolveTypeReferences)

  def testArray(self):
    # typedef Typedef1[] Typedef1;
    type1_ref_ref = syntax_tree.NameTypeReference(_location, 'Typedef1')
    type1_ref = syntax_tree.ArrayTypeReference(_location, type1_ref_ref, None)
    type1 = syntax_tree.Typedef(_location, {}, 'Typedef1', type1_ref)
    unused_scope1 = ContextMock([type1], [], 'globals', None)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      type1.ResolveTypeReferences)

  def testMixed(self):
    # typedef Class Typedef;
    # class Class: Typedef {};
    type1_ref = syntax_tree.NameTypeReference(_location, 'Class')
    type1 = syntax_tree.Typedef(_location, {}, 'Typedef', type1_ref)
    type2_ref = syntax_tree.NameTypeReference(_location, 'Typedef')
    type2 = syntax_tree.Class(_location, {}, 'Class', type2_ref, [])
    unused_scope1 = ContextMock([type1, type2], [], 'globals', None)
    syntax_tree.CheckTypeInChain(type1, type2)
    syntax_tree.CheckTypeInChain(type2, type1)
    type1.ResolveTypeReferences()
    syntax_tree.CheckTypeInChain(type1, type2)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      syntax_tree.CheckTypeInChain, type2, type1)
    self.assertRaises(syntax_tree.CircularTypedefError,
                      type2.ResolveTypeReferences)


class LookUpScopeTest(unittest.TestCase):
  def testLookUpScope(self):
    type1 = MakeType('Type1')
    type2 = MakeType('Type2')
    scope1 = MakeScope('Scope1')
    scope2 = MakeScope('Scope2')
    scope = syntax_tree.LookUpScope([type1, type2, scope1, scope2])
    self.assertEquals(scope.LookUpType('Type1'), type1)
    self.assertEquals(scope.LookUpType('Type2'), type2)
    self.assertEquals(scope.LookUpType('Type3'), None)
    self.assertEquals(scope.LookUpType('Scope1'), None)
    self.assertEquals(scope.LookUpType('Scope2'), None)
    self.assertEquals(scope.LookUpType('Scope3'), None)
    self.assertEquals(scope.FindScopes('Type1'), [])
    self.assertEquals(scope.FindScopes('Type2'), [])
    self.assertEquals(scope.FindScopes('Type3'), [])
    self.assertEquals(scope.FindScopes('Scope1'), [scope1])
    self.assertEquals(scope.FindScopes('Scope2'), [scope2])
    self.assertEquals(scope.FindScopes('Scope3'), [])
    type3 = MakeType('Type3')
    scope3 = MakeScope('Scope3')
    scope3_bis = MakeScope('Scope3')
    scope.list.extend([type3, scope3, scope3_bis])
    scope.ResetCache()
    self.assertEquals(scope.LookUpType('Type1'), type1)
    self.assertEquals(scope.LookUpType('Type2'), type2)
    self.assertEquals(scope.LookUpType('Type3'), type3)
    self.assertEquals(scope.LookUpType('Scope1'), None)
    self.assertEquals(scope.LookUpType('Scope2'), None)
    self.assertEquals(scope.LookUpType('Scope3'), None)
    self.assertEquals(scope.FindScopes('Type1'), [])
    self.assertEquals(scope.FindScopes('Type2'), [])
    self.assertEquals(scope.FindScopes('Type3'), [])
    self.assertEquals(scope.FindScopes('Scope1'), [scope1])
    self.assertEquals(scope.FindScopes('Scope2'), [scope2])
    self.assertEquals(scope.FindScopes('Scope3'), [scope3, scope3_bis])


class GetObjectsRecursiveTest(unittest.TestCase):
  def testGetObjectsRecursive(self):
    class1 = syntax_tree.Class(_location, {}, 'Class1', None,
                               [MakeType('Type1'), MakeType('Type2'),
                                MakeScope('Scope1'), MakeScope('Scope2')])
    class2 = syntax_tree.Class(_location, {}, 'Class1', None,
                               [MakeType('Type1'), MakeType('Type3'),
                                MakeScope('Scope2'), MakeScope('Scope3')])
    type1 = MakeType('Type1')
    scope1 = MakeScope('Scope1')
    self.assertEquals(syntax_tree.GetObjectsRecursive([class1, class2, type1,
                                                        scope1]),
                      class1.GetObjectsRecursive() +
                      class2.GetObjectsRecursive() +
                      type1.GetObjectsRecursive() +
                      scope1.GetObjectsRecursive())


class MergeNamespacesRecursive(unittest.TestCase):
  def testMergeNamespaceResucrsive(self):
    def MakeNamespace(name, defn_list):
      return syntax_tree.Namespace(_location, {}, name, defn_list)

    ns1_1 = MakeNamespace('Namespace1', [MakeType('Type1_1'),
                                         MakeScope('Scope1_1')])
    ns1_2 = MakeNamespace('Namespace1', [MakeType('Type1_2'),
                                         MakeScope('Scope1_2')])
    ns2_1 = MakeNamespace('Namespace2', [ns1_1, ns1_2])
    ns1_3 = MakeNamespace('Namespace1', [MakeType('Type1_3'),
                                         MakeScope('Scope1_3')])
    ns1_4 = MakeNamespace('Namespace1', [MakeType('Type1_4'),
                                         MakeScope('Scope1_4')])
    ns2_2 = MakeNamespace('Namespace2', [ns1_3, ns1_4])
    ns = MakeNamespace('global', [ns2_1, ns2_2])
    syntax_tree.MergeNamespacesRecursive(ns)
    self.assertEquals(ns2_1.scope, ns2_2.scope)
    self.assertNotEquals(ns2_1.defn_list, ns2_2.defn_list)
    self.assertEquals(ns1_1.scope, ns1_2.scope)
    self.assertEquals(ns1_1.scope, ns1_3.scope)
    self.assertEquals(ns1_1.scope, ns1_4.scope)
    self.assertNotEquals(ns1_1.defn_list, ns1_2.defn_list)
    self.assertNotEquals(ns1_1.defn_list, ns1_3.defn_list)
    self.assertNotEquals(ns1_1.defn_list, ns1_4.defn_list)
    self.assertNotEquals(ns1_2.defn_list, ns1_3.defn_list)
    self.assertNotEquals(ns1_2.defn_list, ns1_4.defn_list)
    self.assertNotEquals(ns1_3.defn_list, ns1_4.defn_list)


if __name__ == '__main__':
  unittest.main()
