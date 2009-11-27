struct nonce
{
   enum {
      NonceSize = 8,
   };
   uint8 data[NonceSize];

   nonce() {}
   nonce(const uint8 *ptr) { memcpy(data, ptr, NonceSize); }

   bool operator==(const nonce &theOtherNonce) const { return !memcmp(data, theOtherNonce.data, NonceSize); }
   bool operator!=(const nonce &theOtherNonce) const { return memcmp(data, theOtherNonce.data, NonceSize) != 0; }

   void operator=(const nonce &theNonce) { memcpy(data, theNonce.data, NonceSize); }
   
   void getRandom() { Random::read(data, NonceSize); }
};

template<class S> inline void read(S &stream, nonce *theNonce)
{ 
   stream.readBuffer(nonce::NonceSize, theNonce->data);
}

template<class S> inline void write(S &stream, const nonce &theNonce)
{
   stream.write_buffer(nonce::NonceSize, theNonce.data);
}
