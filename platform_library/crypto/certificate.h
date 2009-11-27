/// The certificate class manages a digitally signed certificate.
/// Certificates consist of an application-defined payload, a public
/// key, and a signature.  It is up to the application to determine
/// from the payload what, if any, certificate authority (CA) signed
/// the certificate.  The validate() method can be used to check the
/// certificate's authenticity against the known public key of the
/// signing certificate Authority.
///
/// The payload could include such items as:
///  - The valid date range for the certificate
///  - The domain of the certificate holder
///  - The identifying name of the certificate Authority
///  - A player's name and user id for a multiplayer game
///
/// 
class certificate : public ref_object
{
protected:
	byte_buffer_ptr _certificate_data;        ///< The full certificate data
	ref_ptr<asymmetric_key> _public_key;      ///< The public key for the holder of this certificate
	byte_buffer_ptr _payload;                ///< The certificate payload, including the identity of the holder and the certificate Authority
	byte_buffer_ptr _signature;              ///< The digital signature of this certificate by the signatory
	bool _is_valid;                         ///< flag to signify whether this certificate has a valid form
	uint32 _signature_byte_size;                ///< Number of bytes of the byte_buffer signed by the CA
public:
	enum {
	max_payload_size = 512,
	};
	/// certificate constructor
	certificate(uint8 *data_ptr, uint32 data_size)
	{
		byte_buffer_ptr theData = new byte_buffer(data_ptr, data_size); 
		set_certificate_data(theData);
	}

	certificate(byte_buffer_ptr &buffer)
	{
		set_certificate_data(buffer);
	}

	certificate(bit_stream &stream)
	{
		byte_buffer_ptr theBuffer;
		core::read(stream, theBuffer);
		set_certificate_data(theBuffer);
	}
	certificate(const byte_buffer_ptr payload, random_generator &random_gen, ref_ptr<asymmetric_key> public_key, ref_ptr<asymmetric_key> authority_private_key)
	{
		_is_valid = false;
		_signature_byte_size = 0;

		if(payload->get_buffer_size() > max_payload_size || !public_key->is_valid())
			return;

		byte_buffer_ptr the_public_key = public_key->get_public_key();
		packet_stream packet;

		core::write(packet, payload);
		core::write(packet, the_public_key);
		_signature_byte_size = packet.get_byte_position();
		packet.set_byte_position(_signature_byte_size);

		_signature = authority_private_key->hash_and_sign(random_gen, packet.get_buffer(), packet.get_byte_position());
		core::write(packet, _signature);

		_certificate_data = new byte_buffer(packet.get_buffer(), packet.get_byte_position());
	}


	/// Parses this certificate into the payload, public key, identiy, certificate authority and signature
	void parse()
	{
		bit_stream the_stream(_certificate_data->get_buffer(), _certificate_data->get_buffer_size());
		core::read(the_stream, _payload);

		_public_key = new asymmetric_key(the_stream);

		_signature_byte_size = the_stream.get_byte_position();

		// advance the bit stream to the next byte:
		the_stream.set_byte_position(the_stream.get_byte_position());

		core::read(the_stream, _signature);

		if(!the_stream.was_error_detected() && _certificate_data->get_buffer_size() == the_stream.get_byte_position() && _public_key->is_valid())
			_is_valid = true;
	}

	/// Sets the full certificate from a byte_buffer, and parses it.
	void set_certificate_data(byte_buffer_ptr &theBuffer)
	{
		_certificate_data = theBuffer;
		_signature_byte_size = 0;
		_is_valid = false;
		parse();
	}

	/// Returns a byte_buffer_ptr to the full certificate data
	byte_buffer_ptr get_certificate_data()
	{
		return _certificate_data;
	}
	/// Returns a byte_buffer_ptr to the full certificate data
	const byte_buffer_ptr get_certificate_data() const
	{
		return _certificate_data;
	}

	/// returns the validity of the certificate's formation
	bool is_valid()
	{
		return _is_valid;
	}
	/// returns true if this certificate was signed by the private key corresponding to the passed public key.
	bool validate(ref_ptr<asymmetric_key> signatoryPublicKey)
	{
		if(!_is_valid)
			return false;

		return signatoryPublicKey->verify_signature(_certificate_data->get_buffer(), _signature_byte_size, *_signature);
	}

	/// Returns the public key from the certificate
	ref_ptr<asymmetric_key> get_public_key() { return _public_key; }

	/// Returns the certificate payload
	byte_buffer_ptr getPayload() { return _payload; }
};

inline void read(bit_stream &str, ref_ptr<certificate> &theCertificate)
{
	byte_buffer_ptr theBuffer;
	core::read(str, theBuffer);
	theCertificate = new certificate(theBuffer);
}

inline void write(bit_stream &str, const ref_ptr<certificate> &theCertificate)
{
	core::write(str, theCertificate->get_certificate_data());
}
