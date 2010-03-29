#include "socketObject.h"
#include "plugin.h"

SocketObject::SocketObject(NPP npp) : Parent(static_cast<Plugin*>(npp->pdata)->npn()), npp_(npp), npn_(static_cast<Plugin*>(npp->pdata)->npn())
{
   if(!Parent::method_table())
   {
      add_method("setKeyPair", &SocketObject::setKeyPair);
      add_method("connect", &SocketObject::connect);
      add_method("acceptChallenge", &SocketObject::acceptChallenge);
      add_method("acceptConnection", &SocketObject::acceptConnection);
      add_method("close", &SocketObject::close);
      add_method("sendTo", &SocketObject::sendTo);
   }
   
   if(!Parent::property_table())
   {
      add_property("OnChallengeResponse", &SocketObject::getOnChallengeResponse, &SocketObject::setOnChallengeResponse);
      add_property("OnConnectRequest", &SocketObject::getOnConnectRequest, &SocketObject::setOnConnectRequest);
      add_property("OnEstablished", &SocketObject::getOnEstablished, &SocketObject::setOnEstablished);
      add_property("OnClose", &SocketObject::getOnClose, &SocketObject::setOnClose);
      add_property("OnPacket", &SocketObject::getOnPacket, &SocketObject::setOnPacket);
      add_property("OnPacketDeliveryNotify", &SocketObject::getOnPacketDeliveryNotify, &SocketObject::setOnPacketDeliveryNotify);
   }
   
   onChallengeResponse_ = NULL;
   onConnectRequest_ = NULL;
   onClose_ = NULL;
   onPacket_ = NULL;
   onPacketDeliveryNotify_ = NULL;
}

bool SocketObject::setKeyPair(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   VOID_TO_NPVARIANT(*result);
   fprintf(stderr, "[HelloWorld] - argCount is %u\n", arg_count);
   fprintf(stderr, "[HelloWorld] - argument is a %i", args[0].type);
   NPVariant value;
   npn_->invokeDefault(npp_, args[0].value.objectValue, NULL, 0, &value);
   npn_->releasevariantvalue(&value);
   return true;
}

bool SocketObject::connect(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   return false;
}

bool SocketObject::acceptChallenge(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   return false;
}

bool SocketObject::acceptConnection(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   return false;
}

bool SocketObject::close(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   return false;
}

bool SocketObject::sendTo(const NPVariant* args, uint32_t arg_count, NPVariant* result)
{
   return false;
}

#define IMP_SET(property)\
NPObject* temp = NPVARIANT_TO_OBJECT(*value);\
npn_->retainobject(temp);\
if(property)\
npn_->releaseobject(property);\
property = temp;  

bool SocketObject::setOnChallengeResponse(const NPVariant* value)
{
   IMP_SET(onChallengeResponse_);
   return true;
}

bool SocketObject::getOnChallengeResponse(NPVariant* result)
{
   OBJECT_TO_NPVARIANT(onChallengeResponse_, *result);
   return true;
}

bool SocketObject::setOnConnectRequest(const NPVariant* value)
{
   IMP_SET(onConnectRequest_);
   return true;
}

bool SocketObject::getOnConnectRequest(NPVariant* result)
{
   OBJECT_TO_NPVARIANT(onConnectRequest_, *result);
   return true;
}

bool SocketObject::setOnEstablished(const NPVariant* value)
{
   IMP_SET(onEstablished_);
   return true;
}

bool SocketObject::getOnEstablished(NPVariant* result)
{
   OBJECT_TO_NPVARIANT(onEstablished_, *result);
   return true;
}

bool SocketObject::setOnClose(const NPVariant* value)
{
   IMP_SET(onClose_);
   return true;
}

bool SocketObject::getOnClose(NPVariant* result)
{
   OBJECT_TO_NPVARIANT(onClose_, *result);
   return true;
}

bool SocketObject::setOnPacket(const NPVariant* value)
{
   IMP_SET(onPacket_);
   return true;
}

bool SocketObject::getOnPacket(NPVariant* result)
{
   OBJECT_TO_NPVARIANT(onPacket_, *result);
   return true;
}

bool SocketObject::setOnPacketDeliveryNotify(const NPVariant* value)
{
   IMP_SET(onPacketDeliveryNotify_);
   return true;
}

bool SocketObject::getOnPacketDeliveryNotify(NPVariant* result)
{
   OBJECT_TO_NPVARIANT(onPacketDeliveryNotify_, *result);
   return true;
}
