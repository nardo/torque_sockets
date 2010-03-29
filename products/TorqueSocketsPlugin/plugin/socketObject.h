#ifndef SOCKET_OBJECT_H_
#define SOCKET_OBJECT_H_

#include "scriptableObject.h"

class SocketObject : public ScriptableObject<SocketObject>
{
public:
   typedef ScriptableObject<SocketObject> Parent;
   
   SocketObject(NPP npp);
   
   NPP npp() const { return npp_; }
   bool setKeyPair(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   bool connect(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   bool acceptChallenge(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   bool acceptConnection(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   bool close(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   bool sendTo(const NPVariant* args, uint32_t arg_count, NPVariant* result);
   
   bool setOnChallengeResponse(const NPVariant* value);
   bool getOnChallengeResponse(NPVariant* result);
   
   bool setOnConnectRequest(const NPVariant* value);
   bool getOnConnectRequest(NPVariant* result);
   
   bool setOnEstablished(const NPVariant* value);
   bool getOnEstablished(NPVariant* result);
   
   bool setOnClose(const NPVariant* value);
   bool getOnClose(NPVariant* result);
   
   bool setOnPacket(const NPVariant* value);
   bool getOnPacket(NPVariant* result);
   
   bool setOnPacketDeliveryNotify(const NPVariant* value);
   bool getOnPacketDeliveryNotify(NPVariant* result);
private:
   NPP npp_;
   NPNetscapeFuncs* npn_;
   
   NPObject* onChallengeResponse_;
   NPObject* onConnectRequest_;
   NPObject* onEstablished_;
   NPObject* onClose_;
   NPObject* onPacket_;
   NPObject* onPacketDeliveryNotify_;
};

#endif
