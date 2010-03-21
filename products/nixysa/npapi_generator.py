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

"""NPAPI glue generator.

This module implements the generator for NPAPI glue code.

For each class, the generator will create two NPObject classes:
  - one for the class instances, exposing all the member functions and
    properties, that wraps C++ instances. This one is created by the binding
    model module - this is called the 'instance object'
  - one for all the static members, like static functions and properties, but
    also inner types. Only one instance of this one will be created. This one
    is common to all the binding models, and is defined in static_object.h
    (glue::globals::NPAPIObject) - this is called the 'static object'

For namespaces, one of the second type is created as well (containing global
functions and properties, and inner namespaces/classes).

That way if the IDL contains something like:

  namespace A {
    class B {
      static void C();
    };
  }

then one can access C in JavaScript through plugin.A.B.C();

The tricky part in this is that for namespaces, the definition of all the
members spans across multiple namespace definitions, possibly across multiple
files, but only one NPObject should exist, gathering all the members from all
the namespace definitions - that means the glue has to be generated for all the
definitions at once, generating the glue for each file separately will not
work. By convention, the NPAPI glue for a namespace will be defined in the
first file encountered that has a part of the namespace definition.

Because of that, the code generation happens in 2 passes. The first one
generates the code for all the definitions except the namespaces, and gather
all the data for the namespace generation. Then the second pass generates the
code for the namespaces.
"""

import string
import cpp_utils
import globals_binding
import idl_parser
import naming
import npapi_utils
import pod_binding
import syntax_tree


# default includes to add to the generated glue files

_cpp_includes = [('plugin_main.h', False)]

_header_includes = [('string.h', True),
                    ('string', True),
                    ('npapi.h', True),
                    ('npruntime.h', True),
                    ('common.h', False),
                    ('static_object.h', False)]


# glue templates and helper strings

_enumerate_static_property_entries = """
  memcpy(output, static_property_ids,
     NUM_STATIC_PROPERTY_IDS * sizeof(NPIdentifier));
  output += NUM_STATIC_PROPERTY_IDS;
"""

_enumerate_static_method_entries = """
  memcpy(output, static_method_ids,
     NUM_STATIC_METHOD_IDS * sizeof(NPIdentifier));
  output += NUM_STATIC_METHOD_IDS;
"""

_enumerate_namespace_entries = """
  memcpy(output, namespace_ids,
     NUM_NAMESPACE_IDS * sizeof(NPIdentifier));
  output += NUM_NAMESPACE_IDS;
"""

_enumerate_property_entries = """
  memcpy(output, property_ids,
     NUM_PROPERTY_IDS * sizeof(NPIdentifier));
  output += NUM_PROPERTY_IDS;
"""

_enumerate_method_entries = """
  memcpy(output, method_ids,
     NUM_METHOD_IDS * sizeof(NPIdentifier));
  output += NUM_METHOD_IDS;
"""

_class_glue_header_static = """
void InitializeGlue(NPP npp);
NPClass *GetStaticNPClass(void);
glue::globals::NPAPIObject *CreateRawStaticNPObject(NPP npp);
void RegisterObjectBases(glue::globals::NPAPIObject *namespace_object,
                         glue::globals::NPAPIObject *root_object);
glue::globals::NPAPIObject *GetStaticNPObject(
    glue::globals::NPAPIObject *root_object);
void StaticEnumeratePropertyHelper(NPIdentifier *output);
uint32_t GetStaticPropertyCount();
bool StaticInvoke(glue::globals::NPAPIObject *object,
                  NPP npp,
                  NPIdentifier name,
                  const NPVariant *args,
                  uint32_t argCount,
                  NPVariant *result,
                  const char **error_handle);
bool StaticGetProperty(glue::globals::NPAPIObject *object,
                       NPP npp,
                       NPIdentifier name,
                       NPVariant *variant,
                       const char **error_handle);
bool StaticSetProperty(glue::globals::NPAPIObject *object,
                       NPP npp,
                       NPIdentifier name,
                       const NPVariant *variant,
                       const char **error_handle);
"""

_class_glue_header_member = """
NPClass *GetNPClass(void);
bool Invoke(${ClassMutableParamType} object,
            NPP npp,
            NPIdentifier name,
            const NPVariant *args,
            uint32_t argCount,
            NPVariant *result,
            const char **error_handle);
bool GetProperty(${ClassParamType} object,
                 NPP npp,
                 NPIdentifier name,
                 NPVariant *variant,
                 const char **error_handle);
bool SetProperty(${ClassMutableParamType} object,
                 NPP npp,
                 NPIdentifier name,
                 const NPVariant *variant,
                 const char **error_handle);
bool EnumeratePropertyEntries(NPObject *header,
                              NPIdentifier **value,
                              uint32_t *count);
void EnumeratePropertyEntriesHelper(NPIdentifier *output);
uint32_t GetPropertyCount();

${BindingGlueHeader}
"""

_class_glue_header_template = string.Template(_class_glue_header_static +
                                              _class_glue_header_member)

_class_glue_cpp_common_head_static = """
static bool StaticHasMethod(NPObject *header, NPIdentifier name);
static bool StaticInvokeEntry(NPObject *header,
                              NPIdentifier name,
                              const NPVariant *args,
                              uint32_t argCount,
                              NPVariant *result);
static bool StaticInvokeDefault(NPObject *header,
                                const NPVariant *args,
                                uint32_t argCount,
                                NPVariant *result);
static bool StaticHasProperty(NPObject *header, NPIdentifier name);
static bool StaticGetPropertyEntry(NPObject *header,
                                   NPIdentifier name,
                                   NPVariant *variant);
static bool StaticSetPropertyEntry(NPObject *header,
                                   NPIdentifier name,
                                   const NPVariant *variant);
static bool StaticEnumeratePropertyEntries(NPObject *header,
                                           NPIdentifier **value,
                                           uint32_t *count);
void StaticEnumeratePropertyHelper(NPIdentifier *output);
uint32_t GetStaticPropertyCount();

static NPClass static_npclass = {
  NP_CLASS_STRUCT_VERSION,
  glue::globals::Allocate,
  glue::globals::Deallocate,
  0,
  StaticHasMethod,
  StaticInvokeEntry,
  StaticInvokeDefault,
  StaticHasProperty,
  StaticGetPropertyEntry,
  StaticSetPropertyEntry,
  0,
  StaticEnumeratePropertyEntries,
};

NPClass *GetStaticNPClass(void)
{
  return &static_npclass;
}

${StaticPropertyTable}
${StaticMethodTable}
${NamespaceTable}

uint32_t GetStaticPropertyCount() {
  return ${StaticPropertyCount} + ${StaticMethodCount} + ${NamespaceCount};
}

static bool StaticEnumeratePropertyEntries(NPObject *header,
                                           NPIdentifier **value,
                                           uint32_t *count) {
  *count = 0;
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticEnumeratePropertyEntries", prof);
  ${AddStaticPropertyCount}
  ${AddStaticMethodCount}
  ${AddNamespaceCount}
  if (*count) {
    GLUE_PROFILE_START(npp, "memalloc");
    *value = static_cast<NPIdentifier *>(
        NPN_MemAlloc(*count * sizeof(NPIdentifier)));
    GLUE_PROFILE_STOP(npp, "memalloc");
    StaticEnumeratePropertyHelper(*value);
  } else {
    *value = NULL;
  }
  return true;
}

// This is broken out into a separate function so that the plugin object can
// call it on the global namespace without extra memory allocation.
// The caller is responsible for making sure there's sufficient space in output.
void StaticEnumeratePropertyHelper(NPIdentifier *output) {
  ${EnumerateStaticPropertyEntries}
  ${EnumerateStaticMethodEntries}
  ${EnumerateNamespaceEntries}
}

static void InitializeStaticIds(NPP npp) {
  ${StaticPropertyInit}
  ${StaticMethodInit}
  ${NamespaceInit}
  ${#InitNamespaceGlues}
}

glue::globals::NPAPIObject *CreateRawStaticNPObject(NPP npp) {
  GLUE_PROFILE_START(npp, "createobject");
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(
          NPN_CreateObject(npp, &static_npclass));
  GLUE_PROFILE_STOP(npp, "createobject");
  ${#CreateNamespaces}
  return object;
}

void RegisterObjectBases(glue::globals::NPAPIObject *namespace_object,
                         glue::globals::NPAPIObject *root_object) {
  ${#RegisterBases}
}

${#GetStaticObjects}

bool StaticInvokeDefault(NPObject *header,
                         const NPVariant *args,
                         uint32_t argCount,
                         NPVariant *result) {
  const char *error=NULL;
  const char **error_handle = &error;
  bool success = true;
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticInvokeDefault", prof);
  ${#StaticInvokeDefaultCode}
  // Skip out early on the profiling, so as not to count error callback time.
  GLUE_SCOPED_PROFILE_STOP(prof);
  if (!success && error) {
    glue::globals::SetLastError(npp, error);
  }
  return false;
}

static bool StaticInvokeEntry(NPObject *header,
                              NPIdentifier name,
                              const NPVariant *args,
                              uint32_t argCount,
                              NPVariant *result) {
  // Chrome transforms InvokeDefault into Invoke with null parameter:
  // http://code.google.com/p/chromium/issues/detail?id=5110
  if (name == NULL)
    return StaticInvokeDefault(header, args, argCount, result);
  const char *error=NULL;
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_PROFILE_START(npp, std::string("${Class}::StaticInvokeEntry(") +
      (id.text() ? id.text() : "") + ")");
  bool success = StaticInvoke(object, npp, name, args, argCount, result,
                              &error);
  GLUE_PROFILE_STOP(npp, std::string("${Class}::StaticInvokeEntry(") +
      (id.text() ? id.text() : "") + ")");
  if (!success && error) {
    glue::globals::SetLastError(npp, error);
  }
  return success;
}

static bool StaticGetPropertyEntry(NPObject *header,
                                   NPIdentifier name,
                                   NPVariant *variant) {
  const char *error=NULL;
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_PROFILE_START(npp, std::string("${Class}::StaticGetPropertyEntry(") +
      (id.text() ? id.text() : "") + ")");
  bool success = StaticGetProperty(object, npp, name, variant, &error);
  GLUE_PROFILE_STOP(npp, std::string("${Class}::StaticGetPropertyEntry(") +
      (id.text() ? id.text() : "") + ")");
  if (!success && error) {
    glue::globals::SetLastError(npp, error);
  }
  return success;
}

static bool StaticSetPropertyEntry(NPObject *header,
                                   NPIdentifier name,
                                   const NPVariant *variant) {
  const char *error=NULL;
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_PROFILE_START(npp, std::string("${Class}::StaticSetPropertyEntry(") +
      (id.text() ? id.text() : "") + ")");
  bool success = StaticSetProperty(object, npp, name, variant, &error);
  GLUE_PROFILE_STOP(npp, std::string("${Class}::StaticSetPropertyEntry(") +
      (id.text() ? id.text() : "") + ")");
  if (!success && error) {
    glue::globals::SetLastError(npp, error);
  }
  return success;
}

"""

_class_glue_cpp_common_head_member = """
static NPObject *Allocate(NPP npp, NPClass *theClass);
static void Deallocate(NPObject *header);
static bool HasMethod(NPObject *header, NPIdentifier name);
static bool InvokeEntry(NPObject *header,
                        NPIdentifier name,
                        const NPVariant *args,
                        uint32_t argCount,
                        NPVariant *result);
static bool HasProperty(NPObject *header, NPIdentifier name);
static bool GetPropertyEntry(NPObject *header,
                             NPIdentifier name,
                             NPVariant *variant);
static bool SetPropertyEntry(NPObject *header,
                             NPIdentifier name,
                             const NPVariant *variant);
bool EnumeratePropertyEntries(NPObject *header,
                              NPIdentifier **value,
                              uint32_t *count);
void EnumeratePropertyEntriesHelper(NPIdentifier *output);
uint32_t GetPropertyCount();
static uint32_t GetLocalPropertyCount();



static NPClass npclass = {
  NP_CLASS_STRUCT_VERSION,
  Allocate,
  Deallocate,
  0,
  HasMethod,
  InvokeEntry,
  0,
  HasProperty,
  GetPropertyEntry,
  SetPropertyEntry,
  0,
  EnumeratePropertyEntries
};

NPClass *GetNPClass(void)
{
  return &npclass;
}

${PropertyTable}
${MethodTable}

uint32_t GetPropertyCount() {
  return GetLocalPropertyCount() + ${BaseGetPropertyCount};
}

static uint32_t GetLocalPropertyCount() {
  return ${PropertyCount} + ${MethodCount};
}

bool EnumeratePropertyEntries(NPObject *header,
                              NPIdentifier **value,
                              uint32_t *count) {
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_PROFILE_START(npp, "${Class}::EnumeratePropertyEntries");
  *count = GetPropertyCount();
  GLUE_PROFILE_START(npp, "memalloc");
  *value = static_cast<NPIdentifier *>(
      NPN_MemAlloc(*count * sizeof(NPIdentifier)));
  GLUE_PROFILE_STOP(npp, "memalloc");
  EnumeratePropertyEntriesHelper(*value);
  GLUE_PROFILE_STOP(npp, "${Class}::EnumeratePropertyEntries");
  return true;
}

// This is broken out into a separate function so that derived classes can
// call it as well without extra memory allocation.
// The caller is responsible for making sure there's sufficient space in output.
void EnumeratePropertyEntriesHelper(NPIdentifier *output) {
  ${EnumeratePropertyEntries}
  ${EnumerateMethodEntries}
  ${EnumeratePropertyEntriesHelperBaseCall}
}

static void InitializeMemberIds(NPP npp) {
  ${PropertyInit}
  ${MethodInit}
}

static void InitializeIds(NPP npp) {
  InitializeMemberIds(npp);
  InitializeStaticIds(npp);
}

static bool InvokeEntry(NPObject *header,
                        NPIdentifier name,
                        const NPVariant *args,
                        uint32_t argCount,
                        NPVariant *result) {
  const char *error=NULL;
  const char **error_handle = &error;
  DebugScopedId id(name);  // debug helper
  bool success = true;
  ${DispatchFunctionHeader}
  // Profile is a bit late, but it makes npp lookup easier.
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::InvokeEntry(") + (id.text() ?
      id.text() : "") + ")", prof);
  if (!success) return false;
  bool ret = Invoke(${Object}, npp, name, args, argCount, result, error_handle);
  GLUE_SCOPED_PROFILE_STOP(prof);
  if (!ret && error) {
    glue::globals::SetLastError(npp, error);
    return false;
  }
  return ret;
}

static bool GetPropertyEntry(NPObject *header,
                             NPIdentifier name,
                             NPVariant *variant) {
  const char *error=NULL;
  const char **error_handle = &error;
  DebugScopedId id(name);  // debug helper
  bool success = true;
  ${DispatchFunctionHeader}
  // Profile is a bit late, but it makes npp lookup easier.
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::GetPropertyEntry(") +
      (id.text() ? id.text() : "") + ")", prof);
  if (!success) return false;  // A rare error case.
  bool ret = GetProperty(${ObjectNonMutable}, npp, name, variant, error_handle);
  GLUE_SCOPED_PROFILE_STOP(prof);
  if (!ret && error) {
    glue::globals::SetLastError(npp, error);
    return false;
  }
  return ret;
}

static bool SetPropertyEntry(NPObject *header,
                             NPIdentifier name,
                             const NPVariant *variant) {
  const char *error=NULL;
  const char **error_handle = &error;
  DebugScopedId id(name);  // debug helper
  bool success = true;
  ${DispatchFunctionHeader}
  // Profile is a bit late, but it makes npp lookup easier.
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::SetPropertyEntry(") +
      (id.text() ? id.text() : "") + ")", prof);
  if (!success) return false;  // A rare error case.
  bool ret = SetProperty(${Object}, npp, name, variant, error_handle);
  GLUE_SCOPED_PROFILE_STOP(prof);
  if (!ret && error) {
    glue::globals::SetLastError(npp, error);
    return false;
  }
  return ret;
}

${BindingGlueCpp}
"""

_class_glue_cpp_base_member = """
bool Invoke(${ClassMutableParamType} object,
            NPP npp,
            NPIdentifier name,
            const NPVariant *args,
            uint32_t argCount,
            NPVariant *result,
            const char **error_handle) {
  DebugScopedId id(name);  // debug helper
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::Invoke(") + (id.text() ?
      id.text() : "") + ")", prof);
  bool success = true;
  ${#InvokeCode}
  return ${BaseClassNamespace}::Invoke(object, npp, name, args, argCount,
      result, error_handle);
}

bool GetProperty(${ClassParamType} object,
                 NPP npp,
                 NPIdentifier name,
                 NPVariant *variant,
                 const char **error_handle) {
  DebugScopedId id(name);  // debug helper
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::GetProperty(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${#GetPropertyCode}
  return ${BaseClassNamespace}::GetProperty(object, npp, name, variant,
      error_handle);
}

bool SetProperty(${ClassMutableParamType} object,
                 NPP npp,
                 NPIdentifier name,
                 const NPVariant *variant,
                 const char **error_handle) {
  DebugScopedId id(name);  // debug helper
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::SetProperty(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${#SetPropertyCode}
  return ${BaseClassNamespace}::SetProperty(object, npp, name, variant,
      error_handle);
}

static bool HasMethod(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::HasMethod(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${MethodCheck}
  return ${BaseClassNamespace}::GetNPClass()->hasMethod(header, name);
}

static bool HasProperty(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::HasProperty(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${PropertyCheck}
  return ${BaseClassNamespace}::GetNPClass()->hasProperty(header, name);
}
"""

_class_glue_cpp_base_static = """
bool StaticInvoke(glue::globals::NPAPIObject *object,
                  NPP npp,
                  NPIdentifier name,
                  const NPVariant *args,
                  uint32_t argCount,
                  NPVariant *result,
                  const char **error_handle) {
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticInvoke", prof);
  bool success = true;
  ${#StaticInvokeCode}
  return ${BaseClassNamespace}::StaticInvoke(
      object->base(), npp, name, args, argCount, result, error_handle);
}

bool StaticGetProperty(glue::globals::NPAPIObject *object,
                       NPP npp,
                       NPIdentifier name,
                       NPVariant *variant,
                       const char **error_handle) {
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticGetProperty", prof);
  bool success = true;
  ${#StaticGetPropertyCode}
  if (glue::globals::GetProperty(object, name, variant)) return true;
  return ${BaseClassNamespace}::StaticGetProperty(
      object->base(), npp, name, variant, error_handle);
}

bool StaticSetProperty(glue::globals::NPAPIObject *object,
                       NPP npp,
                       NPIdentifier name,
                       const NPVariant *variant,
                       const char **error_handle) {
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticSetProperty", prof);
  bool success = true;
  ${#StaticSetPropertyCode}
  if (glue::globals::SetProperty(object, name, variant)) return true;
  return ${BaseClassNamespace}::StaticSetProperty(
      object->base(), npp, name, variant, error_handle);
}

static bool StaticHasMethod(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::StaticHasMethod(") +
      (id.text() ? id.text() : "") + ")", prof);
  ${StaticMethodCheck}
  GLUE_SCOPED_PROFILE(npp, "hasmethod", prof1);
  return NPN_HasMethod(npp, object->base(), name);
}

static bool StaticHasProperty(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::StaticHasProperty(") +
      (id.text() ? id.text() : "") + ")", prof);
  ${StaticPropertyCheck}
  bool success = glue::globals::HasProperty(header, name);
  if (success) {
    return success;
  }
  GLUE_SCOPED_PROFILE(npp, "hasproperty", prof1);
  return NPN_HasProperty(npp, object->base(), name);
}

"""
_class_glue_cpp_no_base_member = """
bool Invoke(${ClassMutableParamType} object,
            NPP npp,
            NPIdentifier name,
            const NPVariant *args,
            uint32_t argCount,
            NPVariant *result,
            const char **error_handle) {
  DebugScopedId id(name);  // debug helper
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::Invoke(") + (id.text() ?
      id.text() : "") + ")", prof);
  bool success = true;
  ${#InvokeCode}
  if (!*error_handle) {
    *error_handle =
        "Method not found; perhaps it doesn't take that number of arguments?";
  }
  return false;
}

bool GetProperty(${ClassParamType} object,
                 NPP npp,
                 NPIdentifier name,
                 NPVariant *variant,
                 const char **error_handle) {
  DebugScopedId id(name);  // debug helper
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::GetProperty(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${#GetPropertyCode}
  if (!*error_handle) {
    *error_handle = "Property not found.";
  }
  return false;
}

bool SetProperty(${ClassMutableParamType} object,
                 NPP npp,
                 NPIdentifier name,
                 const NPVariant *variant,
                 const char **error_handle) {
  DebugScopedId id(name);  // debug helper
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::SetProperty(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${#SetPropertyCode}
  if (!*error_handle) {
    *error_handle = "Property not found.";
  }
  return false;
}

static bool HasMethod(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::HasMethod(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${MethodCheck}
  return false;
}

static bool HasProperty(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::HasProperty(") + (id.text() ?
      id.text() : "") + ")", prof);
  ${PropertyCheck}
  return false;
}
"""

_class_glue_cpp_no_base_static = """
bool StaticInvoke(glue::globals::NPAPIObject *object,
                  NPP npp,
                  NPIdentifier name,
                  const NPVariant *args,
                  uint32_t argCount,
                  NPVariant *result,
                  const char **error_handle) {
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticInvoke", prof);
  bool success = true;
  ${#StaticInvokeCode}
  return false;
}

bool StaticGetProperty(glue::globals::NPAPIObject *object,
                       NPP npp,
                       NPIdentifier name,
                       NPVariant *variant,
                       const char **error_handle) {
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticGetProperty", prof);
  bool success = true;
  ${#StaticGetPropertyCode}
  success = glue::globals::GetProperty(object, name, variant);
  return success;
}

bool StaticSetProperty(glue::globals::NPAPIObject *object,
                       NPP npp,
                       NPIdentifier name,
                       const NPVariant *variant,
                       const char **error_handle) {
  GLUE_SCOPED_PROFILE(npp, "${Class}::StaticSetProperty", prof);
  bool success = true;
  ${#StaticSetPropertyCode}
  if (glue::globals::SetProperty(object, name, variant)) return true;
  return false;
}

static bool StaticHasMethod(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::StaticHasMethod(") +
      (id.text() ? id.text() : "") + ")", prof);
  ${StaticMethodCheck}
  return false;
}

static bool StaticHasProperty(NPObject *header, NPIdentifier name) {
  DebugScopedId id(name);  // debug helper
  glue::globals::NPAPIObject *object =
      static_cast<glue::globals::NPAPIObject *>(header);
  NPP npp = object->npp();
  GLUE_SCOPED_PROFILE(npp, std::string("${Class}::StaticHasProperty(") +
      (id.text() ? id.text() : "") + ")", prof);
  ${StaticPropertyCheck}
  return glue::globals::HasProperty(header, name);
}
"""

_namespace_glue_cpp_tail = """
void InitializeGlue(NPP npp) {
  InitializeStaticIds(npp);
}
"""

_class_glue_cpp_base_template = string.Template(''.join([
    _class_glue_cpp_common_head_static,
    _class_glue_cpp_common_head_member,
    _class_glue_cpp_base_static,
    _class_glue_cpp_base_member]))

_class_glue_cpp_no_base_template = string.Template(''.join([
    _class_glue_cpp_common_head_static,
    _class_glue_cpp_common_head_member,
    _class_glue_cpp_no_base_static,
    _class_glue_cpp_no_base_member]))

_namespace_glue_header = _class_glue_header_static

_namespace_glue_cpp_template = string.Template(''.join([
    _class_glue_cpp_common_head_static,
    _class_glue_cpp_no_base_static,
    _namespace_glue_cpp_tail]))

_callback_glue_cpp_template = string.Template("""
${RunCallback} {
${StartException}
  const char *error=NULL;
  const char **error_handle = &error;
  bool success = true;
  NPVariant args[${ArgCount}];
  NPVariant result;
  NULL_TO_NPVARIANT(result);
  ${ParamsToVariantsPre}
  if (success) {
    ${ParamsToVariantsPost}
    if (async && NPCallback::SupportsAsync()) {
      NPCallback* callback = NPCallback::Create(npp);
      if (callback) {
        callback->Set(npobject, args, ${ArgCount});
        callback->CallAsync();
        NPN_ReleaseObject(callback);
        success = true;
      } else {
        success = false;
      }
    } else {
      GLUE_PROFILE_START(npp, "invokeDefault");
      success = NPN_InvokeDefault(npp,
                                  npobject,
                                  args,
                                  ${ArgCount},
                                  &result);
      GLUE_PROFILE_STOP(npp, "invokeDefault");
      if (success) {
        GLUE_PROFILE_START(npp, "NPN_ReleaseVariantValue");
        NPN_ReleaseVariantValue(&result);
        GLUE_PROFILE_STOP(npp, "NPN_ReleaseVariantValue");
      }
    }
  }
  for (int i = 0; i != ${ArgCount}; ++i) {
    NPN_ReleaseVariantValue(&args[i]);
  }
  ${ReturnEval}
  return ${ReturnValue};
${EndException}
}
""")

_callback_no_param_glue_cpp_template = string.Template("""
${RunCallback} {
${StartException}
  const char *error=NULL;
  const char **error_handle = &error;
  bool success = true;
  NPVariant result;
  NULL_TO_NPVARIANT(result);
  if (success) {
    if (async && NPCallback::SupportsAsync()) {
      NPCallback* callback = NPCallback::Create(npp);
      if (callback) {
        callback->Set(npobject, NULL, 0);
        callback->CallAsync();
        NPN_ReleaseObject(callback);
        success = true;
      } else {
        success = false;
      }  
    } else {
      GLUE_PROFILE_START(npp, "invokeDefault");
      success = NPN_InvokeDefault(npp,
                                  npobject,
                                  NULL,
                                  0,
                                  &result);
      GLUE_PROFILE_STOP(npp, "invokeDefault");
      if (success) {
        GLUE_PROFILE_START(npp, "NPN_ReleaseVariantValue");
        NPN_ReleaseVariantValue(&result);
        GLUE_PROFILE_STOP(npp, "NPN_ReleaseVariantValue");
      }
    }
  }
  ${ReturnEval}
  return ${ReturnValue};
${EndException}
}
""")


_initialize_glue_template = string.Template(
    '${Namespace}::InitializeGlue(npp);')

_create_namespace_template = string.Template("""
object->SetNamespaceObject(${PROPERTY},
    ${Namespace}::CreateRawStaticNPObject(npp));""")

_register_base_template = string.Template("""
{
  glue::globals::NPAPIObject *object =
      namespace_object->GetNamespaceObjectByIndex(${PROPERTY});
  object->set_base(${BaseClassNamespace}::GetStaticNPObject(root_object));
  ${Namespace}::RegisterObjectBases(object, root_object);
}""")

_register_no_base_template = string.Template("""
{
  glue::globals::NPAPIObject *object =
      namespace_object->GetNamespaceObjectByIndex(${PROPERTY});
  ${Namespace}::RegisterObjectBases(object, root_object);
}""")

_get_ns_object_template = string.Template("""
namespace ${Namespace} {
glue::globals::NPAPIObject *GetStaticNPObject(
    glue::globals::NPAPIObject *root_object) {
  glue::globals::NPAPIObject *parent =
      ${ParentNamespace}::GetStaticNPObject(root_object);
  return parent->GetNamespaceObjectByIndex(${PROPERTY});
}
}  // namespace ${Namespace}""")

_globals_glue_header_tail = """
glue::globals::NPAPIObject *CreateStaticNPObject(NPP npp);
"""

_globals_glue_cpp_tail = """
glue::globals::NPAPIObject *GetStaticNPObject(
    glue::globals::NPAPIObject *root_object) {
  return root_object;
}

glue::globals::NPAPIObject *CreateStaticNPObject(NPP npp) {
  glue::globals::NPAPIObject *root_object = CreateRawStaticNPObject(npp);
  RegisterObjectBases(root_object, root_object);
  return root_object;
}
"""

# code pieces templates

_method_invoke_template = string.Template("""
  if (name == ${table}[${method_id}] && argCount == ${argCount}) do {
    bool success = true;
    ${code}
  } while(false);""")

_method_default_invoke_template = string.Template("""
  if (argCount == ${argCount}) do {
    bool success = true;
    ${code}
  } while(false);""")

_property_template = string.Template("""
  if (name == ${table}[${property_id}]) do {
    bool success = true;
    ${code}
  } while(false);""")

_failure_test_string = '    if (!success) break;'

_exception_context_start_template = string.Template(
    """#define ${exception_macro_name} "${type} '${name}'" """)

_exception_context_end_template = string.Template(
    """#undef ${exception_macro_name}""")

_exception_macro_name = 'NPAPI_GLUE_EXCEPTION_CONTEXT'

def GenExceptionContext(exception_macro_name, type, name):
  """Create code to define the context for exception error messages.

  Args:
    exception_macro_name: the name to use for the macro
    type: the type of access that name represents (field, parameter, etc.)
    name: the name of the variable

  Returns:
    a tuple of 2 strings; the first #defines the text to stick in the
    exception, and the second #undefs the string to clean up the namespace.
  """
  start = _exception_context_start_template.substitute(type=type,
                                                       name=name,
                                                       exception_macro_name=
                                                           exception_macro_name)
  end = _exception_context_end_template.substitute(exception_macro_name=
                                                       exception_macro_name)
  return (start, end)

def GenEndExceptionContext(exception_macro_name):
  """Create code to clean up the context definition for exception error
  messages.

  Args:
    exception_macro_name: the name to use for the macro

  Returns:
    the string to #undef the macro
  """

def GetGlueHeader(idl_file):
  """Gets the name of the glue header file.

  Args:
    idl_file: an idl_parser.File, the source IDL file.

  Returns:
    the name of the header file.
  """
  if 'npapi_header' in idl_file.__dict__:
    return idl_file.npapi_header
  else:
    return idl_file.basename + '_glue.h'


def GetGlueCpp(idl_file):
  """Gets the name of the glue implementation file.

  Args:
    idl_file: an idl_parser.File, the source IDL file.

  Returns:
    the name of the implementation file.
  """
  if 'npapi_cpp' in idl_file.__dict__:
    return idl_file.npapi_cpp
  else:
    return idl_file.basename + '_glue.cc'


class MethodWithoutReturnType(Exception):
  """Raised when finding a function without return type."""

  def __init__(self, obj):
    Exception.__init__(self)
    self.object = obj


def GenNamespaceCode(context):
  """Generates the code for namespace glue.

  This function generates the necessary code to initialize the
  globals::NPAPIObject instance with the inner namespace objects.

  Args:
    context: the NpapiGenerator.CodeGenContext for generating the glue.

  Returns:
    a dict is generated by npapi_utils.MakeIdTableDict, and contains the
    substitution strings for the namespace ids.
  """
  namespace_ids = []
  if context.namespace_list:
    context.namespace_create_section.EmitCode(
        'object->AllocateNamespaceObjects(NUM_NAMESPACE_IDS);')
    context.namespace_create_section.EmitCode(
        'object->set_names(namespace_ids);')
    for ns_obj in context.namespace_list:
      id_enum = 'SCOPE_%s' % naming.Normalize(ns_obj.name, naming.Upper)
      namespace_ids.append((id_enum, '"%s"' % ns_obj.name))
      full_namespace = npapi_utils.GetGlueFullNamespace(ns_obj)
      context.namespace_init_section.EmitCode(
          _initialize_glue_template.substitute(Namespace=full_namespace))
      context.namespace_create_section.EmitCode(
          _create_namespace_template.substitute(PROPERTY=id_enum,
                                                Namespace=full_namespace))
      if ns_obj.defn_type == 'Class' and ns_obj.base_type:
        base_class_namespace = npapi_utils.GetGlueFullNamespace(
            ns_obj.base_type.GetFinalType())
        context.namespace_register_base_section.EmitCode(
            _register_base_template.substitute(
                PROPERTY=id_enum,
                BaseClassNamespace=base_class_namespace,
                Namespace=full_namespace))
      else:
        context.namespace_register_base_section.EmitCode(
            _register_no_base_template.substitute(PROPERTY=id_enum,
                                                  Namespace=full_namespace))

      context.namespace_get_static_object_section.EmitCode(
          _get_ns_object_template.substitute(
              Namespace=npapi_utils.GetGlueNamespace(ns_obj.GetFinalType()),
              ParentNamespace=npapi_utils.GetGlueFullNamespace(
                  ns_obj.parent.GetFinalType()),
              PROPERTY=id_enum))
  return npapi_utils.MakeIdTableDict(namespace_ids, 'namespace')


def MakePodType(name):
  """Creates a pod type with reasonable attributes.

  This function is used to be able to generate parameters that are not directly
  referenced in the IDL

  Args:
    name: the name of the pod type.

  Returns:
    a Definition for the type.
  """
  source_file = idl_parser.File('<internal>')
  source_file.header = None
  source_file.npapi_cpp = None
  source_file.npapi_header = None
  source = idl_parser.SourceLocation(source_file, 0)
  attributes = {'binding_model': 'pod'}
  type_defn = syntax_tree.Typename(source, attributes, name)
  type_defn.binding_model = pod_binding
  type_defn.podtype = 'variant'
  return type_defn


class NpapiGenerator(object):
  """Main generator class."""

  def __init__(self, output_dir):
    """Inits a NpapiGenerator instance.

    Args:
      output_dir: the output directory for generated files.
    """
    self._output_dir = output_dir
    self._namespace_map = {}
    self._finalize_functions = []
    # TODO: instead of passing a raw void *, it would be better to define a
    # PluginInstance class. Needs a fair amount of refactoring in the C++ code.
    self._plugin_data_type = MakePodType('void *')

  class CodeGenContext(object):
    """Code generation context.

    This class gathers all the data that needs to be passed around in code
    generation functions.

    Note: the section fields of this class are generated programatically.

    Attributes:
      type: the container type (can be a Class or a Namespace).
      binding_model: the binding model for the containing type.
      is_namespace: whether or not the container is a namespace.
      scope: current code generation scope, to generate properly qualified type
        references.
      header_section: the current code section in the header file.
      cpp_section: the current code section in the implementation file.
      static_prop_ids: the list of (enum_name, JS name) for properties in the
        static object for the container type.
      static_method_ids: the list of (enum_name, JS name) for methods in the
        static object for the container type.
      namespace_list: the list of inner namespace and classes of the container
        type (to generate the static object)
      prop_ids: the list of (enum_name, JS name) for properties in the
        instance object for the container type.
      method_ids: the list of (enum_name, JS name) for methods in the
        instance object for the container type.
      namespace_init_section: a section where the initialization code for the
        namespaces in the static object will go.
      namespace_create_section: a section where the globals::NPAPIObject
        creation code will go.
      namespace_register_base_section: a section where the class bases get
        registered into their corresponding globals::NPAPIObject.
      namespace_get_static_object_section: a section where the
        GetStaticNPObject functions get defined.
      static_invoke_section: a section where the Invoke implementation for the
        static object will go (for static functions).
      static_invoke_default_section: a section where the InvokeDefault
        implementation for the static object will go (for constructors).
      static_get_prop_section: a section where the GetProperty implementation
        for the static object will go (for static members, enum values, and
        inner namespaces).
      static_set_prop_section: a section where the SetProperty implementation
        for the static object will go (for static members).
      invoke_section: a section where the Invoke implementation for the
        instance object will go (for non-static methods).
      get_prop_section: a section where the GetProperty implementation for the
        instance object will go (for non-static members).
      set_prop_section: a section where the SetProperty implementation for the
        instance object will go (for non-static members).
    """

    _sections = [('namespace_init_section', 'InitNamespaceGlues'),
                 ('namespace_create_section', 'CreateNamespaces'),
                 ('namespace_register_base_section', 'RegisterBases'),
                 ('namespace_get_static_object_section', 'GetStaticObjects'),
                 ('static_invoke_section', 'StaticInvokeCode'),
                 ('static_invoke_default_section', 'StaticInvokeDefaultCode'),
                 ('static_get_prop_section', 'StaticGetPropertyCode'),
                 ('static_set_prop_section', 'StaticSetPropertyCode')]

    _class_sections = [('invoke_section', 'InvokeCode'),
                       ('get_prop_section', 'GetPropertyCode'),
                       ('set_prop_section', 'SetPropertyCode')]

    def __init__(self, type_defn, scope, header_section, cpp_section,
                 share_context):
      """Inits a CodeGenContext.

      Args:
        type_defn: the container type.
        scope: current code generation scope, to generate properly qualified
          type references.
        header_section: the current code section in the header file.
        cpp_section: the current code section in the implementation file.
        share_context: share the definition sections and the id lists with that
          context (can be None) - used when encountering namespaces defined
          previously.
      """
      self.type_defn = type_defn
      self.binding_model = type_defn.binding_model or globals_binding
      self.is_namespace = type_defn.defn_type == 'Namespace'
      self.scope = scope
      self.header_section = header_section
      self.cpp_section = cpp_section
      if self.is_namespace:
        all_sections = self._sections
      else:
        all_sections = self._sections + self._class_sections
      if share_context:
        self.static_prop_ids = share_context.static_prop_ids
        self.static_method_ids = share_context.static_method_ids
        self.namespace_list = share_context.namespace_list
        if not share_context.is_namespace:
          self.prop_ids = share_context.prop_ids
          self.method_ids = share_context.method_ids
        # programmatically copy fields
        for field_name, section_name in all_sections:
          setattr(self, field_name, getattr(share_context, field_name))
      else:
        self.static_prop_ids = []
        self.static_method_ids = []
        self.namespace_list = []
        if not self.is_namespace:
          self.prop_ids = []
          self.method_ids = []
        # programmatically create fields
        for field_name, section_name in all_sections:
          setattr(self, field_name,
                  cpp_section.CreateUnlinkedSection(section_name))
          getattr(self, field_name).needed_glue = cpp_section.needed_glue

  def GetParamInputStrings(self, scope, param_list):
    """Gets the code to retrieve parameters from an array of NPVariants.

    Args:
      scope: the code generation scope.
      param_list: a list of Function.Param.

    Returns:
      a 3-uple. The first element is a list of strings that contains the code
      to retrieve the parameter values. The second element is a list of
      expressions to access each of the parameters. The third element is the set
      of all the types whose glue header is needed.
    """
    strings = []
    param_names = []
    needed_glue = set()
    for i in range(len(param_list)):
      param = param_list[i]
      needed_glue.add(param.type_defn)
      param_binding = param.type_defn.binding_model
      start_exception, end_exception = GenExceptionContext(
          _exception_macro_name, "parameter",
          naming.Normalize(param.name, naming.Java))
      code, param_access = param_binding.NpapiFromNPVariant(
          scope, param.type_defn, 'args[%d]' % i, 'param_%s' % param.name,
          'success', _exception_macro_name, 'npp')
      strings.append(start_exception)
      strings.append(code)
      strings.append(_failure_test_string)
      strings.append(end_exception)
      param_names.append(param_access)
    return strings, param_names, needed_glue

  def GetReturnStrings(self, scope, type_defn, expression, result):
    """Gets the code to set a return value into a NPVariant.

    Args:
      scope: the code generation scope.
      type_defn: the type of the return value.
      expression: the expression that evaluates to the return value.
      result: the expression that evaluates to a pointer to the NPVariant.

    Returns:
      a 3-uple. The first element is the code to generate the
      NPVariant-compatible value (that can fail). The second element is the
      code to set the NPVariant. The third element is the set of all the types
      whose glue header is needed.
    """
    binding_model = type_defn.binding_model
    pre, post = binding_model.NpapiExprToNPVariant(scope, type_defn, 'retval',
                                                   expression, result,
                                                   'success', 'npp')
    return pre, post, set([type_defn])

  def GetVoidReturnStrings(self, type_defn, result):
    """Gets the code to return a void value for a write-only member.

    Args:
      type_defn: the type of the return value.
      result: the expression that evaluates to a pointer to the NPVariant.

    Returns:
      a 3-uple. The first element is empty, and is needed only to mimic the
      return of GetReturnStrings. The second element is the code to set the
      NPVariant. The third element is the set of all the types whose glue
      header is needed.
    """
    post = "VOID_TO_NPVARIANT(*%s);\n" % result
    return "", post, set([type_defn])

  def GenerateCppFunction(self, section, scope, function):
    """Generates a function header.

    Args:
      section: the code section to generate the function header into.
      scope: the code generation scope.
      function: the Function to generate.
    """
    prototype, unused_val = cpp_utils.GetFunctionPrototype(scope, function, '')
    section.EmitCode(prototype + ';')

  def GetUserGlueMethodFunc(self, scope, type_defn, method):
    """Creates a definition for the user glue function for a non-static method.

    Args:
      scope: the code generation scope.
      type_defn: the type of the method container.
      method: the method for which to create the user glue function.

    Returns:
      a syntax_tree.Function for the user glue function.
    """
    glue_function = syntax_tree.Function(method.source, [],
                                         'userglue_method_%s' % method.name,
                                         None, [])
    glue_function.type_defn = method.type_defn
    glue_function.parent = scope
    this_param = syntax_tree.Function.Param(type_defn.name, '_this')
    this_param.type_defn = type_defn
    this_param.mutable = True
    glue_function.params = [this_param] + method.params
    return glue_function

  def GetUserGlueStaticMethodFunc(self, scope, method):
    """Creates a definition for the user glue function for a static function.

    Args:
      scope: the code generation scope.
      method: the method for which to create the user glue function.

    Returns:
      a syntax_tree.Function for the user glue function.
    """
    glue_function = syntax_tree.Function(method.source, [],
                                         'userglue_static_%s' % method.name,
                                         None, [])
    glue_function.type_defn = method.type_defn
    glue_function.params = method.params[:]
    glue_function.parent = scope
    return glue_function

  def GetUserGlueConstructorFunc(self, scope, type_defn, method):
    """Creates a definition for the user glue function for a constructor.

    Args:
      scope: the code generation scope.
      type_defn: the type of the method container.
      method: the constructor for which to create the user glue function.

    Returns:
      a syntax_tree.Function for the user glue function.
    """
    glue_function = syntax_tree.Function(method.source, [],
                                         'userglue_construct_%s' % method.name,
                                         None, [])
    glue_function.type_defn = type_defn
    glue_function.params = method.params[:]
    glue_function.parent = scope
    return glue_function

  def GetUserGlueSetterFunc(self, scope, type_defn, field):
    """Creates a definition for the user glue function for a setter method.

    For a field name 'myField' of type "FieldType" in an object of type
    'ObjectType', this creates a function like:
    void userglue_setter_myField(ObjectType _this, FieldType param_myField)

    Args:
      scope: the code generation scope.
      type_defn: the type of the method container.
      method: the method for which to create the user glue function.

    Returns:
      a syntax_tree.Function for the user glue function.
    """
    glue_function = syntax_tree.Function(field.source, [],
                                         'userglue_setter_%s' % field.name,
                                         None, [])
    glue_function.type_defn = scope.LookUpTypeRecursive('void')
    glue_function.parent = scope
    this_param = syntax_tree.Function.Param(type_defn.name, '_this')
    this_param.type_defn = type_defn
    this_param.mutable = True
    value_param = syntax_tree.Function.Param(type_defn.name, 'param_' +
                                             field.name)
    value_param.type_defn = field.type_defn
    glue_function.params = [this_param, value_param]
    return glue_function

  def GetUserGlueGetterFunc(self, scope, type_defn, field):
    """Creates a definition for the user glue function for a getter method.

    For a field name 'myField' of type "FieldType" in an object of type
    'ObjectType', this creates a function like:
    FieldType userglue_getter_myField(ObjectType _this)

    Args:
      scope: the code generation scope.
      type_defn: the type of the method container.
      method: the method for which to create the user glue function.

    Returns:
      a syntax_tree.Function for the user glue function.
    """
    glue_function = syntax_tree.Function(field.source, [],
                                         'userglue_getter_%s' % field.name,
                                         None, [])
    glue_function.type_defn = field.type_defn
    glue_function.parent = scope
    this_param = syntax_tree.Function.Param(type_defn.name, '_this')
    this_param.type_defn = type_defn
    this_param.mutable = True
    glue_function.params = [this_param]
    return glue_function

  def AddPluginDataParam(self, scope, func, param_exprs):
    """Adds the plugin data parameter to a function and parameter list.

    Args:
      scope: the scope for the parameter type.
      func: the function to which we add a parameter.
      param_exprs: the list of parameters exressions to which the plugin data
        is added
    """
    scope = scope  # silence gpylint
    plugin_data_param = syntax_tree.Function.Param(
        self._plugin_data_type.name, 'plugin_data')
    plugin_data_param.type_defn = self._plugin_data_type
    func.params.insert(0, plugin_data_param)
    param_exprs.insert(0, 'npp->pdata')

  def EmitMemberCall(self, context, func):
    """Emits the glue for a non-static member function call.

    Args:
      context: the code generation context.
      func: the method to call.
    """
    scope = context.scope
    type_defn = context.type_defn
    binding_model = context.binding_model
    section = context.invoke_section
    id_enum = 'METHOD_%s' % naming.Normalize(func.name, naming.Upper)
    name = '"%s"' % naming.Normalize(func.name, naming.Java)
    context.method_ids.append((id_enum, name))
    strings, param_exprs, needed_glue = self.GetParamInputStrings(scope,
                                                                  func.params)
    section.needed_glue.update(needed_glue)
    if 'userglue' in func.attributes:
      glue_func = self.GetUserGlueMethodFunc(scope, type_defn, func)
      param_exprs.insert(0, 'object')
      if 'plugin_data' in func.attributes:
        self.AddPluginDataParam(context.scope, glue_func, param_exprs)
      self.GenerateCppFunction(context.header_section, scope, glue_func)
      expression = globals_binding.CppCallStaticMethod(scope, scope, glue_func,
                                                       param_exprs)
    else:
      expression = binding_model.CppCallMethod(scope, type_defn, 'object',
                                               True, func, param_exprs)
    pre, post, needed_glue = self.GetReturnStrings(scope, func.type_defn,
                                                   expression, 'result')
    section.needed_glue.update(needed_glue)
    strings += [pre, _failure_test_string, post, 'return true;']
    self.EmitInvokeCode(section, 'method_ids', id_enum, len(func.params),
                        '\n'.join(strings))

  def EmitStaticCall(self, context, func):
    """Emits the glue for a static function call.

    Args:
      context: the code generation context.
      func: the function to call.
    """
    scope = context.scope
    type_defn = context.type_defn
    binding_model = context.binding_model
    section = context.static_invoke_section
    id_enum = 'STATIC_METHOD_%s' % naming.Normalize(func.name, naming.Upper)
    name = '"%s"' % naming.Normalize(func.name, naming.Java)
    context.static_method_ids.append((id_enum, name))

    strings, param_exprs, needed_glue = self.GetParamInputStrings(scope,
                                                                  func.params)
    section.needed_glue.update(needed_glue)
    if 'userglue' in func.attributes:
      glue_func = self.GetUserGlueStaticMethodFunc(scope, func)
      if 'plugin_data' in func.attributes:
        self.AddPluginDataParam(context.scope, glue_func, param_exprs)
      self.GenerateCppFunction(context.header_section, scope, glue_func)
      expression = globals_binding.CppCallStaticMethod(scope, scope, glue_func,
                                                       param_exprs)
    else:
      expression = binding_model.CppCallStaticMethod(scope, type_defn, func,
                                                     param_exprs)
    pre, post, needed_glue = self.GetReturnStrings(scope, func.type_defn,
                                                   expression, 'result')
    section.needed_glue.update(needed_glue)
    strings += [pre, _failure_test_string, post, 'return true;']
    self.EmitInvokeCode(section, 'static_method_ids', id_enum,
                        len(func.params), '\n'.join(strings))

  def EmitConstructorCall(self, context, func):
    """Emits the glue for a constructor call.

    Args:
      context: the code generation context.
      func: the constructor to call.
    """
    scope = context.scope
    type_defn = context.type_defn
    binding_model = context.binding_model
    section = context.static_invoke_default_section
    strings, param_exprs, needed_glue = self.GetParamInputStrings(scope,
                                                                  func.params)
    section.needed_glue.update(needed_glue)
    if 'userglue' in func.attributes:
      glue_func = self.GetUserGlueConstructorFunc(scope, type_defn, func)
      if 'plugin_data' in func.attributes:
        self.AddPluginDataParam(context.scope, glue_func, param_exprs)
      self.GenerateCppFunction(context.header_section, scope, glue_func)
      expression = globals_binding.CppCallStaticMethod(scope, scope, glue_func,
                                                       param_exprs)
    else:
      expression = binding_model.CppCallConstructor(scope, type_defn, func,
                                                    param_exprs)
    pre, post, needed_glue = self.GetReturnStrings(scope, type_defn, expression,
                                                   'result')
    section.needed_glue.update(needed_glue)
    strings += [pre, _failure_test_string, post, 'return true;']
    self.EmitInvokeDefaultCode(section, len(func.params), '\n'.join(strings))

  def EmitMemberProp(self, context, field):
    """Emits the glue for a non-static member field access.

    Args:
      context: the code generation context.
      field: the field to access.
    """
    scope = context.scope
    type_defn = context.type_defn
    binding_model = context.binding_model
    id_enum = 'PROPERTY_%s' % naming.Normalize(field.name, naming.Upper)
    prop_name = '"%s"' % naming.Normalize(field.name, naming.Java)
    context.prop_ids.append((id_enum, prop_name))
    if 'getter' in field.attributes:
      if 'userglue_getter' in field.attributes:
        glue_func = self.GetUserGlueGetterFunc(scope, type_defn, field)
        param_exprs = ['object']
        if 'plugin_data' in field.attributes:
          self.AddPluginDataParam(scope, glue_func, param_exprs)
        self.GenerateCppFunction(context.header_section, scope, glue_func)
        expression = globals_binding.CppCallStaticMethod(scope, scope,
                                                         glue_func, param_exprs)
      else:
        expression = binding_model.CppGetField(scope, type_defn, 'object',
                                               field)
      pre, post, needed_glue = self.GetReturnStrings(scope, field.type_defn,
                                                     expression, 'variant')
    else:
      # Return a void value for write-only members.
      pre, post, needed_glue = self.GetVoidReturnStrings(field.type_defn,
                                                         'variant')

    section = context.get_prop_section
    section.needed_glue.update(needed_glue)
    get_string = '\n'.join([pre, _failure_test_string, post, 'return true;'])
    self.EmitPropertyCode(section, 'property_ids', id_enum, get_string)

    if 'setter' in field.attributes:
      # TODO: Add a specific error for trying to set a read-only prop.
      field_binding = field.type_defn.binding_model
      start_exception, end_exception = GenExceptionContext(
          _exception_macro_name, "field",
          naming.Normalize(field.name, naming.Java))
      code, param_expr = field_binding.NpapiFromNPVariant(
          scope, field.type_defn, '(*variant)', 'param_%s' % field.name,
          'success', _exception_macro_name, 'npp')
      section = context.set_prop_section
      section.needed_glue.add(field.type_defn)
      if 'userglue_setter' in field.attributes:
        glue_func = self.GetUserGlueSetterFunc(scope, type_defn, field)
        param_exprs = ['object', param_expr]
        if 'plugin_data' in field.attributes:
          self.AddPluginDataParam(scope, glue_func, param_exprs)
        self.GenerateCppFunction(context.header_section, scope, glue_func)
        expression = globals_binding.CppCallStaticMethod(scope, scope,
                                                         glue_func, param_exprs)
      else:
        expression = binding_model.CppSetField(scope, type_defn, 'object',
                                               field, param_expr)
      strings = [start_exception, code, _failure_test_string,
          '%s;' % expression, 'return true;', end_exception]
      self.EmitPropertyCode(section, 'property_ids', id_enum,
                            '\n'.join(strings))

  def EmitStaticMemberProp(self, context, field):
    """Emits the glue for a static field access.

    Args:
      context: the code generation context.
      field: the field to access.
    """
    scope = context.scope
    type_defn = context.type_defn
    binding_model = context.binding_model
    id_enum = 'STATIC_PROPERTY_%s' % naming.Normalize(field.name, naming.Upper)
    prop_name = '"%s"' % naming.Normalize(field.name, naming.Java)
    context.static_prop_ids.append((id_enum, prop_name))
    if 'getter' in field.attributes:
      expression = binding_model.CppGetStatic(scope, type_defn, field)
      pre, post, needed_glue = self.GetReturnStrings(scope, field.type_defn,
                                                     expression, 'variant')
    else:
      # Return a void value for write-only members.
      pre, post, needed_glue = self.GetVoidReturnStrings(field.type_defn,
                                                         'variant')
    section = context.static_get_prop_section
    section.needed_glue.update(needed_glue)
    get_string = '\n'.join([pre, _failure_test_string, post, 'return true;'])
    self.EmitPropertyCode(section, 'static_property_ids', id_enum,
                          get_string)

    if 'setter' in field.attributes:
      # TODO: Add a specific error for trying to set a read-only prop.
      field_binding = field.type_defn.binding_model
      start_exception, end_exception = GenExceptionContext(
          _exception_macro_name, "field",
          naming.Normalize(field.name, naming.Java))
      code, param_expr = field_binding.NpapiFromNPVariant(
          scope, field.type_defn, '(*variant)', 'param_%s' % field.name,
          'success', _exception_macro_name, 'npp')
      section = context.static_set_prop_section
      section.needed_glue.add(field.type_defn)
      expression = binding_model.CppSetStatic(scope, type_defn, field,
                                              param_expr)
      strings = [start_exception, code, _failure_test_string,
          '%s;' % expression, 'return true;', end_exception]
      self.EmitPropertyCode(section, 'static_property_ids', id_enum,
                            '\n'.join(strings))

  def EmitEnumValue(self, context, enum, enum_value):
    """Emits the glue for an enum value access.

    Args:
      context: the code generation context.
      enum: the enum definition.
      enum_value: the enum value to access.
    """
    enum = enum  # silence gpylint.
    scope = context.scope
    type_defn = context.type_defn
    section = context.static_get_prop_section
    name = naming.Normalize(enum_value.name, naming.Upper)
    id_enum = 'ENUM_%s' % name
    prop_name = '"%s"' % name
    context.static_prop_ids.append((id_enum, prop_name))
    strings = ['INT32_TO_NPVARIANT(%s::%s, *variant);' %
               (cpp_utils.GetScopedName(scope, type_defn), enum_value.name),
               'return true;']
    self.EmitPropertyCode(section, 'static_property_ids', id_enum,
                          '\n'.join(strings))

  def EmitInvokeCode(self, section, table, id_enum, arg_count, code):
    """Emits glue code in an 'Invoke' dispatch function.

    Args:
      section: the code section of the dispatch function.
      table: the table in which the method identifier is defined.
      id_enum: the method identifier enum.
      arg_count: the number of arguments for the function.
      code: the glue code.
    """
    section.EmitCode(_method_invoke_template.substitute(table=table,
                                                        method_id=id_enum,
                                                        argCount=arg_count,
                                                        code=code))

  def EmitInvokeDefaultCode(self, section, arg_count, code):
    """Emits glue code in an 'InvokeDefault' dispatch function.

    Args:
      section: the code section of the dispatch function.
      arg_count: the number of arguments for the function.
      code: the glue code.
    """
    section.EmitCode(_method_default_invoke_template.substitute(
        argCount=arg_count, code=code))

  def EmitPropertyCode(self, section, table, id_enum, code):
    """Emits glue code in a 'GetProperty' or 'SetProperty' dispatch function.

    Args:
      section: the code section of the dispatch function.
      table: the table in which the property identifier is defined.
      id_enum: the property identifier enum.
      code: the glue code.
    """
    section.EmitCode(_property_template.substitute(table=table,
                                                   property_id=id_enum,
                                                   code=code))

  def Variable(self, context, obj):
    """Emits the glue code for a Variable definition.

    Args:
      context: the code generation context.
      obj: the Variable definition.
    """
    if 'private' in obj.attributes or 'protected' in obj.attributes:
      return
    if 'static' in obj.attributes or context.is_namespace:
      self.EmitStaticMemberProp(context, obj)
    else:
      self.EmitMemberProp(context, obj)

  def Enum(self, context, obj):
    """Emits the glue code for an Enum definition.

    Args:
      context: the code generation context.
      obj: the Enum definition.
    """
    if 'private' in obj.attributes or 'protected' in obj.attributes:
      return
    for value in obj.values:
      self.EmitEnumValue(context, obj, value)

  def Function(self, context, obj):
    """Emits the glue code for a Function definition.

    Args:
      context: the code generation context.
      obj: the Function definition.

    Raises:
      MethodWithoutReturnType: a non-constructor function doesn't have a return
        type.
    """
    if 'private' in obj.attributes or 'protected' in obj.attributes:
      return
    if 'static' in obj.attributes or context.is_namespace:
      self.EmitStaticCall(context, obj)
    else:
      if not obj.type_defn:
        if obj.name == context.type_defn.name:
          # constructor
          self.EmitConstructorCall(context, obj)
        elif obj.name == '~' + context.type_defn.name:
          # destructor (ignore)
          return
        else:
          # method without return type: error
          raise MethodWithoutReturnType(obj)
      else:
        self.EmitMemberCall(context, obj)

  def Callback(self, context, obj):
    """Emits the glue code for a Callback definition.

    Args:
      context: the code generation context.
      obj: the Callback definition.
    """
    if 'private' in obj.attributes or 'protected' in obj.attributes:
      return

    binding_model = obj.binding_model
    namespace_name = npapi_utils.GetGlueNamespace(obj)

    scope = syntax_tree.Namespace(None, [], namespace_name, [])
    scope.parent = context.scope

    context.header_section.PushNamespace(namespace_name)
    header_section = context.header_section.CreateSection(namespace_name)
    header_section.needed_defn = context.header_section.needed_defn
    context.header_section.PopNamespace()

    context.cpp_section.PushNamespace(namespace_name)
    cpp_section = context.cpp_section.CreateSection(namespace_name)
    cpp_section.needed_glue = context.cpp_section.needed_glue
    context.cpp_section.PopNamespace()

    param_to_variant_pre = []
    param_to_variant_post = []
    param_strings = []
    for i in xrange(len(obj.params)):
      p = obj.params[i]
      param_string, unused_val = cpp_utils.GetFunctionParamPrototype(scope, p)
      header_section.needed_defn.add(p.type_defn)
      cpp_section.needed_glue.add(p.type_defn)
      param_strings += [param_string]
      bm = p.type_defn.binding_model
      pre, post = bm.NpapiExprToNPVariant(scope, p.type_defn, 'var_' + p.name,
                                          p.name, '(args + %d)' % i, 'success',
                                          'npp')
      param_to_variant_pre.append(pre)
      param_to_variant_post.append(post)

    if param_strings:
      param_strings = [''] + param_strings

    return_type = obj.type_defn
    header_section.needed_defn.add(return_type)
    cpp_section.needed_glue.add(return_type)
    bm = return_type.binding_model
    return_type_string, unused_val = bm.CppReturnValueString(scope,
                                                             return_type)
    run_callback = ('%s RunCallback(NPP npp, NPObject *npobject, bool async%s)'
                    % (return_type_string, ', '.join(param_strings)))

    return_eval, return_value = bm.NpapiFromNPVariant(scope, return_type,
                                                      'result', 'retval',
                                                      'success',
                                                      _exception_macro_name,
                                                      'npp')
    start_exception, end_exception = GenExceptionContext(
        _exception_macro_name, "callback return value", "<no name>")
    subst_dict = {'RunCallback': run_callback,
                  'ArgCount': str(len(obj.params)),
                  'ParamsToVariantsPre': '\n'.join(param_to_variant_pre),
                  'ParamsToVariantsPost': '\n'.join(param_to_variant_post),
                  'ReturnEval': return_eval,
                  'ReturnValue': return_value,
                  'StartException': start_exception,
                  'EndException': end_exception}
    if obj.params:
      glue_template = _callback_glue_cpp_template
    else:
      glue_template = _callback_no_param_glue_cpp_template
    cpp_section.EmitCode(glue_template.substitute(subst_dict))
    cpp_section.EmitCode(binding_model.NpapiBindingGlueCpp(scope, obj))
    header_section.EmitCode(binding_model.NpapiBindingGlueHeader(scope, obj))

  def GetDictForEnumerations(self, context, has_base):
    """Creates a dictionary used to fill in the gaps in the property
    enumeration functions.  Note that this dictionary will in some cases cause
    the insertion of the string ${BaseClassNamespace}, so it must be used
    *before* the dictionary that fills in that macro.  This only happens when
    the context is a derived class.

    Args:
      context: the code generation context
      has_base: whether this is a class that has a base class

    Returns:
      a dictionary containing definitions for the code generation templates
    """
    dict = {}
    if not context.is_namespace:
      if context.prop_ids:
        dict.update({
          'PropertyCount': 'NUM_PROPERTY_IDS',
          'EnumeratePropertyEntries': _enumerate_property_entries,
        })
      else:
        dict.update({
          'PropertyCount': '0',
          'EnumeratePropertyEntries': '',
        })
      if context.method_ids:
        dict.update({
          'MethodCount': 'NUM_METHOD_IDS',
          'EnumerateMethodEntries': _enumerate_method_entries,
        })
      else:
        dict.update({
          'MethodCount': '0',
          'EnumerateMethodEntries': '',
        })
      if has_base:
        dict.update({
          'BaseGetPropertyCount': '${BaseClassNamespace}::GetPropertyCount()',
          'EnumeratePropertyEntriesHelperBaseCall':
              """${BaseClassNamespace}::EnumeratePropertyEntriesHelper(
                      output);\n""",
        })
      else:
        dict.update({
          'BaseGetPropertyCount': '0',
          'EnumeratePropertyEntriesHelperBaseCall': '',
        })

    if context.static_prop_ids:
      dict.update({
        'AddStaticPropertyCount': '*count += NUM_STATIC_PROPERTY_IDS;',
        'EnumerateStaticPropertyEntries': _enumerate_static_property_entries,
        'StaticPropertyCount': 'NUM_STATIC_PROPERTY_IDS',
      })
    else:
      dict.update({
        'AddStaticPropertyCount': '',
        'EnumerateStaticPropertyEntries': '',
        'StaticPropertyCount': '0',
      })
    if context.static_method_ids:
      dict.update({
        'AddStaticMethodCount': '*count += NUM_STATIC_METHOD_IDS;',
        'EnumerateStaticMethodEntries': _enumerate_static_method_entries,
        'StaticMethodCount': 'NUM_STATIC_METHOD_IDS',
      })
    else:
      dict.update({
        'AddStaticMethodCount': '',
        'EnumerateStaticMethodEntries': '',
        'StaticMethodCount': '0',
      })
    if context.namespace_list:
      dict.update({
        'AddNamespaceCount': '*count += NUM_NAMESPACE_IDS;',
        'EnumerateNamespaceEntries': _enumerate_namespace_entries,
        'NamespaceCount': 'NUM_NAMESPACE_IDS',
      })
    else :
      dict.update({
        'AddNamespaceCount': '',
        'EnumerateNamespaceEntries': '',
        'NamespaceCount': '0',
      })
    return dict

  def Class(self, parent_context, obj):
    """Emits the glue code for a Class definition.

    Args:
      parent_context: the code generation context.
      obj: the Class definition.
    """
    if 'private' in obj.attributes or 'protected' in obj.attributes:
      return

    binding_model = obj.binding_model

    namespace_name = npapi_utils.GetGlueNamespace(obj)
    parent_context.namespace_list.append(obj)

    scope = syntax_tree.Namespace(None, [], namespace_name, [])
    scope.parent = parent_context.scope

    parent_context.header_section.PushNamespace(namespace_name)
    header_section = parent_context.header_section.CreateSection(namespace_name)
    header_section.needed_defn = parent_context.header_section.needed_defn
    parent_context.header_section.PopNamespace()

    parent_context.cpp_section.PushNamespace(namespace_name)
    cpp_section = parent_context.cpp_section.CreateSection(namespace_name)
    cpp_section.needed_glue = parent_context.cpp_section.needed_glue
    parent_context.cpp_section.PopNamespace()

    context = self.CodeGenContext(obj, scope, header_section, cpp_section, None)
    header_section.needed_defn.add(obj)
    cpp_section.needed_glue.add(obj)

    self.GenerateList(context, obj.defn_list)

    class_name_list = naming.SplitWords(obj.name)
    class_capitalized = naming.Capitalized(class_name_list)
    class_param_type, unused_need_defn = binding_model.CppParameterString(scope,
                                                                          obj)
    class_mutable_param_type, unused_need_defn = (
        binding_model.CppMutableParameterString(scope, obj))
    binding_glue_header = binding_model.NpapiBindingGlueHeader(scope, obj)
    binding_glue_cpp = binding_model.NpapiBindingGlueCpp(scope, obj)
    function_header, object_access = binding_model.NpapiDispatchFunctionHeader(
        scope, obj, 'object', 'npp', 'success')
    object_non_mutable = binding_model.CppMutableToNonMutable(scope, obj,
                                                              object_access)
    static_dict = {'Class': class_capitalized,
                   'ClassParamType': class_param_type,
                   'ClassMutableParamType': class_mutable_param_type,
                   'Object': object_access,
                   'ObjectNonMutable': object_non_mutable,
                   'BindingGlueCpp': binding_glue_cpp,
                   'BindingGlueHeader': binding_glue_header,
                   'DispatchFunctionHeader': function_header}

    enum_dict = self.GetDictForEnumerations(context, obj.base_type)
    if obj.base_type:
      parent_context.cpp_section.needed_glue.add(obj.base_type)
      static_dict['BaseClassNamespace'] = npapi_utils.GetGlueFullNamespace(
          obj.base_type.GetFinalType())
      cpp_template = _class_glue_cpp_base_template.safe_substitute(enum_dict)
    else:
      cpp_template = _class_glue_cpp_no_base_template.safe_substitute(enum_dict)

    cpp_template = string.Template(cpp_template).safe_substitute(static_dict)

    header_section.EmitCode(
        _class_glue_header_template.safe_substitute(static_dict))

    namespace_id_dict = GenNamespaceCode(context)
    parent_context.cpp_section.needed_glue.update(context.namespace_list)
    substitution_dict = {}
    substitution_dict.update(npapi_utils.MakeIdTableDict(
        context.method_ids, 'method'))
    substitution_dict.update(npapi_utils.MakeIdTableDict(
        context.static_method_ids, 'static_method'))
    substitution_dict.update(npapi_utils.MakeIdTableDict(
        context.prop_ids, 'property'))
    substitution_dict.update(npapi_utils.MakeIdTableDict(
        context.static_prop_ids, 'static_property'))
    substitution_dict.update(namespace_id_dict)

    cpp_section.EmitTemplate(string.Template(cpp_template).safe_substitute(
        substitution_dict))

  def Verbatim(self, context, obj):
    """Emits the glue code for a Verbatim definition.

    Args:
      context: the code generation context.
      obj: the Verbatim definition.
    """
    if 'verbatim' in obj.attributes:
      if obj.attributes['verbatim'] == 'cpp_glue':
        context.cpp_section.EmitCode(obj.text)
      elif obj.attributes['verbatim'] == 'header_glue':
        context.header_section.EmitCode(obj.text)

  def Namespace(self, parent_context, obj):
    """Emits the glue code for a Namespace definition.

    Since a namespace can be defined through several Namespace definitions,
    this function doesn't generate all the glue for the namespace until all the
    namespaces definitions have been processed (second pass).

    Args:
      parent_context: the code generation context.
      obj: the Namespace definition.
    """
    namespace_name = npapi_utils.GetGlueNamespace(obj)
    # namespaces that span across multiple files are different objects
    # we keep definitions inside the namespace separate, but all the 'static'
    # glue needs to be gathered. So we create a code generation context that
    # will be re-used when a different part of the namespace will be
    # encountered
    # all the different namespaces share the same scope member, so use that as
    # a key into a dict that maps the context
    if obj.scope in self._namespace_map:
      old_context = self._namespace_map[obj.scope]

      parent_context.header_section.PushNamespace(namespace_name)
      header_section = parent_context.header_section.CreateSection(
          namespace_name)
      header_section.needed_defn = parent_context.header_section.needed_defn
      parent_context.header_section.PopNamespace()

      parent_context.cpp_section.PushNamespace(namespace_name)
      cpp_section = parent_context.cpp_section.CreateSection(namespace_name)
      cpp_section.needed_glue = parent_context.cpp_section.needed_glue
      parent_context.cpp_section.PopNamespace()

      context = self.CodeGenContext(old_context.type_defn, old_context.scope,
                                    header_section, cpp_section, old_context)
    else:
      parent_context.namespace_list.append(obj)

      scope = syntax_tree.Namespace(None, [], namespace_name, [])
      scope.parent = parent_context.scope

      parent_context.header_section.PushNamespace(namespace_name)
      header_section = parent_context.header_section.CreateSection(
          namespace_name)
      header_section.needed_defn = parent_context.header_section.needed_defn
      parent_context.header_section.PopNamespace()

      parent_context.cpp_section.PushNamespace(namespace_name)
      cpp_section = parent_context.cpp_section.CreateSection(namespace_name)
      cpp_section.needed_glue = parent_context.cpp_section.needed_glue
      parent_context.cpp_section.PopNamespace()

      context = self.CodeGenContext(obj, scope, header_section,
                                    cpp_section, None)
      self._namespace_map[obj.scope] = context

      def _Finalize():
        # This part can only be finalized after all files have been processed,
        # because later files can still add definitions to the namespace.
        # So do this work in a function that will get called at the end.
        namespace_id_dict = GenNamespaceCode(context)
        parent_context.cpp_section.needed_glue.update(context.namespace_list)

        substitution_dict = {}
        substitution_dict.update(npapi_utils.MakeIdTableDict(
            context.static_method_ids, 'static_method'))
        substitution_dict.update(npapi_utils.MakeIdTableDict(
            context.static_prop_ids, 'static_property'))
        substitution_dict.update(namespace_id_dict)

        header_section.EmitCode(_namespace_glue_header)

        enum_dict = self.GetDictForEnumerations(context, False)
        temp_string = _namespace_glue_cpp_template.safe_substitute(enum_dict)
        temp_template = string.Template(temp_string)

        cpp_section.EmitTemplate(temp_template.safe_substitute(
            substitution_dict))

      self._finalize_functions.append(_Finalize)

    context.cpp_section.needed_glue.add(obj)
    self.GenerateList(context, obj.defn_list)

  def Typedef(self, context, obj):
    """Emits the glue code for a Typedef definition.

    Args:
      context: the code generation context.
      obj: the Typedef definition.
    """
    # TODO: implement this.
    pass

  def Typename(self, context, obj):
    """Emits the glue code for a Typename definition.

    Typename being unknown types, no glue is generated for them.

    Args:
      context: the code generation context.
      obj: the Typename definition.
    """
    pass

  def GenerateList(self, context, defn_list):
    """Emits the glue code for a list of definitions.

    Args:
      context: the code generation context.
      defn_list: the definition list.
    """
    for obj in defn_list:
      if 'nojs' in obj.attributes:
        continue
      if 'include' in obj.attributes:
        context.header_section.needed_defn.add(obj)
      func = getattr(self, obj.defn_type)
      func(context, obj)

  def CreateGlueWriters(self, idl_file):
    """Creates CppFileWriter instances for glue header and implementation.

    Args:
      idl_file: an idl_parser.File for the source file.

    Returns:
      a pair of CppFileWriter, the first being the glue header writer, the
      second one being the glue implementation writer.
    """
    cpp_writer = cpp_utils.CppFileWriter(
        '%s/%s' % (self._output_dir, GetGlueCpp(idl_file)), False)
    for include, system in _cpp_includes:
      cpp_writer.AddInclude(include, system)
    cpp_writer.AddInclude(GetGlueHeader(idl_file), False)

    header_writer = cpp_utils.CppFileWriter(
        '%s/%s' % (self._output_dir, GetGlueHeader(idl_file)), True)
    for include, system in _header_includes:
      header_writer.AddInclude(include, system)
    return header_writer, cpp_writer

  def CreateGlueSection(self, writer):
    """Utility function to create a 'glue' section in a writer.

    This function will create a new section inside a 'glue' namespace.

    Args:
      writer: a CppFileWriter in which to create the section.

    Returns:
      the created section.
    """
    writer.PushNamespace('glue')
    section = writer.CreateSection('glue')
    writer.PopNamespace()
    return section

  def BeginFile(self, idl_file, parent_context, defn_list):
    """Runs the pass 1 generation for an IDL file.

    Args:
      idl_file: the source IDL file.
      parent_context: the code generation context.
      defn_list: the list of top-level definitions in the IDL file.

    Returns:
      a 3-uple. The first element is the code generation context for that file.
      The second element is the glue header writer. The third element is the
      glue implementation writer.
    """
    header_writer, cpp_writer = self.CreateGlueWriters(idl_file)
    header_writer.needed_defn = set()
    cpp_writer.needed_glue = set()
    header_section = self.CreateGlueSection(header_writer)
    cpp_section = self.CreateGlueSection(cpp_writer)
    header_section.needed_defn = header_writer.needed_defn
    cpp_section.needed_glue = cpp_writer.needed_glue

    context = self.CodeGenContext(parent_context.type_defn,
                                  parent_context.scope, header_section,
                                  cpp_section, parent_context)

    self.GenerateList(context, defn_list)
    return context, header_writer, cpp_writer

  def FinishFile(self, idl_file, context, header_writer, cpp_writer):
    """Runs the pass 2 generation for an IDL file.

    Args:
      idl_file: the source IDL file.
      context: the code generation context for this file (returned by
        BeginFile)
      header_writer: the glue header writer (returned by BeginFile).
      cpp_writer: the glue implementation writer (returned by BeginFile).

    Returns:
      a list of CppFileWriter instances that contain the generated files.
    """
    context = context  # silence gpylint
    source_files = (type_defn.GetFinalType().source.file for type_defn in
                    cpp_writer.needed_glue)
    cpp_needed_glue_includes = set(GetGlueHeader(source_file) for source_file
                                   in source_files)
    cpp_needed_glue_includes.add(GetGlueHeader(idl_file))

    for include_file in cpp_needed_glue_includes:
      if include_file:
        cpp_writer.AddInclude(include_file)

    for include_file in set(type_defn.GetDefinitionInclude() for type_defn
                            in header_writer.needed_defn):
      if include_file:
        header_writer.AddInclude(include_file)

    return [header_writer, cpp_writer]

  def BeginGlobals(self, idl_file, namespace):
    """Runs the pass 1 generation for the global namespace.

    A separate files are written containing the global namespace glue.

    Args:
      idl_file: an idl_file.File for the global namespace file.
      namespace: the global namespace.

    Returns:
      a 3-uple. The first element is the code generation context for that file.
      The second element is the glue header writer. The third element is the
      glue implementation writer.
    """
    scope = syntax_tree.Namespace(None, [], 'glue', [])
    scope.parent = namespace

    header_writer, cpp_writer = self.CreateGlueWriters(idl_file)
    header_writer.needed_defn = set()
    cpp_writer.needed_glue = set()
    header_section = self.CreateGlueSection(header_writer)
    cpp_section = self.CreateGlueSection(cpp_writer)
    header_section.needed_defn = header_writer.needed_defn
    cpp_section.needed_glue = cpp_writer.needed_glue

    context = self.CodeGenContext(namespace, scope, header_section,
                                  cpp_section, None)
    return context, header_writer, cpp_writer

  def FinishGlobals(self, context, header_writer, cpp_writer):
    """Runs the pass 2 generation for the global namespace.

    Args:
      context: the code generation context for the global namespace (returned
        by BeginGlobals).
      header_writer: the glue header writer (returned by BeginGlobals).
      cpp_writer: the glue implementation writer (returned by BeginGlobals).

    Returns:
      a list of CppFileWriter instances that contain the generated files.
    """
    for f in self._finalize_functions:
      f()
    namespace_id_dict = GenNamespaceCode(context)

    substitution_dict = {}
    substitution_dict.update(npapi_utils.MakeIdTableDict(
        context.static_method_ids, 'static_method'))
    substitution_dict.update(npapi_utils.MakeIdTableDict(
        context.static_prop_ids, 'static_property'))
    substitution_dict.update(namespace_id_dict)

    context.header_section.EmitCode(_namespace_glue_header)

    enum_dict = self.GetDictForEnumerations(context, False)
    temp_string = _namespace_glue_cpp_template.safe_substitute(enum_dict)
    temp_template = string.Template(temp_string)

    context.cpp_section.EmitTemplate(
        temp_template.safe_substitute(substitution_dict))

    context.header_section.EmitCode(_globals_glue_header_tail)
    context.cpp_section.EmitCode(_globals_glue_cpp_tail)

    includes = set(GetGlueHeader(ns_obj.source.file) for ns_obj in
                   context.namespace_list)

    for include_file in includes:
      if include_file is not None:
        cpp_writer.AddInclude(include_file)

    return [header_writer, cpp_writer]


def ProcessFiles(output_dir, pairs, namespace):
  """Generates the NPAPI glue for all input files.

  Args:
    output_dir: the output directory.
    pairs: a list of (idl_parser.File, syntax_tree.Definition list) describing
      the list of top-level definitions in each source file.
    namespace: a syntax_tree.Namespace for the global namespace.

  Returns:
    a list of cpp_utils.CppFileWriter, one for each output glue header or
    implementation file.
  """
  globals_file = idl_parser.File('<internal>')
  globals_file.header = None
  globals_file.basename = 'globals'
  generator = NpapiGenerator(output_dir)

  # pass 1
  global_context, global_header_writer, global_cpp_writer = (
      generator.BeginGlobals(globals_file, namespace))
  file_map = {}
  for (idl_file, defn) in pairs:
    context, header_writer, cpp_writer = generator.BeginFile(
        idl_file, global_context, defn)
    file_map[idl_file] = (context, header_writer, cpp_writer)

  # pass 2
  writer_list = generator.FinishGlobals(global_context, global_header_writer,
                                        global_cpp_writer)
  for (idl_file, defn) in pairs:
    context, header_writer, cpp_writer = file_map[idl_file]
    writer_list += generator.FinishFile(idl_file, context, header_writer,
                                        cpp_writer)
  return writer_list


def main():
  pass

if __name__ == '__main__':
  main()
