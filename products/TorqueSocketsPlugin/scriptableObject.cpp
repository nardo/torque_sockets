#include "scriptableObject.h"

void Deallocate(NPObject* object) {
  delete static_cast<BaseObject*>(object);
}

void Invalidate(NPObject* object) {
  return static_cast<BaseObject*>(object)->Invalidate();
}

bool HasMethod(NPObject* object, NPIdentifier name) {
   fprintf(stderr, "[HelloWorld] - HasMethod\n");
   fflush(stderr);
  return static_cast<BaseObject*>(object)->HasMethod(name);
}

bool Invoke(NPObject* object, NPIdentifier name,
            const NPVariant* args, uint32_t arg_count,
            NPVariant* result) {
   fprintf(stderr, "[HelloWorld] - Invoke\n");
   fflush(stderr);
  return static_cast<BaseObject*>(object)->Invoke(
      name, args, arg_count, result);
}

bool InvokeDefault(NPObject* object, const NPVariant* args, uint32_t arg_count,
                   NPVariant* result) {
  return static_cast<BaseObject*>(object)->InvokeDefault(
      args, arg_count, result);
}

bool HasProperty(NPObject* object, NPIdentifier name) {
  return static_cast<BaseObject*>(object)->HasProperty(name);
}

bool GetProperty(NPObject* object, NPIdentifier name, NPVariant* result) {
  return static_cast<BaseObject*>(object)->GetProperty(name, result);
}

bool SetProperty(NPObject* object, NPIdentifier name, const NPVariant* value) {
  return static_cast<BaseObject*>(object)->SetProperty(name, value);
}

bool RemoveProperty(NPObject* object, NPIdentifier name) {
  return static_cast<BaseObject*>(object)->RemoveProperty(name);
}
