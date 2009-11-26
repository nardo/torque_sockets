#define crypto_key            ecc_key
#define crypto_make_key       ecc_make_key
#define crypto_free           ecc_free
#define crypto_import         ecc_import
#define crypto_export         ecc_export
#define crypto_shared_secret  ecc_shared_secret

class asymmetric_key : public ref_object
{
enum {
   static_crypto_buffer_size = 2048,
};

   /// Raw key data.
   ///
   /// The specific format of this buffer varies by cryptosystem, so look
   /// at the subclass to see how it's laid out. In general this is an
   /// opaque buffer.
   void *_key_data;

   /// Size of the key at construct time.
   uint32 _key_size;

   /// Do we have the private key for this?
   ///
   /// We may only have access to the public key (for instance, when validating
   /// a message signed by someone else).
   bool _has_private_key;

   /// Buffer containing the public half of this keypair.
   byte_buffer_ptr _public_key;

   /// Buffer containing the private half of this keypair.
   byte_buffer_ptr _private_key;
   bool _is_valid;

   /// Load keypair from a buffer.
   void load(const uint8 *buffer_ptr, uint32 buffer_size)
	{
		uint8 static_crypto_buffer[static_crypto_buffer_size];
	   _is_valid = false;

	   crypto_key *theKey = (crypto_key *) memory_allocate(sizeof(crypto_key));
	   _has_private_key = buffer_ptr[0] == key_type_private;

	   if(buffer_size < sizeof(uint32) + 1)
		  return;

	   _key_size = read_uint32_from_buffer(buffer_ptr + 1);

	   if( crypto_import(buffer_ptr + sizeof(uint32) + 1, buffer_size - sizeof(uint32) - 1, theKey)
			 != CRYPT_OK)
		  return;

	   _key_data = theKey;

	   if(_has_private_key)
	   {
		  unsigned long bufferLen = sizeof(static_crypto_buffer) - sizeof(uint32) - 1;
		  static_crypto_buffer[0] = key_type_public;

		  write_uint32_to_buffer(_key_size, static_crypto_buffer);

		  if( crypto_export(static_crypto_buffer + sizeof(uint32) + 1, &bufferLen, PK_PUBLIC, theKey)
				!= CRYPT_OK )
			 return;

		  bufferLen += sizeof(uint32) + 1;

		  _public_key = new byte_buffer(static_crypto_buffer, bufferLen);
		  _private_key = new byte_buffer((uint8 *) buffer_ptr, buffer_size);
	   }
	   else
	   {
		  _public_key = new byte_buffer((uint8 *) buffer_ptr, buffer_size);
	   }
	   _is_valid = true;
	}



   /// Enum used to indicate the portion of the key we are working with.
   enum key_type {
      key_type_private,
      key_type_public,
   };
public:

   /// Constructs an asymmetric_key from the specified data pointer.
   asymmetric_key(uint8 *data_ptr, uint32 buffer_size) : _key_data(NULL)
   {
      load(data_ptr, buffer_size);
   }

   /// Constructs an asymmetric_key from a byte_buffer.
   asymmetric_key(const byte_buffer &the_buffer) : _key_data(NULL)
   {
      load(the_buffer.get_buffer(), the_buffer.get_buffer_size());
   }

   /// Constructs an asymmetric_key by reading it from a bit_stream.
   asymmetric_key(bit_stream &the_stream)
   {
      byte_buffer_ptr the_buffer;
	   core::read(the_stream, the_buffer);
      load(the_buffer->get_buffer(), the_buffer->get_buffer_size());
   }

   /// Generates a new asymmetric key of key_size bytes
   asymmetric_key(uint32 key_size, random_generator &the_random_generator)
	{
		uint8 static_crypto_buffer[static_crypto_buffer_size];
	   _is_valid = false;

	   int descriptorIndex = register_prng ( &yarrow_desc );
	   crypto_key *theKey = (crypto_key *) memory_allocate(sizeof(crypto_key));

	   if( crypto_make_key(the_random_generator.get_state(), descriptorIndex,
		  key_size, theKey) != CRYPT_OK )
		  return;

	   _key_data = theKey;
	   _key_size = key_size;

	   unsigned long bufferLen = sizeof(static_crypto_buffer) - sizeof(uint32) - 1;

	   static_crypto_buffer[0] = key_type_private;
	   write_uint32_to_buffer(_key_size, static_crypto_buffer + 1);

	   crypto_export(static_crypto_buffer + sizeof(uint32) + 1, &bufferLen, PK_PRIVATE, theKey);
	   bufferLen += sizeof(uint32) + 1;

	   _private_key = new byte_buffer(static_crypto_buffer, bufferLen);

	   bufferLen = sizeof(static_crypto_buffer) - sizeof(uint32) - 1;

	   static_crypto_buffer[0] = key_type_public;
	   write_uint32_to_buffer(_key_size, static_crypto_buffer + 1);

	   crypto_export(static_crypto_buffer + sizeof(uint32) + 1, &bufferLen, PK_PUBLIC, theKey);
	   bufferLen += sizeof(uint32) + 1;

	   _public_key = new byte_buffer(static_crypto_buffer, bufferLen);

	   _has_private_key = true;
	   _is_valid = true;
	}


   
   /// Destructor for the asymmetric_key.
   ~asymmetric_key()
	{
	   if(_key_data)
	   {
		  crypto_free((crypto_key *) _key_data);
		  memory_deallocate(_key_data);
	   }
	}



   /// Returns a byte_buffer containing an encoding of the public key.
   byte_buffer_ptr getPublicKey() { return _public_key; }

   /// Returns a byte_buffer containing an encoding of the private key.
   byte_buffer_ptr getPrivateKey() { return _private_key; }

   /// Returns true if this asymmetric_key is a key pair.
   bool hasPrivateKey() { return _has_private_key; }

   /// Returns true if this is a valid key.
   bool isValid() { return _is_valid; }
   /// Compute a key we can share with the specified asymmetric_key
   /// for a symmetric crypto.
   byte_buffer_ptr computeSharedSecretKey(asymmetric_key *publicKey)
	{
	   if(publicKey->getKeySize() != getKeySize() || !_has_private_key)
		  return NULL;

	   uint8 hash[32];
		uint8 static_crypto_buffer[static_crypto_buffer_size];
	   unsigned long outLen = sizeof(static_crypto_buffer);

	   crypto_shared_secret((crypto_key *) _key_data, (crypto_key *) publicKey->_key_data,
		  static_crypto_buffer, &outLen);

	   hash_state hashState;
	   sha256_init(&hashState);
	   sha256_process(&hashState, static_crypto_buffer, outLen);
	   sha256_done(&hashState, hash);

	   return new byte_buffer(hash, 32);
	}

   /// Returns the strength of the asymmetric_key in byte size.
   uint32 getKeySize() { return _key_size; }

   /// Constructs a digital signature for the specified buffer of bits.  This
   /// method only works for private keys.  A public key only Asymmetric key
   /// will generate a signature of 0 bytes in length.
   byte_buffer_ptr hashAndSign(random_generator &the_random_generator, const uint8 *buffer, uint32 buffer_size)
	{
	   int descriptorIndex = register_prng ( &yarrow_desc );

	   uint8 hash[32];
	   hash_state hashState;
		uint8 static_crypto_buffer[static_crypto_buffer_size];

	   sha256_init(&hashState);
	   sha256_process(&hashState, buffer, buffer_size);
	   sha256_done(&hashState, hash);

	   unsigned long outlen = sizeof(static_crypto_buffer);

	   ecc_sign_hash(hash, 32,
		  static_crypto_buffer, &outlen,
		  the_random_generator.get_state(), descriptorIndex, (crypto_key *) _key_data);

	   return new byte_buffer(static_crypto_buffer, (uint32) outlen);
	}


   /// Returns true if the private key associated with this asymmetric_key 
   /// signed theByteBuffer with theSignature.
   bool verifySignature(const uint8 *signedBytes, uint32 signed_bytes_size, const byte_buffer &theSignature)
	{
	   uint8 hash[32];
	   hash_state hashState;

	   sha256_init(&hashState);
	   sha256_process(&hashState, signedBytes, signed_bytes_size);
	   sha256_done(&hashState, hash);

	   int stat;

	   ecc_verify_hash(theSignature.get_buffer(), theSignature.get_buffer_size(), hash, 32, &stat, (crypto_key *) _key_data);
	   return stat != 0;
	}


};

typedef ref_ptr<asymmetric_key> asymmetric_key_ptr;

