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

"""Classes describing the parsed IDL as a syntax tree.

This module contains all the classes that are used to describe the IDL as a
syntax tree.
"""

# TODO: this module has grown too big, it should be split.


class Error(Exception):
  """Base exception for the syntax_tree module."""


class TypeNotFoundError(Error):
  """Raised when a type reference doesn't resolve to a definition."""

  def __init__(self, type_defn, location=None):
    Error.__init__(self)
    self.type_defn = type_defn
    self.location = location

  def __str__(self):
    if self.location == None:
      return self.type_defn
    else:
      return '%s at %s' % (self.type_defn, self.location)

class DerivingFromNonClassError(Error):
  """Raised when a class derives from a non-class."""

  def __init__(self, type_defn, base_type, location=None):
    Error.__init__(self)
    self.type_defn = type_defn
    self.base_type = base_type
    self.location = location


class ArrayOfNonTypeError(Error):
  """Raised when trying to reference an array of a non-type definition."""

  def __init__(self, data_defn, size):
    Error.__init__(self)
    self.type_defn = data_defn
    self.size = size


class UnknownBindingModelError(Error):
  """Raised when a binding model cannot be found."""

  def __init__(self, name):
    Error.__init__(self)
    self.name = name

  def __str__(self):
    return self.name

class CircularTypedefError(Error):
  """Raised when 2 typedefs reference each other (even indirectly)."""

  def __init__(self, type1, type2):
    Error.__init__(self)
    self.type1 = type1
    self.type2 = type2

  def __str__(self):
    return '%s and %s' % (self.type1, self.type2)


class UnimplementedMethodError(Error):
  """Raised when an "abstract" method is not implemented in a derived type."""


class TypeReference(object):
  """Base class for type references.

  TypeReference objects are constructed while parsing the IDL file and provide
  a deferred lookup mechanism, removing the necessity to forward-declare all
  the types before using them.
  """

  def __init__(self, location):
    """Inits a TypeReference instance.

    Args:
      location: idl_parser.SourceLocation object, describing the source
        location of the reference in the IDL.
    """
    self.location = location

  def GetType(self, context):
    """Look-up the referenced type.

    Args:
      context: context in which to look-up named items (scopes, types). This
        must be a Definition instance.

    Returns:
      The type, as a Definition instance.

    Raises:
      TypeNotFoundError: the type wasn't found.
    """
    type_defn = self.GetTypeInternal(context, False)
    if type_defn:
      return type_defn
    else:
      raise TypeNotFoundError(self, self.location)

  def GetTypeInternal(self, context, scoped):
    """Implements type look-up.

    This method should be implemented in derived classes to perform the
    reference-specific look-up.

    Args:
      context: context in which to look-up named items (scopes, types). This
        must be a Definition instance.
      scoped: boolean deciding whether or not the look-up is scoped by the
        context, or if parent context should be looked-up as well.

    Returns:
      The type, if found, or None if not.
    """
    context, scoped = context, scoped  # silence lint
    return None


class NameTypeReference(TypeReference):
  """Class representing a type reference by name.

  This class represents the reference to a type by name. Look-up will produce a
  type that has the specified name, within the specified look-up context - if
  it exists.

  Produced by the IDL construct 'Type' where Type is the name of a type
  (typedef, typename, class, enum).
  """

  def __init__(self, location, name):
    """Inits a NameTypeReference instance.

    Args:
      location: idl_parser.SourceLocation object, describing the source
        location of the reference in the IDL.
      name: the name of the type to look for.
    """
    TypeReference.__init__(self, location)
    self.name = name

  def GetTypeInternal(self, context, scoped):
    """Implementation of the type look-up for NameTypeReference."""
    if scoped:
      type_defn = context.LookUpType(self.name)
    else:
      type_defn = context.LookUpTypeRecursive(self.name)
    return type_defn or None

  def __str__(self):
    return self.name


class ScopedTypeReference(TypeReference):
  """Class representing a scoped type reference.

  This class represents the reference to a type that is scoped by a particular
  name. Look-up will produce a type that can be referenced inside that
  particular scope.

  Produced by the IDL construct 'Scope::TypeRef' where Scope is the name of a
  scope, and TypeRef is a type reference.
  """

  def __init__(self, location, scope_name, type_ref):
    """Inits a ScopedTypeReference instance.

    Args:
      location: idl_parser.SourceLocation object, describing the source
        location of the reference in the IDL.
      scope_name: the name of the scope to look inside.
      type_ref: the type reference to look for, inside the named scope.
    """
    TypeReference.__init__(self, location)
    self.scope_name = scope_name
    self.type_ref = type_ref

  def GetTypeInternal(self, context, scoped):
    """Implementation of the type look-up for ScopedTypeReference."""
    if scoped:
      scope_list = context.FindScopes(self.scope_name)
    else:
      scope_list = context.FindScopesRecursive(self.scope_name)

    for scope in scope_list:
      type_defn = self.type_ref.GetTypeInternal(scope, True)
      if type_defn:
        return type_defn
    return None

  def __str__(self):
    return '%s::%s' % (self.scope_name, self.type_ref)


class ArrayTypeReference(TypeReference):
  """Class representing an array type reference.

  This class represents the reference to a type that is an array of a referenced
  data type.
  The array can be either sized or unsized.

  Produced by the IDL constructs 'TypeRef[]' or 'TypeRef[size]' where TypeRef
  is a type reference.
  """

  def __init__(self, location, type_ref, size):
    """Inits a ArrayTypeReference instance.

    Args:
      location: idl_parser.SourceLocation object, describing the source
        location of the reference in the IDL.
      type_ref: the type reference for the data type.
      size: the size of the array, or None if unsized.
    """
    TypeReference.__init__(self, location)
    self.type_ref = type_ref
    self.size = size

  def GetTypeInternal(self, context, scoped):
    """Implementation of the type look-up for ArrayTypeReference."""
    type_defn = self.type_ref.GetTypeInternal(context, scoped)
    if type_defn:
      return type_defn.GetArrayType(self.size)
    else:
      return None

  def __str__(self):
    if self.size == None:
      return '%s[]' % self.type_ref
    else:
      return '%s[%s]' % (self.type_ref, self.size)


class QualifiedTypeReference(TypeReference):
  """Class representing a type reference with a qualifier.

  This class represents the reference to a type that is specialized by a type
  qualifier, such as const or volatile or nullable.

  Produced by the IDL construct 'qualifier TypeRef' where qualifier is a type
  qualifier (const, restrict, volatile), and TypeRef is a type reference.
  """

  def __init__(self, location, qualifier, type_ref):
    """Inits a QualifiedTypeReference instance.

    Args:
      location: idl_parser.SourceLocation object, describing the source
        location of the reference in the IDL.
      qualifier: the qualifier.
      type_ref: the type reference for the un-qualified type.
    """
    TypeReference.__init__(self, location)
    self.qualifier = qualifier
    self.type_ref = type_ref

  def GetTypeInternal(self, context, scoped):
    """Implementation of the type look-up for QualifiedTypeReference."""
    type_defn = self.type_ref.GetTypeInternal(context, scoped)
    if type_defn:
      if self.qualifier == 'nullable':
        return type_defn.GetNullableType()
      else:
        return type_defn
    else:
      return None

  def __str__(self):
    return '%s %s' % (self.qualifier, self.type_ref)


class Definition(object):
  """Base class for definitions.

  Definition objects represent all the items that are defined in the IDL, such
  as classes or functions. This class provides some common functionality, such
  as walking the syntax tree to look for types, and provide an interface that
  should be implemented by sub-classes, such as listing all the scopes the
  object may contain.

  Attributes:
    defn_type: a string representing the type of this definition
    source: idl_parser.SourceLocation object, describing the source location
        of this definition
    attributes: a dictionary of attributes associated with that definition
    name: (optional) the name of the definition. None if undefined
    is_type: this definition is a type, and should be found by LookUpType. e.g.
      Class, Typedef, Enum, ...
    is_scope: this definition is a scope, and should be found by FindScopes.
      e.g. Class, Namespace, ...
    parent: the parent scope of that definition
    array_defns: a dictionary of array definitions of this type, hashed by the
      array size. Empty for non-types
    nullable: the nullable form of this type.
    binding_model: the binding model module for this type. None for non-types
  """
  defn_type = 'Definition'

  def __init__(self, source, attributes, name):
    """Inits a Definition instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this definition
      attributes: a dictionary of attributes associated with that definition
      name: (optional) the name of the definition
    """
    self.source = source
    self.attributes = attributes
    self.name = name
    self.is_type = False
    self.is_scope = False
    self.parent = None
    self.array_defns = {}
    self.nullable = None
    self.binding_model = None

  def __repr__(self):
    return '%s(%s)' % (self.defn_type, self.name)

  def GetParentScopeStack(self):
    """Gets the stack of englobing scopes."""
    stack = []
    cursor = self.parent
    while cursor:
      stack = [cursor] + stack
      cursor = cursor.parent
    return stack

  def LookUpTypeRecursive(self, name):
    """Looks up a type by name, recursively.

    This method looks for a type by name, in the current definition and all
    the parent scopes recursively. The first type found will be returned. See
    LookUpType.

    Args:
      name: the name of the type looked for.

    Returns:
      The type that was found, or None if no type was found.
    """
    lookup_context = self
    while lookup_context:
      type_defn = lookup_context.LookUpType(name)
      if type_defn:
        return type_defn
      lookup_context = lookup_context.parent
    return None

  def FindScopesRecursive(self, name):
    """Finds all scopes matching a name, recursively.

    This method finds all scopes matching a name, in the current definition
    and recursively in all the parent scopes. See FindScopes.

    Args:
      name: the name of the scope looked for.

    Returns:
      The list of all the scopes that were found, in traversal order. May be []
      if no scope of that name was found.
    """
    scopes = []
    lookup_context = self
    while lookup_context:
      scopes.extend(lookup_context.FindScopes(name))
      lookup_context = lookup_context.parent
    return scopes

  def GetDefinitionInclude(self):
    """Returns the include file that has the C++ definition of this type."""
    if 'include' in self.attributes:
      return self.attributes['include']
    else:
      return self.source.file.header

  def SetBindingModel(self, binding_models):
    """Assigns the binding model module to this description.

    This method assigns the binding model module to this description, and all
    its array types. Binding models are usually defined as the 'binding_model'
    attribute, but some types may inherit binding models from some other types
    (e.g.  Classes inherit the binding model from their base class, and
    Typedefs inherit the binding model from their type). See
    LookUpBindingModel.

    Args:
      binding_models: a dictionary mapping binding model names to binding model
      modules

    Raises:
      UnknownBindingModelError if this description doesn't have a valid binding
      model.
    """
    for k in self.array_defns:
      self.array_defns[k].SetBindingModel(binding_models)
    if self.nullable and self.nullable != self:
      self.nullable.SetBindingModel(binding_models)
    name = self.LookUpBindingModel()
    try:
      self.binding_model = binding_models[name]
    except KeyError:
      raise UnknownBindingModelError(name)

  def GetArrayType(self, size):
    """Returns a Definition representing an array of this type.

    This method returns a Description representing an array of this type.
    Arrays of the same data type and same size are shared.

    Args:
      size: the size of the array, or None for an unsized array.

    Returns:
      The array type description.

    Raises:
      ArrayOfNonTypeError: this description is not a type.
    """
    if not self.is_type:
      raise ArrayOfNonTypeError(self, size)
    # An array is not a 'definition' per say, it is a construct that is
    # instanciated on use. For several purposes (looking up binding models for
    # example), we want to treat them as a definition, so hook a unique version
    # of each array (unsized, each size) in the type.
    if size in self.array_defns:
      return self.array_defns[size]
    else:
      array = Array(self, size)
      self.array_defns[size] = array
      return array

  def GetNullableType(self):
    """Returns a Definition representing a nullable version of this type."""
    if not self.nullable:
      self.nullable = Nullable(self)
    return self.nullable

  # members to be overridden by sub-types
  def LookUpBindingModel(self):
    """Looks up the binding model name for this description.

    This method should be overridden by Description sub-types that are types.

    Returns:
      The binding model name, or None if not found.

    Raises:
      UnimplementedMethodError: the method was not overridden in the sub-type.
    """
    raise UnimplementedMethodError

  def GetObjectsRecursive(self):
    """Gets the list of all objects defined in this description, recursively.

    This method gets the list of all objects defined within this description,
    going recursively through them. In this list, parent objects must be
    returned before their children. This method should be overridden by
    Description sub-types that are scopes, but the default behavior is valid
    for non-scopes.

    Returns:
      The list of objects.
    """
    return [self]

  def ResolveTypeReferences(self):
    """Resolve all type references needed for this type.

    This method should be overridden by Description sub-types that have type
    references (like Functions or Classes).

    Raises:
      TypeNotFoundError: a type reference could not be resolved.
      CircularTypedefError: circular type references were found.
      DerivingFromNonClassError: a class definition derives from a non-class
        definition.
    """
    pass

  def LookUpType(self, name):
    """Looks up a type by name, in the current definition.

    This method looks up a type by name, in the current definition. The first
    type found will be returned. Since this method may be used while resolving
    type references, it may raise the same exceptions as ResolveTypeReferences.
    This method should be overridden by Description sub-types that are scopes,
    but the default behavior is correct for non-scopes.

    Args:
      name: the name of the type looked for.

    Returns:
      The type that was found, or None if no type was found.

    Raises:
      TypeNotFoundError: a type reference could not be resolved.
      CircularTypedefError: circular type references were found.
      DerivingFromNonClassError: a class definition derives from a non-class
        definition.
    """
    name = name  # silence lint
    return None

  def FindScopes(self, name):
    """Finds all scopes matching a name, in the current definition.

    This method finds all scopes matching a name, in the current definition.
    Since this method may be used while resolving type references, it may raise
    the same exceptions as ResolveTypeReferences. This method should be
    overridden by Description sub-types that are scopes, but the default
    behavior is correct for non-scopes.

    Args:
      name: the name of the scope looked for.

    Returns:
      The list of all the scopes that were found, in traversal order. May be []
      if no scope of that name was found.
    """
    name = name  # silence lint
    return []

  def GetFinalType(self):
    """Returns the final type of this description, following Typedefs links.

    This method should be overridden by Description sub-types that are types,
    and represent links to other types (like Typedefs).

    Returns:
      The final type.
    """
    return self


class Class(Definition):
  """Class definition.

  Class objects represent classes as defined in the IDL by the 'class Name {};'
  constructs. A Class is a type and a scope, it can contain definitions within
  itself, such as member fields and methods, or other types. Only single
  inheritance is supported.

  Attributes:
    base_type_ref: the reference to the base type of the class.
    base_type: the base type of the class, once the reference has been
      resolved.
    defn_list: the list of definitions contained in the class scope.
  """
  defn_type = 'Class'

  def __init__(self, source, attributes, name, base_type_ref, defn_list):
    """Inits a Class instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this class.
      attributes: a dictionary of attributes associated with that class.
      name: the name of the class.
      base_type_ref: a TypeReference, referencing the base class if any, or
        None if this class doesn't derive from another class.
      defn_list: the list of definitions within the class scope.
    """
    Definition.__init__(self, source, attributes, name)
    self.base_type_ref = base_type_ref
    self.base_type = None
    self.defn_list = defn_list
    self._scope = LookUpScope(defn_list)
    self.is_type = True
    self.is_scope = True
    self._types_resolved = False
    for o in defn_list:
      o.parent = self

  def GetObjectsRecursive(self):
    """Implementation of GetObjectsRecursive for Class."""
    return [self] + GetObjectsRecursive(self.defn_list)

  def ResolveTypeReferences(self):
    """Implementation of ResolveTypeReferences for Class.

    This method will resolve the base class reference.

    Raises:
      DerivingFromNonClassError: a class definition derives from a non-class
        definition.
    """
    if self._types_resolved:
      return
    self._types_resolved = True
    if self.base_type_ref is None:
      self.base_type = None
    else:
      base_type = self.base_type_ref.GetType(self.parent)
      CheckTypeInChain(self, base_type)
      self.base_type = base_type
      if self.base_type.GetFinalType().defn_type != 'Class':
        raise DerivingFromNonClassError(self, self.base_type, self.source)

  def GetBaseSafe(self):
    """Gets the base type safely, by making sure it is resolved if needed.

    Returns:
      The base type.

    Raises:
      TypeNotFoundError: a type reference could not be resolved.
      CircularTypedefError: circular type references were found.
      DerivingFromNonClassError: a class definition derives from a non-class
        definition.
    """
    self.ResolveTypeReferences()
    return self.base_type

  def LookUpType(self, name):
    """Implementation of LookUpType for Class."""
    type_defn = self._scope.LookUpType(name)
    if type_defn:
      return type_defn
    base = self.GetBaseSafe()
    if base:
      return base.LookUpType(name)
    return None

  def FindScopes(self, name):
    """Implementation of FindScopes for Class."""
    scopes = self._scope.FindScopes(name)
    base = self.GetBaseSafe()
    if base:
      scopes.extend(base.FindScopes(name))
    return scopes

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Class."""
    base = self.GetBaseSafe()
    if 'binding_model' in self.attributes:
      return self.attributes['binding_model']
    elif base:
      return base.LookUpBindingModel()
    else:
      return None


class Namespace(Definition):
  """Namespace definition.

  Namespace objects represent namespaces as defined in the IDL by the
  'namespace Name {}' constructs. A Namespace is a scope, it can contain
  definitions within itself, such as member fields and methods, or other types.
  Namespaces can be defined by parts, namespaces that have the same name, and
  are defined within the same outer scope are semantically merged for type and
  scope lookup purposes. See the MergeNamespacesRecursive function.

  Attributes:
    defn_list: the list of definitions contained in the namespace scope.
    scope: a LookUpScope object, that will be shared across namespaces that
      should be merged for lookup.
  """
  defn_type = 'Namespace'

  def __init__(self, source, attributes, name, defn_list):
    """Inits a Namespace instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this namespace.
      attributes: a dictionary of attributes associated with that namespace.
      name: the name of the namespace.
      defn_list: the list of definitions within the namespace scope.
    """
    Definition.__init__(self, source, attributes, name)
    self.defn_list = defn_list
    self.scope = LookUpScope(defn_list)
    self.is_scope = True
    for o in defn_list:
      o.parent = self

  def GetObjectsRecursive(self):
    """Implementation of GetObjectsRecursive for Namespace."""
    return [self] + GetObjectsRecursive(self.defn_list)

  def MergeLookUpScope(self, other):
    """Merges the LookUpScope object from another Namespace into this one.

    See the MergeNamespacesRecursive function.

    Args:
      other: the other Namespace to merge into this one.
    """
    self.scope.ResetCache()
    # self.scope.list may be a reference to the namespace defn_list, so create
    # a copy before modifying the list.
    self.scope.list = self.scope.list[:]
    self.scope.list.extend(other.scope.list)
    other.scope = self.scope

  def LookUpType(self, name):
    """Implementation of LookUpType for Namespace."""
    return self.scope.LookUpType(name)

  def FindScopes(self, name):
    """Implementation of FindScopes for Namespace."""
    return self.scope.FindScopes(name)


class Enum(Definition):
  """Enum definition.

  Enum objects represent enums as defined in the IDL by the
  'enum Name {VALUE1, VALUE2};' constructs. An Enum is a type.

  Attributes:
    values: the list of values defined in the enum, as Enum.Value objects.
  """

  class Value(object):
    """Enum value description.

    This is simple structure, with a name and an optional value.

    Attributes:
      name: the name of the value.
      value: the value, or None for the default value.
    """

    def __init__(self, name, value):
      """Inits an Enum.Value instance.

      Args:
        name: the name of the value.
        value: the value, or None for the default value.
      """
      self.name = name
      self.value = value

  defn_type = 'Enum'

  def __init__(self, source, attributes, name, values):
    """Inits an Enum instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this enum.
      attributes: a dictionary of attributes associated with that enum.
      name: the name of the enum.
      values: the list of values defined for this enum, as Enum.Value objects.
    """
    Definition.__init__(self, source, attributes, name)
    self.values = values
    self.is_type = True

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Enum."""
    return 'enum'


class Function(Definition):
  """Function definition.

  Function objects represent functions, methods and constructors/destructors, as
  defined in the IDL by the 'Type Function(Type param1, Type param2);'
  constructs.

  Attributes:
    type_ref: the reference to the return type of the function, or None for
      constructors/destructors.
    type: the return type of the function, once the reference has been
      resolved, or None for constructors/destructors.
    params: the list of the function parameters, as Function.Param objects.
  """

  class Param(object):
    """Function parameter description.

    This represents any of the function parameter, with its name and type.

    Attributes:
      type_ref: the reference to the type of the parameter.
      type: the type of the parameter, once the reference has been resolved.
      name: the name of the parameter.
      mutable: a boolean indicating whether the parameter is mutable. While
        (currently) there is no mechanism to specify it in the IDL, code
        generation internals use this flag to generate functions with a mutable
        'this' parameter for methods.
    """

    def __init__(self, type_ref, name):
      """Inits a Function.Param instance.

      Args:
        type_ref: the reference to the type of the parameter.
        name: the name of the parameter.
      """
      self.type_ref = type_ref
      self.type_defn = None
      self.name = name
      self.mutable = False

  defn_type = 'Function'

  def __init__(self, source, attributes, name, type_ref, params):
    """Inits a Function instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this function.
      attributes: a dictionary of attributes associated with that function.
      name: the name of the function.
      type_ref: a TypeReference, referencing the return type if any, or
        None for constructors/destructors.
      params: the list of parameters, as (TypeReference, name) pairs.
    """
    Definition.__init__(self, source, attributes, name)
    self.type_ref = type_ref
    self.type_defn = None
    self.params = [self.Param(type_ref, name) for type_ref, name in params]

  def ResolveTypeReferences(self):
    """Implementation of ResolveTypeReferences for Function."""
    if self.type_ref is None:
      self.type_defn = None
    else:
      self.type_defn = self.type_ref.GetType(self.parent)
    for param in self.params:
      param.type_defn = param.type_ref.GetType(self.parent)


class Callback(Definition):
  """Callback definition.

  Callback objects represent callback functions as defined in the IDL by the
  'callback Type Function(Type param1, Type param2);' constructs. Callbacks are
  meant to behave like function objects. Callbacks are types.

  Attributes:
    type_ref: the reference to the return type of the callback.
    type: the return type of the callback, once the reference has been
      resolved.
    params: the list of the function parameters, as Function.Param objects.
  """

  defn_type = 'Callback'

  def __init__(self, source, attributes, name, type_ref, params):
    """Inits a Callback instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this callback.
      attributes: a dictionary of attributes associated with that callback.
      name: the name of the callback.
      type_ref: a TypeReference, referencing the return type if any, or
        None for constructors/destructors.
      params: the list of parameters, as (TypeReference, name) pairs.
    """
    Definition.__init__(self, source, attributes, name)
    self.type_ref = type_ref
    self.type_defn = None
    self.params = [Function.Param(type_ref, name) for type_ref, name in params]
    self.is_type = True

  def ResolveTypeReferences(self):
    """Implementation of ResolveTypeReferences for Callback."""
    if self.type_ref is None:
      self.type_defn = None
    else:
      self.type_defn = self.type_ref.GetType(self.parent)
    for param in self.params:
      param.type_defn = param.type_ref.GetType(self.parent)

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Callback."""
    if 'binding_model' in self.attributes:
      return self.attributes['binding_model']
    else:
      return 'callback'


class Variable(Definition):
  """Variable definition.

  Variable objects represent variables, whether global, or class member fields,
  as defined in the IDL by the 'Type variable;' constructs.

  Attributes:
    type_ref: the reference to the type of the variable
    type: the type of the variable, once the reference has been resolved.
  """
  defn_type = 'Variable'

  def __init__(self, source, attributes, name, type_ref):
    """Inits a Variable instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this variable.
      attributes: a dictionary of attributes associated with that variable.
      name: the name of the variable.
      type_ref: a TypeReference, referencing the type of the variable.
    """
    Definition.__init__(self, source, attributes, name)
    self.type_ref = type_ref
    self.type_defn = None

  def ResolveTypeReferences(self):
    """Implementation of ResolveTypeReferences for Variable."""
    self.type_defn = self.type_ref.GetType(self.parent)

  def MakeGetter(self, attributes, name):
    """Creates a Function for the getter for the variable.

    Args:
      attributes: the attributes for the getter function.
      name: the name of the getter function.

    Returns:
      the getter definition.
    """
    func = Function(self.source, attributes, name, self.type_ref, [])
    func.parent = self.parent
    func.ResolveTypeReferences()
    return func

  def MakeSetter(self, attributes, name):
    """Creates a Function for the setter for the variable.

    Args:
      attributes: the attributes for the setter function.
      name: the name of the setter function.

    Returns:
      the setter definition.
    """
    void_ref = NameTypeReference(self.source, 'void')
    func = Function(self.source, attributes, name, void_ref, [(self.type_ref,
                                                               self.name)])
    func.parent = self.parent
    func.ResolveTypeReferences()
    return func


class Typedef(Definition):
  """Typedef definition.

  Typedef objects represent type 'symbolic links', defining a new name for an
  existing type, as defined in the IDL by the 'typedef Type NewType;'
  constructs. Typedefs are types, and scopes.

  Attributes:
    type_ref: the reference to the original type of the typedef
    type: the original type of the typedef, once the reference has been
      resolved.
  """
  defn_type = 'Typedef'

  def __init__(self, source, attributes, name, type_ref):
    """Inits a Typedef instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this typedef.
      attributes: a dictionary of attributes associated with that typedef.
      name: the name of the typedef.
      type_ref: a TypeReference, referencing the original type of the typedef.
    """
    Definition.__init__(self, source, attributes, name)
    self.type_ref = type_ref
    self.type_defn = None
    self.is_type = True
    self.is_scope = True
    self._types_resolved = False

  def ResolveTypeReferences(self):
    """Implementation of ResolveTypeReferences for Typedef."""
    if self._types_resolved:
      return
    self._types_resolved = True
    type_defn = self.type_ref.GetType(self.parent)
    CheckTypeInChain(self, type_defn)
    self.type_defn = type_defn

  def GetTypeSafe(self):
    """Gets the original type safely, by making sure it is resolved if needed.

    Returns:
      The original type.

    Raises:
      TypeNotFoundError: a type reference could not be resolved.
      CircularTypedefError: circular type references were found.
      DerivingFromNonClassError: a class definition derives from a non-class
        definition.
    """
    self.ResolveTypeReferences()
    return self.type_defn

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Typedef."""
    if 'binding_model' in self.attributes:
      return self.attributes['binding_model']
    else:
      return self.GetTypeSafe().LookUpBindingModel()

  def GetFinalType(self):
    """Implementation of GetFinalType for Typedef."""
    return self.GetTypeSafe().GetFinalType()

  def LookUpType(self, name):
    """Implementation of LookUpType for Typedef."""
    return self.GetTypeSafe().LookUpType(name)

  def FindScopes(self, name):
    """Implementation of FindScopes for Typedef."""
    return self.GetTypeSafe().FindScopes(name)


class Typename(Definition):
  """Typename definition.

  Typename objects represent unknown types, whose definition cannot be
  expressed in IDL, but still need to be referenced in the IDL.  They are
  declared in IDL with the 'typename Type;' constructs. Typenames are types.

  Attributes:
    None
  """
  defn_type = 'Typename'

  def __init__(self, source, attributes, name):
    """Inits a Typename instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this typename.
      attributes: a dictionary of attributes associated with that typename.
      name: the name of the typename.
    """
    Definition.__init__(self, source, attributes, name)
    self.is_type = True

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Typename."""
    try:
      return self.attributes['binding_model']
    except KeyError:
      return None


class Verbatim(Definition):
  """Verbatim definition.

  Verbatim objects represent text that needs to be included as is into the
  generated code. Attributes control in which context the code should be
  included. They are defined in IDL within '%{' and '%}' quotes.

  Attributes:
    text: the verbatim text.
  """
  defn_type = 'Verbatim'

  def __init__(self, source, attributes, text):
    """Inits a Verbatim instance.

    Args:
      source: idl_parser.SourceLocation object, describing the source location
        of this verbatim.
      attributes: a dictionary of attributes associated with that verbatim.
      text: the text of the verbatim.
    """
    Definition.__init__(self, source, attributes, None)
    self.text = text


class Array(Definition):
  """Array definition.

  Array objects represent sized and unsized arrays of a data type. They are not
  defined explicitly in IDL, but implicitly through array references. Arrays
  are types.

  Attributes:
    data_type: the type of individual elements of the array
    size: the size of the array, as an integer, or None for an unsized array
  """
  defn_type = 'Array'

  def __init__(self, data_type, size):
    """Inits an Array instance.

    Args:
      data_type: the type of individual elements of the array
      size: the size of the array, as an integer, or None for an unsized array
    """
    Definition.__init__(self, data_type.source, [], None)
    self.data_type = data_type
    self.size = size
    self.is_type = True

  def __repr__(self):
    return '%s[]' % str(self.data_type)

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Array."""
    if self.size:
      return 'sized_array'
    else:
      return 'unsized_array'


class Nullable(Definition):
  """Nullable definition.

  Nullable objects are those that can have a null value. They are not
  defined explicitly in IDL, but implicitly through nullable references.
  Nullables are types.

  Attributes:
    data_type: the type this Nullable has when it is not null.
  """
  defn_type = 'Nullable'

  def __init__(self, data_type):
    """Inits an Array instance.

    Args:
      data_type: the type this Nullable has when it is not null.
    """
    Definition.__init__(self, data_type.source, [], None)
    self.data_type = data_type
    self.is_type = True
    self.nullable = self

  def __repr__(self):
    return '%s?' % str(self.data_type)

  def LookUpBindingModel(self):
    """Implementation of LookUpBindingModel for Nullable."""
    return 'nullable'


def CheckTypeInChain(type_defn, chain_head):
  """Checks that a type isn't in a type chain.

  Circular references of types shouldn't happen within typedefs, base class or
  array chains. For example the following IDL constructs should be forbidden:

    typedef Typedef1 Typedef2;
    typedef Typedef2 Typedef1;

    class Class1: Class2 {};
    class Class2: Class1 {};

    typedef Typedef1[] Typedef1;

    typedef Class Typedef;
    class Class: Typedef {};

  and so on. This function checks that a particular type doesn't already exists
  in a chain, defined by following typedefs, base classes and array data types.

  Args:
    type_defn: the type to look for.
    chain_head: the head of the chain to look into.

  Raises:
    CircularTypedefError: a circular reference was found, that is 'type' was
      found in the chain.
  """
  current = chain_head
  while current:
    if type_defn == current:
      raise CircularTypedefError(type_defn, current)
    if current.defn_type == 'Typedef':
      # Don't use GetTypeSafe, just break if the type hasn't been set
      # using GetTypeSafe would produce an infinite loop...
      # It is ok to break if the type hasn't been set yet. If there is a
      # cycle, it will be detected when that other type calls GetTypeSafe
      current = current.type_defn
    elif current.defn_type == 'Class':
      # same as above, don't use GetBaseTypeSafe
      current = current.base_type
    elif current.defn_type == 'Array':
      current = current.data_type
    else:
      break


class LookUpScope(object):
  """Helper class for lookup in scope definitions.

  This class provides basic functionality for LookUpType and FindScope
  implementations, when the definition is a scope, like classes and namespaces.
  Caching and such is implemented here. Currently, only type lookups is cached
  with a name -> definition dictionary.

  Attributes:
    list: the list of definition the lookup operates on.
  """

  def __init__(self, definition_list):
    """Inits a LookUpScope instance.

    Args:
      definition_list: the list of definition the lookup operates on.
    """
    self.list = definition_list
    self.ResetCache()

  def ResetCache(self):
    """Resets all caches. To be used if the definition list changes."""
    self._types = None

  def MakeCache(self):
    """Initializes caches."""
    if not self._types:
      self._types = dict([(i.name, i) for i in self.list if i.is_type])

  def LookUpType(self, name):
    """Looks up a type by name within the definition list.

    Args:
      name: the name of the type looked for.

    Returns:
      The type definition if found, None otherwise.
    """
    self.MakeCache()
    try:
      return self._types[name]
    except KeyError:
      return None

  def FindScopes(self, name):
    """Finds all socpes matching a name, in the definition list.

    Args:
      name: the name of the scopes looked for.

    Returns:
      The list of all the scopes that were found. May be [] if no scope of that
      name was found.
    """
    return [i for i in self.list if i.is_scope and i.name == name]


def GetObjectsRecursive(object_list):
  """Gets the list of all objects recursively defined in a list of objects.

    This method gets the list of all objects defined within a list of top-level
    objects, going recursively through them.

    Args:
      object_list: the list of top-level objects.

    Returns:
      The list of objects.
    """
  return sum([obj.GetObjectsRecursive() for obj in object_list], [])


def MergeNamespacesRecursive(namespace):
  """Merges scopes described by namespaces constructs.

  Namespaces can be defined by parts, this function merges scopes of namespaces
  that have the same name, and are defined within the same outer scope. E.g.
  the following:
  namespace Foo {
    namespace Bar {
      class Class1 {};
      void Function2();
    }

    namespace Bar {
      int my_variable;
    }
  }
  will create within the namespace 'Foo' two Namespace objects, both named
  'Bar', each containing different definitions, and different inner LookUpScope
  objects.
  This function will merge the LookUpScope objects between these namespaces, for
  it to contain all the definitions. The Namespace objects will still exist as
  separate entities, only their scope member changes.

  Args:
    namespace: the root namespace containing namespace definitions to merge.
  """
  namespaces = [o for o in namespace.scope.list if o.defn_type == 'Namespace']
  namespace_dict = {}
  for o in namespaces:
    if o.name in namespace_dict:
      namespace_dict[o.name].MergeLookUpScope(o)
    else:
      namespace_dict[o.name] = o
  for name in namespace_dict:
    MergeNamespacesRecursive(namespace_dict[name])


def FinalizeObjects(namespace, binding_models):
  """Finalize all objects after parsing.

  When objects are first parsed, they are not completely ready for code
  generation. This functions takes care of finalizing objects:
  - merging namespace scopes that have the same name in the same outer scope.
  - resolving type references.
  - setting binding models on types.

  Args:
    namespace: 'global' namespace, containing all the definitions.
    binding_models: a dictionary that maps binding model name -> binding model
      module.

  Raises:
    TypeNotFoundError: a type reference could not be resolved.
    CircularTypedefError: circular type references were found.
    DerivingFromNonClassError: a class definition derives from a non-class
      definition.
    UnknownBindingModelError: a type definition doesn't have a valid binding
      model.
  """
  MergeNamespacesRecursive(namespace)
  all_defns = namespace.GetObjectsRecursive()
  for defn in all_defns:
    defn.ResolveTypeReferences()
  for defn in all_defns:
    if defn.is_type:
      defn.SetBindingModel(binding_models)


def main():
  pass


if __name__ == '__main__':
  main()
