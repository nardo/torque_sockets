#ifndef SCRIPTABLE_OBJECT_H_
#define SCRIPTABLE_OBJECT_H_

#include "npapi.h"
#include "npupp.h"

#include <map>

extern void Deallocate(NPObject* object);
extern void Invalidate(NPObject* object);
extern bool HasMethod(NPObject* object, NPIdentifier name);
extern bool Invoke(NPObject* object, NPIdentifier name, const NPVariant* args, uint32_t arg_count, NPVariant* result);
extern bool InvokeDefault(NPObject* object, const NPVariant* args, uint32_t arg_count, NPVariant* result);
extern bool HasProperty(NPObject* object, NPIdentifier name);
extern bool GetProperty(NPObject* object, NPIdentifier name, NPVariant* result);
extern bool SetProperty(NPObject* object, NPIdentifier name, const NPVariant* value);
extern bool RemoveProperty(NPObject* object, NPIdentifier name);

class BaseObject : public NPObject
{
public:
   virtual ~BaseObject() {}
   virtual void Invalidate() {}
   virtual bool HasMethod(NPIdentifier name) { return false; }
   virtual bool Invoke(NPIdentifier name, const NPVariant* args, uint32_t arg_count, NPVariant* result) { return false; }
   virtual bool InvokeDefault(const NPVariant* args, uint32_t arg_count, NPVariant* result) { return false; }
   virtual bool HasProperty(NPIdentifier name) { return false; }
   virtual bool GetProperty(NPIdentifier name, NPVariant* result) { return false; }
   virtual bool SetProperty(NPIdentifier name, const NPVariant* value) { return false; }
   virtual bool RemoveProperty(NPIdentifier name) { return false; }
};

template<class T>
class ScriptableObject : public BaseObject
{
public:
   ScriptableObject(NPNetscapeFuncs* funcs) : npn(funcs) {}
   ~ScriptableObject() {}

   typedef bool(T::*Method)(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   typedef bool(T::*GetPropertyFn)(NPVariant* result);
   typedef bool(T::*SetPropertyFn)(const NPVariant* value);

   static std::map<NPIdentifier, Method>*& method_table() 
   {
      static std::map<NPIdentifier, Method>* table;
      return table;
   }
   
   static std::map<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >*& property_table()
   {
      static std::map<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >* table;
      return table;
   }

   bool add_method(const char* name, Method method)
   {
      if(!method_table())
         method_table() = new(std::nothrow) std::map<NPIdentifier, Method>;
      if(method_table() == NULL)
         return false;

      NPIdentifier id = npn->getstringidentifier(name);
      method_table()->insert(std::pair<NPIdentifier, Method>(id, method));
      return true;
   }

   bool add_property(const char* name, GetPropertyFn g, SetPropertyFn s)
   {
      if(!property_table())
         property_table() = new(std::nothrow) std::map<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >;
      if(!property_table())
         return false;

      NPIdentifier id = npn->getstringidentifier(name);
      property_table()->insert(std::pair<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >(id, std::pair<GetPropertyFn, SetPropertyFn>(g, s)));
      return true;
   }

   virtual bool HasMethod(NPIdentifier name)
   {
      if(!method_table())
         return false;

      typename std::map<NPIdentifier, Method>::iterator i;
      i = method_table()->find(name);
      return i != method_table()->end();
   }

   virtual bool Invoke(NPIdentifier name, const NPVariant* args, uint32_t arg_count, NPVariant* result)
   {
      if(!method_table())
         return false;

      VOID_TO_NPVARIANT(*result);

      typename std::map<NPIdentifier, Method>::iterator i;
      i = method_table()->find(name);
      if (i != method_table()->end()) {
        return (static_cast<T*>(this)->*(i->second))(args, arg_count, result);
      }
      return false;
   }

   virtual bool HasProperty(NPIdentifier name)
   {
      if(!property_table())
         return false;

      typename std::map<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >::iterator i;
      i = property_table()->find(name);
      return i != property_table()->end();
   }

   virtual bool GetProperty(NPIdentifier name, NPVariant* result)
   {
      if(!property_table())
         return false;

      VOID_TO_NPVARIANT(*result);

      typename std::map<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >::iterator i;
      i = property_table()->find(name);
      if (i != property_table()->end()) {
         if(i->second.first)
            return (static_cast<T*>(this)->*(i->second.first))(result);
      }
      return false;
   }

   virtual bool SetProperty(NPIdentifier name, const NPVariant* value)
   {
      if(!property_table())
         return false;

      typename std::map<NPIdentifier, std::pair<GetPropertyFn, SetPropertyFn> >::iterator i;
      i = property_table()->find(name);
      if (i != property_table()->end()) {
         if(i->second.second)
            return (static_cast<T*>(this)->*(i->second.second))(value);
      }
      return false;
   }

   static NPObject* Allocate(NPP npp, NPClass*)
   {
      fprintf(stderr, "[HelloWorld] - Allocate\n");
      return static_cast<NPObject*>(new T(npp));
   }

   static NPClass* GetNPClass()
   {
      static NPClass np_class = {
         NP_CLASS_STRUCT_VERSION,
         ScriptableObject<T>::Allocate,
         ::Deallocate,
         ::Invalidate,
         ::HasMethod,
         ::Invoke,
         ::InvokeDefault,
         ::HasProperty,
         ::GetProperty,
         ::SetProperty,
         ::RemoveProperty
      };
      return &np_class;
   }
private:
   NPNetscapeFuncs* npn;
};

#endif
