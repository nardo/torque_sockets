// Pepper plugin framework class
// (C) GarageGames.  This code is released under the MIT license.
#ifdef WIN32
#include <windows.h>
#endif

#include "npapi.h"
#include "npupp.h"
#include "npapi.h"
#include "npupp.h"

#include <map>
#include <string>
#include <string.h>
#include <stdio.h>

#ifndef WIN32
#define OSCALL
#endif

static NPNetscapeFuncs NPNFuncs;
class scriptable_class;
class plugin;

extern void plugin_main();
using namespace core;

static type_database &global_type_database()
{
	static context global_context;
	static type_database db(&global_context);
	return db;
}

struct NPObjectRef
{
	NPObject *obj;
	
	NPObjectRef()
	{
		obj = 0;
	}
	~NPObjectRef()
	{
		if(obj)
			NPNFuncs.releaseobject(obj);
	}
	void operator=(NPObject *the_object)
	{
		if(obj)
			NPNFuncs.releaseobject(obj);
		obj = the_object;
		if(obj)
			NPNFuncs.retainobject(obj);
	}
	operator NPObject*()
	{
		return obj;
	}
};

class scriptable_object : public NPObject
{
	friend class scriptable_class;	
	NPP _plugin_instance;
public:
	NPP get_plugin_instance()
	{
		return _plugin_instance;
	}
	
	void _call_function(NPObject *function, function_type_signature *calling_signature, void *return_value, void **arguments)
	{
		NPVariant np_args[function_call_record::max_arguments];
		NPVariant np_return_value;
		type_record *variant_type = get_global_type_record<NPVariant>();
		
		type_database &db = global_type_database();
		for(core::uint32 i = 0; i < calling_signature->argument_count; i++)
			db.type_convert(np_args + i, variant_type, arguments[i], calling_signature->argument_types[i]);
		NPNFuncs.invokeDefault(_plugin_instance, function, np_args, calling_signature->argument_count, &np_return_value);
		db.type_convert(return_value, calling_signature->return_type, &np_return_value, variant_type);
		NPNFuncs.releasevariantvalue(&np_return_value);
	}
	template<class return_type> void call_function(NPObjectRef &func, return_type *return_value)
	{
		static function_call_record_decl<return_type> call_decl;
		_call_function(func, call_decl.get_signature(), return_value, 0);
	}
	
	template<class return_type, class arg0_type> void call_function(NPObjectRef &func, return_type *return_value, arg0_type &arg0 )
	{
		static function_call_record_decl<return_type,arg0_type> call_decl;
		void *args[1];
		args[0] = &arg0;
		_call_function(func, call_decl.get_signature(), return_value, args);
	}
	template<class return_type, class arg0_type, class arg1_type> void call_function(NPObjectRef &func, return_type *return_value, arg0_type &arg0, arg1_type &arg1 )
	{
		static function_call_record_decl<return_type,arg0_type,arg1_type> call_decl;
		void *args[2];
		args[0] = &arg0;
		args[1] = &arg1;
		_call_function(func, call_decl.get_signature(), return_value, args);
	}
	template<class return_type, class arg0_type, class arg1_type, class arg2_type> void call_function(NPObjectRef &func, return_type *return_value, arg0_type &arg0, arg1_type &arg1, arg2_type &arg2 )
	{
		static function_call_record_decl<return_type,arg0_type,arg1_type,arg2_type> call_decl;
		void *args[3];
		args[0] = &arg0;
		args[1] = &arg1;
		args[2] = &arg2;
		_call_function(func, call_decl.get_signature(), return_value, args);
	}
};

class scriptable_class : public NPClass
{
	friend class plugin;
	type_record *_instance_type;
	type_database::type_rep *_type_rep;
	hash_table_flat<NPIdentifier, type_database::field_rep *> _fields;
	hash_table_flat<NPIdentifier, function_record *> _methods;
public:
	static type_database::type_rep *get_type_rep(NPObject *the_object)
	{
		return ((scriptable_class *) the_object->_class)->_type_rep;
	}
	
	static NPObject *_allocate(NPP npp, NPClass *the_class)
	{
		scriptable_class *ci = (scriptable_class *) the_class;
		logprintf("_allocate %s", ci->_type_rep->name.c_str());
		core::uint32 instance_size = ci->_instance_type->size;
		scriptable_object *instance = (scriptable_object *) NPNFuncs.memalloc(instance_size);
		ci->_instance_type->construct_object(instance);
		instance->_plugin_instance = npp;
		instance->_class = the_class;
		instance->referenceCount = 1;
		return instance;
	}
	
	static void _deallocate(NPObject *object)
	{
		logprintf("_deallocate");
		get_type_rep(object)->type->destroy_object(object);
		NPNFuncs.memfree(object);
		logprintf("_deallocate_done");
	}
	
	static void _invalidate(NPObject *object)
	{
	}
	
	static bool _has_method(NPObject *object, NPIdentifier name)
	{
		NPUTF8 *text = NPNFuncs.utf8fromidentifier(name);
		logprintf("_has_method %s", text);
		
		NPNFuncs.memfree(text);
		
		scriptable_class *cls = (scriptable_class *) object->_class;
		return bool(cls->_methods.find(name));
	}
	
	static bool _invoke(NPObject* object, NPIdentifier name, const NPVariant* args, uint32_t arg_count, NPVariant* result)
	{
		NPUTF8 *text = NPNFuncs.utf8fromidentifier(name);
		logprintf("_invoke %s", text);
		NPNFuncs.memfree(text);

		scriptable_class *cls = (scriptable_class *) object->_class;
		function_record **method = cls->_methods.find(name).value();
		if(!method)
			return false;

		function_record *the_method = *method;
		function_type_signature *sig = the_method->get_signature();
		if(arg_count != sig->argument_count)
			return false;
		
		type_database &db = global_type_database();
		function_call_storage storage(sig);
		
		for(core::uint32 i = 0; i < arg_count; i++)
		{
			db.type_convert(storage._args[i], sig->argument_types[i], &args[i], get_global_type_record<NPVariant>());
		}
		the_method->dispatch(object, storage._args, storage._return_value);
		db.type_convert(result, get_global_type_record<NPVariant>(), storage._return_value, sig->return_type);
		return true;
	}

	static bool _invoke_default(NPObject* object, const NPVariant* args, uint32_t arg_count, NPVariant* result)
	{
		return false; //return _invoke(object, args, arg_count, result);
	}
	
	static bool _has_property(NPObject* object, NPIdentifier name)
	{
		NPUTF8 *text = NPNFuncs.utf8fromidentifier(name);
		logprintf("_has_property %s", text);
		NPNFuncs.memfree(text);

		scriptable_class *cls = (scriptable_class *) object->_class;
		type_database::field_rep **field = cls->_fields.find(name).value();
		return field != 0;
	}

	static bool _get_property(NPObject* object, NPIdentifier name, NPVariant* result)
	{
		NPUTF8 *text = NPNFuncs.utf8fromidentifier(name);
		logprintf("_get_property %s", text);
		NPNFuncs.memfree(text);
		
		scriptable_class *cls = (scriptable_class *) object->_class;
		type_database::field_rep **field = cls->_fields.find(name).value();
		if(!field)
			return false;
		global_type_database().type_convert(result, get_global_type_record<NPVariant>(), ((core::uint8*)object) + (*field)->offset, (*field)->type);
		return true;
	}
	
	static bool _set_property(NPObject* object, NPIdentifier name, const NPVariant* value)
	{
		NPUTF8 *text = NPNFuncs.utf8fromidentifier(name);
		logprintf("_set_property %s", text);
		NPNFuncs.memfree(text);
		
		scriptable_class *cls = (scriptable_class *) object->_class;
		type_database::field_rep **field = cls->_fields.find(name).value();
		if(!field)
			return false;
		global_type_database().type_convert( ((core::uint8*)object) + (*field)->offset, (*field)->type, value, get_global_type_record<NPVariant>());
		return true;
	}

	static bool _remove_property(NPObject* object, NPIdentifier name)
	{
		return false;
	}
	
	static bool _enumerate(NPObject *npobj, NPIdentifier **value, uint32_t *count)
	{
		return false;
	}
	
	static bool _construct(NPObject *npobj, const NPVariant *args,uint32_t argCount, NPVariant *result)
	{
		return false;
	}
	
	scriptable_class(type_record *type, type_database *db)
	{
		structVersion = NP_CLASS_STRUCT_VERSION;
		allocate = &_allocate;
		deallocate = &_deallocate;
		invalidate = &_invalidate;
		hasMethod = &_has_method;
		invoke = &_invoke;
		invokeDefault = &_invoke_default;
		hasProperty = &_has_property;
		getProperty = &_get_property;
		setProperty = &_set_property;
		removeProperty = &_remove_property;
		enumerate = &_enumerate;
		construct = &_construct;
		
		_type_rep = db->find_type(type);
		assert(_type_rep); // class must be registered in the type database
		_instance_type = type;

		_instance_type = _type_rep->type;
		logprintf("Added class %s, type_record = %08x", _type_rep->name.c_str(), _instance_type);
	
		for(dictionary<type_database::field_rep>::pointer p = _type_rep->fields.first(); p; ++p)
		{
			indexed_string name = *(p.key());
			type_database::field_rep *field = p.value();
			NPIdentifier ident = NPNFuncs.getstringidentifier(name.c_str());
			_fields.insert(ident, field);
		}
		for(hash_table_flat<indexed_string, function_record *>::pointer p = _type_rep->method_table.first(); p; ++p)
		{
			indexed_string name = *(p.key());
			function_record *method = *(p.value());
			NPIdentifier ident = NPNFuncs.getstringidentifier(name.c_str());
			_methods.insert(ident, method);			
		}
	}
};

bool void_from_np_variant(empty_type *dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_void");
	return true; // NP_VARIANT_IS_NULL(src
}

bool int32_from_np_variant(core::int32 *dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_int32");
	*dest = NPVARIANT_TO_INT32(*src);
	return true;
}

bool bool_from_np_variant(bool *dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_bool");
	*dest = NPVARIANT_TO_BOOLEAN(*src);
	return true;
}

bool double_from_np_variant(core::float64 *dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_double");
	*dest = NPVARIANT_TO_DOUBLE(*src);
	return true;
}

bool string_from_np_variant(core::string *dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_string");
	if(NPVARIANT_IS_STRING(*src))
	{		
		dest->set(src->value.stringValue.utf8characters, src->value.stringValue.utf8length);
		return true;
	}
	dest->set("");
	return false;
}

bool object_ref_from_np_variant(NPObjectRef *dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_object_ref");
	if(NPVARIANT_IS_OBJECT(*src))
	{
		*dest = NPVARIANT_TO_OBJECT(*src);
		return true;
	}
	*dest = 0;
	return false;
}

bool object_from_np_variant(NPObject **dest, NPVariant *src, context *)
{
	logprintf("np_variant_to_object");
	if(NPVARIANT_IS_OBJECT(*src))
	{
		*dest = NPVARIANT_TO_OBJECT(*src);
		return true;
	}
	*dest = 0;
	return false;
}

bool np_variant_from_void(NPVariant *dest, empty_type *src, context *)
{
	logprintf("void_to_np_variant");
	VOID_TO_NPVARIANT(*dest);
	return true;
}

bool np_variant_from_int32(NPVariant *dest, core::int32 *src, context *)
{
	logprintf("int32_to_np_variant");
	INT32_TO_NPVARIANT(*src, *dest);
	return true;
}

bool np_variant_from_bool(NPVariant *dest, bool *src, context *)
{
	logprintf("bool_to_np_variant");
	BOOLEAN_TO_NPVARIANT(*src, *dest);
	return true;
}

bool np_variant_from_double(NPVariant *dest, core::float64 *src, context *)
{
	logprintf("double_to_np_variant");
	DOUBLE_TO_NPVARIANT(*src, *dest);
	return true;
}

bool np_variant_from_string(NPVariant *dest, core::string *src, context *)
{
	logprintf("string_to_np_variant");
	STRINGN_TO_NPVARIANT(src->c_str(), src->len(), *dest);
	return true;
}

bool np_variant_from_object(NPVariant *dest, NPObject **src, context *)
{
	logprintf("object_to_np_variant");
	OBJECT_TO_NPVARIANT(*src, *dest);
	return true;
}

bool np_variant_from_object_ref(NPVariant *dest, NPObjectRef *src, context *)
{
	logprintf("object_to_np_variant");
	NPObject *the_object = *src;
	OBJECT_TO_NPVARIANT(the_object, *dest);
	return true;
}


class plugin
{
	array<scriptable_class *> _classes;
	type_record *_plugin_class;
public:		
	void add_class(type_record *the_type)
	{
		scriptable_class *cls = new scriptable_class(the_type, &global_type_database());
		_classes.push_back(cls);
	}
	
	void set_plugin_class(type_record *class_type)
	{
		_plugin_class = class_type;
	}
	
	type_record *get_plugin_class()
	{
		return _plugin_class;
	}
		
	plugin()
	{
		_plugin_class = 0;
		type_database &db = global_type_database();
		db.add_type_conversion(void_from_np_variant, false);
		db.add_type_conversion(int32_from_np_variant, false);
		db.add_type_conversion(bool_from_np_variant, false);
		db.add_type_conversion(double_from_np_variant, false);
		db.add_type_conversion(string_from_np_variant, false);
		db.add_type_conversion(object_from_np_variant, false);
		db.add_type_conversion(object_ref_from_np_variant, false);
		
		db.add_type_conversion(np_variant_from_void, false);
		db.add_type_conversion(np_variant_from_int32, false);
		db.add_type_conversion(np_variant_from_bool, false);
		db.add_type_conversion(np_variant_from_double, false);
		db.add_type_conversion(np_variant_from_string, false);
		db.add_type_conversion(np_variant_from_object, false);
		db.add_type_conversion(np_variant_from_object_ref, false);
	}
	
	~plugin()
	{
		for(core::uint32 i = 0; i < _classes.size(); i++)
			delete _classes[i];
	}
	
	NPObject *create_object(NPP instance, type_record *class_type)
	{
		scriptable_class *the_class = 0;
		logprintf("Creating object instance of class %08x", class_type);
		for(core::uint32 i = 0; i < _classes.size(); i++)
			if(_classes[i]->_instance_type == class_type)
				the_class = _classes[i];
		assert(the_class); // the class was not added to this plugin
		return NPNFuncs.createobject(instance, the_class);
	}
};

plugin global_plugin;

class plugin_wrapper
{
	NPObject *_scriptable_object;
public:
	plugin_wrapper()
	{
		logprintf("plugin_wrapper()");
		_scriptable_object = 0;
	}
	~plugin_wrapper()
	{
		logprintf("~plugin_wrapper()");
		if(_scriptable_object)
			NPNFuncs.releaseobject(_scriptable_object);
		//logprintf("~plugin_wrapper_fin()");
	}
	
	NPObject *get_scriptable_object(NPP instance)
	{
		if(!_scriptable_object)
			_scriptable_object = global_plugin.create_object(instance, global_plugin.get_plugin_class());

		if(_scriptable_object)
			NPNFuncs.retainobject(_scriptable_object);
		
		return _scriptable_object;
	}
};

// Called to create a new instance of the plugin
NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
	logprintf("NPP_New");
	instance->pdata = new plugin_wrapper;
	if(!instance->pdata)
		return NPERR_OUT_OF_MEMORY_ERROR;
	return NPERR_NO_ERROR;
}

// Called to destroy an instance of the plugin
NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
	logprintf("NPP_Destroy");
	if(!instance)
		return NPERR_INVALID_INSTANCE_ERROR;
	delete (plugin_wrapper *) instance->pdata;
	instance->pdata = NULL;
	return NPERR_NO_ERROR;
}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
	if (variable == NPPVpluginScriptableNPObject)
	{
		if(!instance)
			return NPERR_INVALID_INSTANCE_ERROR;
		logprintf("NPPGetValue");
		*((NPObject**) value) = (NPObject *) ((plugin_wrapper *) instance->pdata)->get_scriptable_object(instance);
		return NPERR_NO_ERROR;
	}
	
	return NPERR_INVALID_PARAM;
}

// Called to update a plugin instances's NPWindow
NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
	return NPERR_NO_ERROR;
}


NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16_t* stype)
{
	*stype = NP_ASFILEONLY;
	return NPERR_NO_ERROR;
}

NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason)
{
	return NPERR_NO_ERROR;
}

int32_t NPP_WriteReady(NPP instance, NPStream* stream)
{
	return 0;
}

int32_t NPP_Write(NPP instance, NPStream* stream, int32_t offset, int32_t len, void* buffer)
{
	return 0;
}

void NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname)
{
}

void NPP_Print(NPP instance, NPPrint* platformPrint)
{
	
}

int16_t NPP_HandleEvent(NPP instance, void* event)
{
	return 0;
}

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
	
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
	return NPERR_GENERIC_ERROR;
}

#ifdef __cplusplus
extern "C" {
#endif


#ifdef XP_UNIX
NPError NP_Initialize(NPNetscapeFuncs *pFuncs, NPPluginFuncs* outFuncs)
#else
NPError NP_Initialize(NPNetscapeFuncs *pFuncs)
#endif
{
	logprintf("NP_Initialize");
	if (pFuncs == NULL)
	{
		logprintf("NPERR_INVALID_FUNCTABLE_ERROR");
		return NPERR_INVALID_FUNCTABLE_ERROR;
	}
	
	if ((pFuncs->version >> 8) > NP_VERSION_MAJOR)
	{
		logprintf("NPERR_INCOMPATIBLE_VERSION_ERROR");
		return NPERR_INCOMPATIBLE_VERSION_ERROR;
	}
	
	logprintf("I am version %i, and host is version %i", NP_VERSION_MINOR, (int)((char)pFuncs->version));
	
	// Safari sets the pfuncs size to 0
	if (pFuncs->size == 0)
		pFuncs->size = sizeof(NPNetscapeFuncs);
	if (pFuncs->size < sizeof (NPNetscapeFuncs))
	{
		logprintf("Too Small, %i vs %lu", pFuncs->size, sizeof(NPNetscapeFuncs));
		return NPERR_INVALID_FUNCTABLE_ERROR;
	}
	
	NPNFuncs = *pFuncs;
	plugin_main();

#ifndef XP_MACOSX
	NP_GetEntryPoints(outFuncs);
#endif
	
	logprintf("Sweet Success");
	
	return NPERR_NO_ERROR;
}
	
NPError NP_Shutdown()
{
	logprintf("NP_Shutdown");
	return NPERR_NO_ERROR;
}	

char* NP_GetMIMEDescription()
{
	return "application/x-hello-world::Hello World Plugin";
}

NPError NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
	if (pFuncs == NULL) {
		logprintf("NP_GetEntryPoints no table");
		return NPERR_INVALID_FUNCTABLE_ERROR;
	}
	
	// Safari sets the size field of pFuncs to 0
	if (pFuncs->size == 0)
		pFuncs->size = sizeof(NPPluginFuncs);
	if (pFuncs->size < sizeof(NPPluginFuncs)) {
		logprintf("NP_GetEntryPoints table too small");
		return NPERR_INVALID_FUNCTABLE_ERROR;
	}
	
	logprintf("NP_GetEntryPoints");
	
	pFuncs->version       = (NP_VERSION_MAJOR << 8) | NP_VERSION_MINOR;
	pFuncs->newp          = NPP_New;
	pFuncs->destroy       = NPP_Destroy;
	pFuncs->setwindow     = NPP_SetWindow;
	pFuncs->newstream     = NPP_NewStream;
	pFuncs->destroystream = NPP_DestroyStream;
	pFuncs->asfile        = NPP_StreamAsFile;
	pFuncs->writeready    = NPP_WriteReady;
	pFuncs->write         = NPP_Write;
	pFuncs->print         = NPP_Print;
	pFuncs->event         = NPP_HandleEvent;
	pFuncs->urlnotify     = NPP_URLNotify;
	pFuncs->getvalue      = NPP_GetValue;
	pFuncs->setvalue      = NPP_SetValue;
	pFuncs->javaClass     = NULL;
	
	return NPERR_NO_ERROR;
}

NPError NP_GetValue(void*, NPPVariable variable, void* value)
{
	if (variable == NPPVpluginNameString)
	{
		const char** val = (const char**)value;
		*val = "TorqueSocket Plugin";
		return NPERR_NO_ERROR;
	}
	
	if (variable == NPPVpluginDescriptionString)
	{
		const char** val = (const char**)value;
		*val = "TorqueSocket Plugin";
		return NPERR_NO_ERROR;
	}
	
	return NPERR_INVALID_PARAM;
}

#ifdef __cplusplus
};
#endif
