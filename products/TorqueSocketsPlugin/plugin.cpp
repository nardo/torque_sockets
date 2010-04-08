/*
 *  plugin.cpp
 *  Hello
 *
 *  Created by Alex Scarborough on 11/9/09.
 *  Copyright 2009 GarageGames.com, Inc. All rights reserved.
 *
 */

#include "plugin.h"
#include <string>
#include <string.h>
#include <stdio.h>
#include "socketObject.h"

static NPNetscapeFuncs NPNFuncs;

NPObject* Plugin::GetScriptableObject()
{
   if(!scriptable_object_)
      scriptable_object_ = npn_->createobject(npp_, ScriptableObject<ScriptablePlugin>::GetNPClass());
   if(scriptable_object_)
      npn_->retainobject(scriptable_object_);
      
   return scriptable_object_;
}

Plugin::~Plugin()
{
   if(scriptable_object_)
      npn_->releaseobject(scriptable_object_);
}

ScriptablePlugin::ScriptablePlugin(NPP npp) : Parent(static_cast<Plugin*>(npp->pdata)->npn()), npp_(npp), npn_(static_cast<Plugin*>(npp->pdata)->npn())
{
   if(!Parent::method_table())
   {
      fprintf(stderr, "[HelloWorld] - Creating ScriptablePlugin\n");
      add_method("torqueSocket", &ScriptablePlugin::CreateTorqueSocket);
   }
}

bool ScriptablePlugin::CreateTorqueSocket(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   NPObject* object = npn_->createobject(npp_, ScriptableObject<SocketObject>::GetNPClass());
   OBJECT_TO_NPVARIANT(object, *result);
   return true;
}

#ifdef XP_UNIX
NPError NP_Initialize(NPNetscapeFuncs *pFuncs, NPPluginFuncs* outFuncs)
#else
NPError NP_Initialize(NPNetscapeFuncs *pFuncs)
#endif
{
	fprintf(stderr, "[HelloWorld] - NP_Initialize\n");
   if (pFuncs == NULL)
   {
   	fprintf(stderr, "[HelloWorld] - NPERR_INVALID_FUNCTABLE_ERROR\n");
      return NPERR_INVALID_FUNCTABLE_ERROR;
   }

   if ((pFuncs->version >> 8) > NP_VERSION_MAJOR)
   {
   	fprintf(stderr, "[HelloWorld] - NPERR_INCOMPATIBLE_VERSION_ERROR\n");
      return NPERR_INCOMPATIBLE_VERSION_ERROR;
   }
   
   fprintf(stderr, "[HelloWorld] - I am version %i, and host is version %i\n", NP_VERSION_MINOR, (int)((char)pFuncs->version));

   // Safari sets the pfuncs size to 0
   if (pFuncs->size == 0)
      pFuncs->size = sizeof(NPNetscapeFuncs);
   if (pFuncs->size < sizeof (NPNetscapeFuncs))
   {
   	fprintf(stderr, "[HelloWorld] - Too Small, %i vs %lu\n", pFuncs->size, sizeof(NPNetscapeFuncs));
      return NPERR_INVALID_FUNCTABLE_ERROR;
   }

   NPNFuncs = *pFuncs;

#ifndef XP_MACOSX
   NP_GetEntryPoints(outFuncs);
#endif
   
   fprintf(stderr, "[HelloWorld] - Sweet Success\n");

   return NPERR_NO_ERROR;
}

char* NP_GetMIMEDescription()
{
	return "application/x-hello-world::Hello World Plugin";
}

NPError NP_GetEntryPoints(NPPluginFuncs* pFuncs)
{
   if (pFuncs == NULL) {
      fprintf(stderr, "[HelloWorld] - NP_GetEntryPoints no table\n");
      return NPERR_INVALID_FUNCTABLE_ERROR;
   }

   // Safari sets the size field of pFuncs to 0
   if (pFuncs->size == 0)
      pFuncs->size = sizeof(NPPluginFuncs);
   if (pFuncs->size < sizeof(NPPluginFuncs)) {
	   fprintf(stderr, "[HelloWorld] - NP_GetEntryPoints table too small\n");
      return NPERR_INVALID_FUNCTABLE_ERROR;
   }
   
   fprintf(stderr, "[HelloWorld] - NP_GetEntryPoints\n");

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
      *val = "Hello World Plugin";
      return NPERR_NO_ERROR;
   }

   if (variable == NPPVpluginDescriptionString)
   {
		const char** val = (const char**)value;
      *val = "Hello World Plugin";
      return NPERR_NO_ERROR;
   }
   
   return NPERR_INVALID_PARAM;
}

// Called to create a new instance of the plugin
NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16_t mode, 
	int16_t argc, char* argn[], char* argv[], NPSavedData* saved)
{
   Plugin* plugin = new(std::nothrow) Plugin(instance, &NPNFuncs);
   if(!plugin)
      return NPERR_OUT_OF_MEMORY_ERROR;
   
   instance->pdata = plugin;
   return NPERR_NO_ERROR;
}

// Called to destroy an instance of the plugin
NPError NPP_Destroy(NPP instance, NPSavedData** save)
{
   if(!instance)
      return NPERR_INVALID_INSTANCE_ERROR;
   
   Plugin* plugin = static_cast<Plugin*>(instance->pdata);
   delete plugin;
   instance->pdata = NULL;
   return NPERR_NO_ERROR;
}

// Called to update a plugin instances's NPWindow
NPError NPP_SetWindow(NPP instance, NPWindow* window)
{
  return NPERR_NO_ERROR;
}


NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, 
	NPBool seekable, uint16_t* stype)
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

int32_t NPP_Write(NPP instance, NPStream* stream, int32_t offset, 
	int32_t len, void* buffer)
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

void NPP_URLNotify(NPP instance, const char* url, NPReason reason, 
	void* notifyData)
{

}

NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
	if (variable == NPPVpluginScriptableNPObject)
   {
      fprintf(stderr, "[HelloWorld] - NPPGetValue\n");
      if(!instance)
         return NPERR_INVALID_INSTANCE_ERROR;
         
      NPObject* object = NULL;
      Plugin* plugin = static_cast<Plugin*>(instance->pdata);
      if(plugin)
         fprintf(stderr, "[HelloWorld] - Have plugin\n");
      if(plugin)
         object = plugin->GetScriptableObject();
      if(object)
         fprintf(stderr, "[HelloWorld] - Returning scriptable object\n");
      fflush(stderr);
      *reinterpret_cast<NPObject**>(value) = object;
      return NPERR_NO_ERROR;
   }

  return NPERR_INVALID_PARAM;
}

NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value)
{
  return NPERR_GENERIC_ERROR;
}

NPError NP_Shutdown()
{
	return NPERR_NO_ERROR;
}
