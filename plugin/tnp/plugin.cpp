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

// This method will be called on Unix and on OS X.
// On Unix, this function is the only way to get MIME type information from a
// plugin, and this function must be implemented.  On OS X the information
// returned from this function overrides the information from the Info.plist
// file in the plugin bundle.  This function is optional on OS X.
char* NP_GetMIMEDescription()
{
   return MIME_TYPE"::Torque Network Platform Plugin";
}

// This function is called on Unix to retrieve a human readable plugin name
// and description to display in an about page or help page or otherwise
// (you get the idea).
//
// The first parameter would be an NPP instance, the idea being that you could
// use NP_GetValue as your NP*P*_GetValue. I have chosen not to do this for
// clarity.  NP_GetValue is explicitly exported on Unix and the plugin loads
// this function directly.
NPError OSCALL NP_GetValue(void*, NPPVariable variable, void* value)
{
   // Return the human readable name of our plugin.
   if (variable == NPPVpluginNameString)
   {
      const char** val = (const char**)value;
      *val = "Torque Network Platform Plugin";
      return NPERR_NO_ERROR;
   }

   // Return a human readable description of what the plugin does.
   if (variable == NPPVpluginDescriptionString)
   {
      const char** val = (const char**)value;
      // In your own plugin, make this a bit more descriptive :p
      *val = "Torque Network Platform Plugin";
      return NPERR_NO_ERROR;
   }

   // We don't recognize any other parameters.  If you're not using NP_GetValue
   // as NP*P*_GetValue, then it shouldn't be called with any other parameters
   // anyways.
   return NPERR_INVALID_PARAM;
}
