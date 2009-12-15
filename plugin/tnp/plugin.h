/*
 *  plugin.h
 *  Hello
 *
 *  Created by Alex Scarborough on 11/9/09.
 *  Copyright 2009 GarageGames.com, Inc. All rights reserved.
 *
 */

#ifndef HELLO_PLUGIN_H_
#define HELLO_PLUGIN_H_

#ifdef WIN32
#include <windows.h>
#endif

#include "npapi.h"
#include "npupp.h"

#ifndef WIN32
#define OSCALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef XP_UNIX
NPError NP_Initialize(NPNetscapeFuncs *browserFuncs, NPPluginFuncs* outFuncs);
#else
NPError OSCALL NP_Initialize(NPNetscapeFuncs *browserFuncs);
#endif

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *pluginFuncs);
char* NP_GetMIMEDescription();
NPError OSCALL NP_Shutdown(void);

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char* argn[], char* argv[], NPSavedData* saved);
NPError NPP_Destroy(NPP instance, NPSavedData** save);
NPError NPP_SetWindow(NPP instance, NPWindow* window);
NPError NPP_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype);
NPError NPP_DestroyStream(NPP instance, NPStream* stream, NPReason reason);
int32   NPP_WriteReady(NPP instance, NPStream* stream);
int32   NPP_Write(NPP instance, NPStream* stream, int32 offset, int32 len, void* buffer);
void    NPP_StreamAsFile(NPP instance, NPStream* stream, const char* fname);
void    NPP_Print(NPP instance, NPPrint* platformPrint);
int16   NPP_HandleEvent(NPP instance, void* event);
void    NPP_URLNotify(NPP instance, const char* URL, NPReason reason, void* notifyData);
NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value);
NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value);

#ifdef __cplusplus
}
#endif


#endif
