/// torque_socket class.
///
/// Manages all valid and pending notify protocol connections for a single open network socket (port/IP address).  A torque_socket instance is not thread safe, so should only be used from a single thread.  The torque_sockets library is thread safe in the sense that torque_socket instances may be created on different threads.
///
/// <b>Connection handshaking basic overview:</b>
///
/// torque_sockets does a two phase connect handshake to prevent a several types of Denial-of-Service (DoS) attacks.
///
/// The connection initiator starts the connection by sending a unique random nonce (number, used once) value to the host as part of the connect_challenge_request packet.
/// initiator->host: connect_challenge_request(initiator_nonce)
///
/// The server responds to the connect_challenge_request with a "Client Puzzle" that has the property that verifying a solution to the puzzle is computationally simple, but can be of a specified computational, brute-force difficulty to compute the solution itself.  The client puzzle is of the form: secureHash(Ic, initiator_nonce, host_nonce, X) = Y >> k, where Ic is the identity of the client, and X is a value computed by the client such that the high k bits of the value y are all zero.  The client identity is computed by the server as a partial hash of the client's IP address and port and some random data on the host.  The response contains the host's current nonce, the initiator nonce, k, the host's public key and any data set via set_challenge_response_data
/// host->initiator: connect_challenge_response(initiator_nonce, host_nonce, k, host_public_key, host_challenge_response_data)
///
/// The initiator, upon receipt of the connect_challenge_response, validates the packet sent by the host and computes a solution to the puzzle the host sent.  The initiator generates a shared secret from the initiator's key pair and the host's public key.  The initiator response to the host consists of:
/// initiator->host: connect_request(initiator_nonce, host_nonce, X, initiator_public_key, shared_secret(key1, sequence1, connect_request_data))
///
/// The host then can validation the solution to the puzzle the initiator submitted, along with the client identity (Ic).  Until this point the host has allocated no memory for the initiator and has verified that the initiator is sending from a valid IP address, and that the initiator has done some amount of work to prove its willingness to start a connection.  As the host load increases, the host may choose to increase the difficulty (k) of the client puzzles it issues, thereby making a CPU/connection resource depletion DoS attack successively more difficult to launch.
///
/// If the server accepts the connection, it sends a connect accept packet that is encrypted and hashed using the shared secret.  The contents of the packet are another sequence number (sequence2) and another key (key2).  The sequence numbers are the initial send and receive sequence numbers for the torque_connection protocol, and the key2 value becomes the IV of the symmetric cipher.  The connection_accept_data is also sent and encrypted in this channel.
/// 
/// Connections using the secure key exchange are still vulnerable to Man-in-the-middle attacks.  Using a key signed by a trusted certificate authority (CA), makes the communications channel as securely trusted as the trust in the CA.  This could be implemented by the host sending its certificate and a signature of the public key down in the challenge_response_data
///
/// <b>Introduced Connection handshaking:</b>
///
/// torque_socket can also facilitate introduced connections.  Introduced connections are necessary when both parties to the connection are behind firewalls or NAT routers, or don't want to accept anonymous connections from the network.  Suppose there are two processes, initiator and host that want to establish a direct connection and that those two processes are connected to the same third party introducer.  All three parties can simultaneously initiate an introduced connection by calling the appropriate methods on their torque_socket instances.  The protocol works as follows:
/// initiator/host->introducer: request_introduction(remote_connection_id, shared_secret(initiator/host _nonce))
/// introducer->initiator/host: introduction_response( shared_secret( remote_public_addresses, introduced_connection_shared_secret, remote_nonce ) )
/// The remote public addresses are the public (NAT'd) address of the hosts from the perspective of the introducer, as well as the IP addresses each client detects for itself.  This allows clients behind the same firewall to connect with each other using local (untranslated) addresses.
///
/// Once the initiator and host receive introduction_responses, they being sending "punch" packets to the known possible addresses of each other.  The punch packet I sends enables the punch packet H sends to be delivered through the router or firewall since it will appear as though it is a service response to I's initial packet, and vice versa.
///
/// Upon receipt of the punch packet by the initiator of the connection, an arranged_connect_request packet is sent.  If the non-initiator of the connection gets an ArrangedPunch packet, it simply sends another punch packet to the remote host, but narrows down its Address range to the address it received the packet from.
/// The ArrangedPunch packet from the intiator contains the nonce for the non-initiator, and the nonce for the initiator encrypted with the shared secret.  The ArrangedPunch packet for the host of the connection contains all that, plus the public key/key_size of the receiver.

class torque_connection;
struct connection_parameters;

class torque_socket : public ref_object
{
	friend class torque_connection;

/// packet_type is encoded as the first byte of each packet.
	///
	/// Packets received on a socket with a first byte >= first_valid_info_packet_id and < 128 will be passed along in the form of torque_socket_packet_event events.  All other first byte values are reserved by the implementation of torque_sockets.
	
	/// Packets that arrive with the high bit of the first byte set (i.e. the first unsigned byte is greater than 127), are assumed to be connected protocol packets, and are dispatched to the appropriate connection for further processing.
public:
	enum packet_type
	{
		connect_challenge_request_packet, ///< Initial packet of the two-phase connect process
		connect_challenge_response_packet, ///< Response packet to the ChallengeRequest containing client identity, a client puzzle, and possibly the server's public key.
		connect_reject_packet, ///< A client puzzle submission failed - this could be because of server puzzle timing rotation, failure to supply randomized data to the API (two connecting clients supply identical nonces) or because of an attack.  If a connection attempt receives this torque sockets will by default re-randomize the client nonce and try again once.
		connect_request_packet, ///< A connect request packet, including all puzzle solution data and connection initiation data.
		connect_accept_packet, ///< A packet sent to notify a host that a connection was accepted.
		disconnect_packet, ///< A packet sent to notify a host that the specified connection has terminated or that the requested connection is politely rejected.
		introduced_connection_request_packet, ///< sent from the initiator and host to the introducer.  An introducer will ignore introduced_connection_request packets until a call to torque_socket_introduce is made.  Once introduced_connection_request packets are received from both initiator and host, and upon subsequent receipt of introduced_connection_request packets, the introducer will send connection_introduction packets to introducer and host.
		connection_introduction_packet, ///< Packet sent by introducer to properly connect initiator and host.
		punch_packet, ///< Packets sent by initiator or host of an introduced connection to "punch" a connection hole through NATs and firewalls.

		first_valid_info_packet_id = 32, ///< The first valid first byte of an info packet sent from a torque_socekt
		last_valid_info_packet_id = 127, ///< The last valid first byte of an info packet sent from a torque_socekt 
	};
protected:
	enum torque_socket_constants
	{
		challenge_retry_count = 4, ///< Number of times to send connect challenge requests before giving up.
		challenge_retry_time = 2500, ///< Timeout interval in milliseconds before retrying connect challenge.
		
		connect_retry_count = 4, ///< Number of times to send connect requests before giving up.
		connect_retry_time = 2500, ///< Timeout interval in milliseconds before retrying connect request.
		
		punch_retry_count = 6, ///< Number of times to send groups of firewall punch packets before giving up.
		punch_retry_time = 2500, ///< Timeout interval in milliseconds before retrying punch sends.
		
		introduced_connection_connect_timeout = 45000, ///< interval a pending hosted introduced connection will wait between challenge response and connect request
		timeout_check_interval = 1500, ///< Interval in milliseconds between checking for connection timeouts.
		puzzle_solution_timeout = 30000, ///< If the server gives us a puzzle that takes more than 30 seconds, time out.
		introduction_timeout = 30000, ///< Amount of time the introducer tracks a connection introduction request.
	};
	
	enum disconnect_reason
	{
		reason_failed_puzzle,
		reason_self_disconnect,
		reason_shutdown,
		reason_reconnecting,
		reason_disconnect_call,
	};
	
	/// Computes an identity token for the connecting client based on the address of the client and the client's unique nonce value.
	uint32 compute_client_identity_token(const address &the_address, const nonce &the_nonce)
	{
		hash_state state;
		uint32 hash[8];
		
		sha256_init(&state);
		sha256_process(&state, (uint8 *) &the_address, sizeof(address));
		sha256_process(&state, (uint8 *) &the_nonce, sizeof(the_nonce));
		sha256_process(&state, _random_hash_data, sizeof(_random_hash_data));
		sha256_done(&state, (uint8 *) hash);
		
		return hash[0];
	}

	/// Returns the address of the first network torque_socket in the list that the socket on this torque_socket is bound to.
	address get_first_bound_interface_address()
	{
		address the_address = _socket.get_bound_address();
		
		if(the_address.is_same_host(address(address::any, 0)))
		{
			array<address> torque_socket_addresses;
			address::get_interface_addresses(torque_socket_addresses);
			uint16 save_port = the_address.get_port();
			if(torque_socket_addresses.size())
			{
				the_address = torque_socket_addresses[0];
				the_address.set_port(save_port);
			}
		}
		return the_address;
	}
	
	/// returns the current process time for this torque_socket
	time get_process_start_time()
	{
		return _process_start_time;
	}
	
	/// Sends a connect challenge request on behalf of the connection to the remote host.
	void _send_challenge_request(pending_connection *the_connection)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Connect Challenge Request to %s", the_connection->get_address().to_string().c_str()));
		packet_stream out;
		core::write(out, uint8(connect_challenge_request_packet));
		core::write(out, the_connection->get_initiator_nonce());
		out.send_to(_socket, the_connection->get_address());
	}
	
	/// Handles a connect challenge request by replying to the requestor of a connection with a unique token for that connection, as well as (possibly) a client puzzle (for DoS prevention), or this torque_socket's public key.
	void _handle_connect_challenge_request(const address &addr, bit_stream &stream)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Challenge Request from %s", addr.to_string().c_str()));
		
		if(!_allow_connections)
			return;
		
		nonce initiator_nonce;
		core::read(stream, initiator_nonce);

		// In the case of an introduced connection we will already have a pending connenection for this address.  If so, validate that the nonce is correct.
		pending_connection* conn = _find_pending_connection(addr);
		if(conn)
		{
			if(initiator_nonce != conn->get_initiator_nonce())
				return;
			if(conn->get_type() == pending_connection::introduced_connection_host && conn->get_state() == pending_connection::sending_punch_packets)
			{
				conn->set_state(pending_connection::awaiting_connect_request);
				conn->_state_send_retry_count = 0;
				conn->_state_send_retry_interval = introduced_connection_connect_timeout;
				conn->_state_last_send_time = get_process_start_time();
			}
		}
		_send_connect_challenge_response(addr, initiator_nonce);
	}
	
	/// Sends a connect challenge request to the specified address.  This can happen as a result of receiving a connect challenge request, or during an "arranged" connection for the non-initiator of the connection.
	void _send_connect_challenge_response(const address &addr, nonce &initiator_nonce)
	{
		packet_stream out;
		core::write(out, uint8(connect_challenge_response_packet));
		core::write(out, initiator_nonce);
		
		uint32 identity_token = compute_client_identity_token(addr, initiator_nonce);
		core::write(out, identity_token);
		
		// write out a client puzzle
		uint32 difficulty = _puzzle_manager.get_current_difficulty();
		core::write(out, _puzzle_manager.get_current_nonce());
		core::write(out, difficulty);
		core::write(out, _private_key->get_public_key());
		core::write(out, _challenge_response);

		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Challenge Response: %8x", identity_token));
		out.send_to(_socket, addr);
	}
	
	/// Processes a connect_challenge_response; if it's correctly formed and for a pending connection that is requesting_challenge_response, post a challenge_response event and awayt a local_challenge_accept.
	
	void _handle_connect_challenge_response(const address &the_address, bit_stream &stream)
	{
		pending_connection *conn = _find_pending_connection(the_address);
		if(!conn || conn->get_state() != pending_connection::requesting_challenge_response)
			return;
		
		nonce initiator_nonce, host_nonce;
		core::read(stream, initiator_nonce);
		
		if(initiator_nonce != conn->get_initiator_nonce())
			return;

		core::read(stream, conn->_client_identity);
		
		// see if the server wants us to solve a client puzzle
		core::read(stream, conn->get_host_nonce());
		core::read(stream, conn->_puzzle_difficulty);
		
		if(conn->_puzzle_difficulty > client_puzzle_manager::max_puzzle_difficulty)
			return;
		
		conn->_public_key = new asymmetric_key(stream);
		if(!conn->_public_key->is_valid())
			return;

		if(_private_key.is_null() || _private_key->get_key_size() != conn->_public_key->get_key_size())
		{
			// we don't have a private key, so generate one for this connection
			conn->_private_key = new asymmetric_key(conn->_public_key->get_key_size());
		}
		else
			conn->_private_key = _private_key;
		conn->set_shared_secret(conn->_private_key->compute_shared_secret_key(conn->_public_key));
		//logprintf("shared secret (client) %s", conn->get_shared_secret()->encodeBase64()->get_buffer());
		_random_generator.random_buffer(conn->_symmetric_key, symmetric_cipher::key_size);

		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Challenge Response: %8x", conn->_client_identity ));

		byte_buffer_ptr response_data;
		core::read(stream, response_data);

		torque_socket_event *event = _allocate_queued_event();
		event->event_type = torque_connection_challenge_response_event_type;

		event->public_key_size = conn->_public_key->get_public_key()->get_buffer_size();
		event->public_key = _allocate_queue_data(event->public_key_size);
		memcpy(event->public_key, conn->_public_key->get_public_key()->get_buffer(), event->public_key_size);

		event->connection = conn->_connection_index;
		event->data_size = response_data->get_buffer_size();
		event->data = _allocate_queue_data(event->data_size);
		
		memcpy(event->data, response_data->get_buffer(), event->data_size);
		conn->set_state(pending_connection::awaiting_local_challenge_accept);
		conn->_state_send_retry_count = 0;
		conn->_state_send_retry_interval = introduction_timeout;
		conn->_state_last_send_time = get_process_start_time();
	}
	
	/// Sends a connect request on behalf of a pending connection.
	void _send_connect_request(pending_connection *conn)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Connect Request"));
		packet_stream out;
		
		core::write(out, uint8(connect_request_packet));
		core::write(out, conn->get_initiator_nonce());
		core::write(out, conn->get_host_nonce());
		core::write(out, conn->_client_identity);
		core::write(out, conn->_puzzle_difficulty);
		core::write(out, conn->_puzzle_solution);
		
		uint32 encrypt_pos = 0;
	
		core::write(out, conn->_private_key->get_public_key());
		encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(encrypt_pos);
		out.write_bytes(conn->_symmetric_key, symmetric_cipher::key_size);

		core::write(out, conn->get_initial_send_sequence());
		core::write(out, conn->_packet_data);
		
		// Write a hash of everything written into the packet, then  symmetrically encrypt the packet from the end of the public key to the end of the signature.
		symmetric_cipher the_cipher(conn->get_shared_secret());
		bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);
		out.send_to(_socket, conn->get_address());
	}
	
	/// Handles a connection request from a remote host.
	///
	/// This will verify the validity of the connection token, as well as any solution to a client puzzle this torque_socket sent to the remote host.  If those tests pass, and there is not an existing pending connection in awaiting_connect_request state it will construct a pending connection instance to track the rest of the connection negotiation.
	void _handle_connect_request(const address &the_address, bit_stream &stream)
	{
		nonce initiator_nonce;
		nonce host_nonce;
		
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		
		// check if this connection has already been accepted:
		torque_connection *existing = _find_connection(the_address);
		if(existing && existing->get_initiator_nonce() == initiator_nonce && existing->get_host_nonce() == host_nonce)
			_send_connect_accept(existing);
		
		// see if there's a pending connection from that address
		pending_connection *pending = _find_pending_connection(the_address);
		
		// if the pending connection has the same nonces and is in an awaiting_local_accept state, assume this is a duplicated connection request packet
		if(pending && pending->get_state() == pending_connection::awaiting_local_accept && pending->get_initiator_nonce() == initiator_nonce && pending->get_host_nonce() == host_nonce)
			return;
		
		// if anonymous connections are not allowed, there must be a pending introduced connection waiting for a connect request
		if(!_allow_connections && !(pending && pending->get_state() == pending_connection::awaiting_connect_request))
			return;

		uint32 client_identity;
		core::read(stream, client_identity);		
		if(client_identity != compute_client_identity_token(the_address, initiator_nonce))
		{
			TorqueLogMessageFormatted(LogNettorque_socket, ("Client identity disagreement, params say %i, I say %i", client_identity, compute_client_identity_token(the_address, initiator_nonce)));
			return;
		}

		uint32 puzzle_difficulty;
		uint32 puzzle_solution;
		core::read(stream, puzzle_difficulty);
		core::read(stream, puzzle_solution);
		
		// check the puzzle solution
		client_puzzle_manager::result_code result = _puzzle_manager.check_solution(puzzle_solution, initiator_nonce, host_nonce, puzzle_difficulty, client_identity);
		
		if(result != client_puzzle_manager::success)
		{
			_send_connect_reject(initiator_nonce, host_nonce, the_address, reason_failed_puzzle);
			return;
		}
		if(pending && (pending->get_initiator_nonce() != initiator_nonce || pending->get_host_nonce() != host_nonce))
			return;
		
		if(_private_key.is_null())
			return;
		
		ref_ptr<asymmetric_key> public_key = new asymmetric_key(stream);
		uint32 decrypt_pos = stream.get_next_byte_position();
		
		stream.set_byte_position(decrypt_pos);
		byte_buffer_ptr shared_secret = _private_key->compute_shared_secret_key(public_key);
		//logprintf("shared secret (server) %s", shared_secret->encodeBase64()->get_buffer());
		
		symmetric_cipher the_cipher(shared_secret);
		
		if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;
		
		if(!pending)
			pending = new pending_connection(pending_connection::connection_host, initiator_nonce, _random_generator.random_integer(), _next_connection_index++);
		
		// now read the first part of the connection's symmetric key
		stream.read_bytes(pending->_symmetric_key, symmetric_cipher::key_size);
		_random_generator.random_buffer(pending->_init_vector, symmetric_cipher::key_size);
		
		uint32 connect_sequence;
		core::read(stream, connect_sequence);
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Request %8x", client_identity));
		
		if(existing)
			_disconnect(existing->get_connection_index(), reason_self_disconnect, 0, 0);
				
		pending->_host_nonce = host_nonce;
		pending->set_shared_secret(shared_secret);
		pending->set_address(the_address);
		pending->set_initial_recv_sequence(connect_sequence);
		
		pending->set_symmetric_cipher(new symmetric_cipher(pending->_symmetric_key, pending->_init_vector));
		
		byte_buffer_ptr connect_request_data;
		core::read(stream, connect_request_data);

		_add_pending_connection(pending);

		torque_socket_event *event;
		event = _allocate_queued_event();
		
		event->event_type = torque_connection_requested_event_type;

		event->public_key_size = public_key->get_public_key()->get_buffer_size();
		event->public_key = _allocate_queue_data(event->public_key_size);
		
		memcpy(event->public_key, public_key->get_public_key()->get_buffer(), event->public_key_size);

		event->connection = pending->_connection_index;
		event->data_size = connect_request_data->get_buffer_size();
		event->data = _allocate_queue_data(event->data_size);
		memcpy(event->data, connect_request_data->get_buffer(), event->data_size);
	}
	
	/// Sends a connect accept packet to acknowledge the successful acceptance of a connect request.
	void _send_connect_accept(torque_connection *conn)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Connect Accept - connection established."));
		packet_stream out;
		core::write(out, uint8(connect_accept_packet));
		
		core::write(out, conn->get_initiator_nonce());
		core::write(out, conn->get_host_nonce());
		uint32 encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(encrypt_pos);
		
		core::write(out, conn->get_initial_send_sequence());

		uint8 init_vector[symmetric_cipher::block_size];
		conn->get_symmetric_cipher()->get_init_vector(init_vector);
		out.write_bytes(init_vector, symmetric_cipher::key_size);
		
		symmetric_cipher the_cipher(conn->get_shared_secret());
		bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);

		out.send_to(_socket, conn->get_address());
	}
	
	/// Handles a connect accept packet, putting the connection associated with the remote host (if there is one) into an active state.
	void _handle_connect_accept(const address &the_address, bit_stream &stream)
	{
		nonce initiator_nonce, host_nonce;
		
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		uint32 decrypt_pos = stream.get_next_byte_position();
		stream.set_byte_position(decrypt_pos);
		
		pending_connection *pending = _find_pending_connection(the_address);
		if(!pending || pending->get_state() != pending_connection::requesting_connection || pending->get_initiator_nonce() != initiator_nonce || pending->get_host_nonce() != host_nonce)
			return;
		
		symmetric_cipher the_cipher(pending->get_shared_secret());

		if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;

		uint32 recv_sequence;
		core::read(stream, recv_sequence);
		
		uint8 init_vector[symmetric_cipher::block_size];
		
		stream.read_bytes(init_vector, symmetric_cipher::block_size);
		symmetric_cipher *cipher = new symmetric_cipher(pending->_symmetric_key, init_vector);
		
		torque_connection *the_connection = new torque_connection(pending->get_initiator_nonce(), pending->get_initial_send_sequence(), pending->_connection_index, true);
		the_connection->set_initial_recv_sequence(recv_sequence);
		the_connection->set_address(pending->get_address());
		the_connection->_host_nonce = pending->_host_nonce;
		the_connection->set_torque_socket(this);
		_remove_pending_connection(pending); // remove the pending connection

		_add_connection(the_connection); // first, add it as a regular connection
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Accept - connection established."));

		torque_socket_event *event = _allocate_queued_event();
		event->event_type = torque_connection_accepted_event_type;
		event->connection = the_connection->get_connection_index();
		event->data_size = 0;
		event->data = 0;
	
		_post_connection_event(the_connection->get_connection_index(), torque_connection_established_event_type, 0);
	}
	
	/// Sends a connect rejection to a valid connect request in response to possible error conditions (server full, wrong password, etc).
	void _send_connect_reject(nonce &initiator_nonce, nonce &host_nonce, const address &the_address, uint32 reason)
	{
		packet_stream out;
		core::write(out, uint8(connect_reject_packet));
		core::write(out, initiator_nonce);
		core::write(out, host_nonce);
		core::write(out, reason);
		out.send_to(_socket, the_address);
	}
	
	
	/// Handles a connect rejection packet by notifying the connection ref_object that the connection was rejected.
	void _handle_connect_reject(const address &the_address, bit_stream &stream)
	{
		nonce initiator_nonce;
		nonce host_nonce;
		
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		
		pending_connection *pending = _find_pending_connection(the_address);
		if(!pending || (pending->get_state() != pending_connection::requesting_challenge_response &&
					 pending->get_state() != pending_connection::requesting_connection))
			return;
		if(pending->get_initiator_nonce() != initiator_nonce || pending->get_host_nonce() != host_nonce)
			return;
		
		uint32 reason;
		core::read(stream, reason);
		
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Reject - reason %d", reason));

		// if the reason is a bad puzzle solution, try once more with a
		// new nonce.
		if(reason == reason_failed_puzzle && !pending->_puzzle_retried)
		{
			pending->_puzzle_retried = true;
			pending->set_state(pending_connection::requesting_challenge_response);
			pending->_state_send_retry_count = challenge_retry_count;
			pending->_state_send_retry_interval = challenge_retry_time;
			pending->_state_last_send_time = get_process_start_time();
			pending->_initiator_nonce = _random_generator.random_nonce();
			
			_send_challenge_request(pending);
			return;
		}
		byte_buffer_ptr null;

		_post_connection_event(pending->_connection_index, torque_connection_disconnected_event_type, null);
		_remove_pending_connection(pending);
	}
	
	/// Dispatches a disconnect packet for a specified connection.
	void _handle_disconnect(const address &the_address, bit_stream &stream)
	{
		nonce initiator_nonce, host_nonce;
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		uint32 reason_code;
		uint32 disconnect_data_size;
		uint8 disconnect_data[torque_sockets_max_status_datagram_size];
		
		torque_connection *conn = _find_connection(the_address);
		if(conn)
		{
			if(initiator_nonce != conn->get_initiator_nonce() || host_nonce != conn->get_host_nonce())
				return;
			
			uint32 decrypt_pos = stream.get_next_byte_position();
			stream.set_byte_position(decrypt_pos);
			
			symmetric_cipher the_cipher(conn->get_shared_secret());
			if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, decrypt_pos, &the_cipher))
				return;
			core::read(stream, reason_code);
			core::read(stream, disconnect_data_size);
			if(disconnect_data_size > torque_sockets_max_status_datagram_size)
				disconnect_data_size = torque_sockets_max_status_datagram_size;
			stream.read_bytes(disconnect_data, disconnect_data_size);
			byte_buffer_ptr disconnect_data_buffer = new byte_buffer(disconnect_data, disconnect_data_size);
			_post_connection_event(conn->get_connection_index(), torque_connection_disconnected_event_type, disconnect_data_buffer);
			_remove_connection(conn);
		}
		else
		{
			pending_connection *pending = _find_pending_connection(the_address);
			if(!pending)
				return;
			if(pending->get_initiator_nonce() != initiator_nonce || pending->get_host_nonce() != host_nonce)
				return;
			_post_connection_event(conn->get_connection_index(), torque_connection_disconnected_event_type, 0);
			_remove_pending_connection(pending);
		}
	}
	
	/// Handles all packets that don't fall into the category of torque_connection handshake or game data.
	void _handle_info_packet(const address &address, uint8 packet_type, bit_stream &stream)
	{
		torque_socket_event *event = _allocate_queued_event();
		event->event_type = torque_socket_packet_event_type;
		event->data_size = stream.get_stream_byte_size();
		event->data = (uint8 *) _allocate_queue_data(event->data_size);
		memcpy(event->data, stream.get_buffer(), event->data_size);
		address.to_sockaddr(&event->source_address);
	}
	
	/// Processes a single packet, and dispatches either to handle_info_packet or to the connection associated with the remote address.
	void _process_packet(const address &the_address, bit_stream &packet_stream)
	{
		
		// Determine what to do with this packet:
		
		if(packet_stream.get_buffer()[0] & 0x80) // it's a protocol packet...
		{
			// if the MSB of the first byte is set, it's a protocol data packet so pass it to the appropriate connection.
			logprintf("got data packet");
			torque_connection *conn = _find_connection(the_address);
			if(conn)
			{
				logprintf("got data packet on conn");
				conn->read_raw_packet(packet_stream);
			}
		}
		else
		{
			// Otherwise, it's either a game info packet or a connection handshake packet.
			
			uint8 packet_type;
			core::read(packet_stream, packet_type);
			
			if(packet_type >= first_valid_info_packet_id)
				_handle_info_packet(the_address, packet_type, packet_stream);
			else
			{
				// check if there's a connection already:
				switch(packet_type)
				{
					case connect_challenge_request_packet:
						_handle_connect_challenge_request(the_address, packet_stream);
						break;
					case connect_challenge_response_packet:
						_handle_connect_challenge_response(the_address, packet_stream);
						break;
					case connect_reject_packet:
						_handle_connect_reject(the_address, packet_stream);
						break;
					case connect_request_packet:
						_handle_connect_request(the_address, packet_stream);
						break;
					case connect_accept_packet:
						_handle_connect_accept(the_address, packet_stream);
						break;
					case disconnect_packet:
						_handle_disconnect(the_address, packet_stream);
						break;
					case introduced_connection_request_packet:
						_handle_introduced_connection_request(the_address, packet_stream);
						break;
					case connection_introduction_packet:
						_handle_connection_introduction(the_address, packet_stream);
						break;
					case punch_packet:
						_handle_punch(the_address, packet_stream);
						break;
				}
			}
		}
	}
	void _handle_introduced_connection_request(const address &the_address, bit_stream &packet_stream)
	{

	}

	void _handle_connection_introduction(const address &the_address, bit_stream &packet_stream)
	{

	}

	void _handle_punch(const address &the_address, bit_stream &packet_stream)
	{

	}

	void _send_introduction_request(pending_connection *the_connection)
	{

	}

	void _send_punch(pending_connection *the_connection)
	{

	}

protected:
	/// Structure used to track packets that are delayed in sending for simulating a high-latency connection.  The delay_send_packet is allocated as sizeof(delay_send_packet) + packet_size;
	struct delay_send_packet
	{
		delay_send_packet *next_packet; ///< The next packet in the list of delayed packets.
		address remote_address; ///< The address to send this packet to.
		time send_time; ///< time when we should send the packet.
		uint32 packet_size; ///< Size, in bytes, of the packet data.
		uint8 packet_data[1]; ///< Packet data.
	};
	
	struct event_queue_entry {
		torque_socket_event *event;
		event_queue_entry *next_event;
	};
	
	torque_socket_event *_allocate_queued_event()
	{
		torque_socket_event *ret = (torque_socket_event *) _event_queue_allocator.allocate(sizeof(torque_socket_event));
		event_queue_entry *entry = (event_queue_entry *) _event_queue_allocator.allocate(sizeof(event_queue_entry));
		
		entry->event = ret;
		
		ret->data = 0;
		ret->public_key = 0;
		ret->data_size = 0;
		ret->public_key_size = 0;
		
		entry->next_event = 0;
		if(_event_queue_tail)
		{
			_event_queue_tail->next_event = entry;
			_event_queue_tail = entry;
		}
		else
			_event_queue_head = _event_queue_tail = entry;
		return entry->event;
	}
	
	uint8 *_allocate_queue_data(uint32 data_size)
	{
		return (uint8 *) _event_queue_allocator.allocate(data_size);
	}
	
	torque_socket_event *_post_connection_event(torque_connection_id conn, uint32 event_type, byte_buffer_ptr data)
	{
		torque_socket_event *event = _allocate_queued_event();
		event->event_type = event_type;
		event->connection = conn;
		if(data)
		{
			event->data_size = data->get_buffer_size();
			event->data = _allocate_queue_data(event->data_size);
			memcpy(event->data, data->get_buffer(), event->data_size);
		}
		return event;
	}
	
	/// Checks all connections on this torque_socket for packet sends, and for timeouts and all valid and pending connections.
	void process_connections()
	{
		_process_start_time = time::get_current();
		_puzzle_manager.tick(_process_start_time, _random_generator);
		
		// first see if there are any delayed packets that need to be sent...
		while(_send_packet_list && _send_packet_list->send_time < get_process_start_time())
		{
			delay_send_packet *next = _send_packet_list->next_packet;
			_socket.send_to(_send_packet_list->remote_address,
							_send_packet_list->packet_data, _send_packet_list->packet_size);
			memory_deallocate(_send_packet_list);
			_send_packet_list = next;
		}
		
		if(get_process_start_time() > _last_timeout_check_time + time(timeout_check_interval))
		{
			_last_timeout_check_time = get_process_start_time();
			for(pending_connection **walk = &_pending_connections; *walk;)
			{
				pending_connection *pending = *walk;
				if(get_process_start_time() > pending->_state_last_send_time + time(pending->_state_send_retry_interval))
				{
					if(!pending->_state_send_retry_count)
					{
						// this pending connection request has timed out.
						_post_connection_event(pending->_connection_index, torque_connection_timed_out_event_type, 0);
						*walk = pending->_next;
						delete pending;
					}
					else
					{
						pending->_state_send_retry_count--;
						switch(pending->get_state())
						{
							case pending_connection::requesting_introduction:
								_send_introduction_request(pending);
								break;
							case pending_connection::sending_punch_packets:
								_send_punch(pending);
								break;
						case pending_connection::requesting_challenge_response:
								_send_challenge_request(pending);
						}
						walk = &pending->_next;
					}
				}
				else
					walk = &pending->_next;

			}
			for(torque_connection *connection_walk = _connection_list; connection_walk;)
			{
				torque_connection *the_connection = connection_walk;
				connection_walk = connection_walk->_next;

				if(the_connection->check_timeout(get_process_start_time()))
				{
					_post_connection_event(the_connection->_connection_index, torque_connection_timed_out_event_type, 0);
					_remove_connection(the_connection);
				}
			}
		}
		
		// check if we're trying to solve any client connection puzzles
		byte_buffer_ptr result;
		uint32 request_index;
		while(_puzzle_solver.get_next_result(result, request_index))
		{
			uint32 solution;
			bit_stream s(result->get_buffer(), result->get_buffer_size());
			core::read(s, solution);
			
			for(pending_connection *walk = _pending_connections; walk; walk = walk->_next)
			{
				if(walk->get_state() == pending_connection::computing_puzzle_solution)
				{
					if(walk->_puzzle_request_index != request_index)
						continue;
					// this was the solution for this client...
					walk->_puzzle_solution = solution;
					
					walk->set_state(pending_connection::requesting_connection);
					_send_connect_request(walk);
					break;
				}
			}
		}
	}
	
	/// looks up a connected connection on this torque_socket
	torque_connection *_find_connection(const address &remote_address)
	{
		// TESTING -- this isn't right.
		if(_connection_list)
			return _connection_list;
		
		hash_table_flat<address, torque_connection *>::pointer p = _connection_address_lookup_table.find(remote_address);
		logprintf("finding connection for %s", remote_address.to_string().c_str());
		if(p)
			*(p.value());
		return 0;
	}
	
	torque_connection *_find_connection(torque_connection_id id)
	{
		hash_table_flat<torque_connection_id, torque_connection *>::pointer p = _connection_id_lookup_table.find(id);
		if(p)
			return *(p.value());
		return 0;
	}
	
	/// Finds a connection instance that this torque_socket has initiated.
	pending_connection *_find_pending_connection(const address &the_address)
	{
		for(pending_connection *walk = _pending_connections; walk; walk = walk->_next)
			if(walk->get_address() == the_address)
				return walk;
		return NULL;
	}
	
	pending_connection * _find_pending_connection(torque_connection_id connection_id)
	{
		for(pending_connection *walk = _pending_connections; walk; walk = walk->_next)
			if(walk->_connection_index == connection_id)
				return walk;
		return 0;
	}
	
	void _remove_pending_connection(pending_connection *the_connection)
	{
		for(pending_connection **walk = &_pending_connections; *walk; walk = &((*walk)->_next))
			if(*walk == the_connection)
			{
				*walk = the_connection->_next;
				delete the_connection;
				return;
			}
	}
	
	/// Adds a pending connection the list of pending connections.
	void _add_pending_connection(pending_connection *the_connection)
	{
		the_connection->_next = _pending_connections;
		_pending_connections = the_connection;
	}
	
	/// Adds a connection to the internal connection list.
	void _add_connection(torque_connection *the_connection)
	{
		the_connection->_next = _connection_list;
		the_connection->_prev = 0;
		if(the_connection->_next)
			the_connection->_next->_prev = the_connection;
		_connection_list = the_connection;
		
		_connection_id_lookup_table.insert(the_connection->_connection_index, the_connection);
		logprintf("inserting connection %d at %s", the_connection->_connection_index, the_connection->get_address().to_string().c_str());
		_connection_address_lookup_table.insert(the_connection->get_address(), the_connection);
	}
	
	void _remove_connection(torque_connection *the_connection)
	{
		if(the_connection->_prev)
			the_connection->_prev->_next = the_connection->_next;
		else
			_connection_list = the_connection->_next;
		if(the_connection->_next)
			the_connection->_next->_prev = the_connection->_prev;
		
		_connection_id_lookup_table.remove(the_connection->get_connection_index());
		_connection_address_lookup_table.remove(the_connection->get_address());
		delete the_connection;
	}
public:
	/// Sets the private key this torque_socket will use for authentication and key exchange
	void set_private_key(asymmetric_key *the_key)
	{
		_private_key = the_key;
	}
	
	/// Returns the udp_socket associated with this torque_socket
	udp_socket &get_network_socket()
	{
		return _socket;
	}	
	
	void set_challenge_response(byte_buffer_ptr data)
	{
		_challenge_response = data;
	}
	
	random_generator &random()
	{
		return _random_generator;
	}
	
	/// Returns whether or not this torque_socket allows connections from remote hosts.
	bool does_allow_connections()
	{
		return _allow_connections;
	}
	
	/// Sets whether or not this torque_socket allows connections from remote hosts.
	void set_allows_connections(bool conn)
	{
		_allow_connections = conn;
	}
	
	void _disconnect_existing_connection(const address &remote_host)
	{
		
	}
	
	/// open a connection to the remote host
	torque_connection_id connect(const address &remote_host, uint8 *connect_data, uint32 connect_data_size)
	{
		_disconnect_existing_connection(remote_host);
		uint32 initial_send_sequence = _random_generator.random_integer();
		
		pending_connection *new_connection = new pending_connection(pending_connection::connection_initiator, _random_generator.random_nonce(), initial_send_sequence, _next_connection_index++);
		
		new_connection->_packet_data = new byte_buffer(connect_data, connect_data_size);
		new_connection->_address = remote_host;
		new_connection->_state_send_retry_count = challenge_retry_count;
		new_connection->_state_send_retry_interval = challenge_retry_time;
		new_connection->_state_last_send_time = get_process_start_time();
		
		_add_pending_connection(new_connection);
		_send_challenge_request(new_connection);
		return new_connection->_connection_index;
	}
	
	/// This is called on the middleman of an introduced connection and will allow this host to broker a connection start between the remote hosts at either connection point.
	void introduce_connection(torque_connection_id connection_a, torque_connection_id connection_b, unsigned connection_token)
	{
		
	}
	
	/// Connect to a client connected to the host at middle_man.
	torque_connection_id connect_introduced(torque_connection_id introducer, unsigned remote_client_identity, unsigned connection_token, uint32 connect_data_size, uint8 *connect_data)
	{
		/*/// Connects to a remote host that is also connecting to this torque_connection (negotiated by a third party)
		 void connect_arranged(torque_socket *connection_torque_socket, const array<address> &possible_addresses, nonce &my_nonce, nonce &remote_nonce, byte_buffer_ptr shared_secret, bool is_initiator)
		 {
		 _connection_parameters._possible_addresses = possible_addresses;
		 _connection_parameters._is_initiator = is_initiator;
		 _connection_parameters._is_arranged = true;
		 _connection_parameters._nonce = my_nonce;
		 _connection_parameters._server_nonce = remote_nonce;
		 _connection_parameters._arranged_secret = shared_secret;
		 
		 set_torque_socket(connection_torque_socket);
		 _torque_socket->start_arranged_connection(this);
		 }*/
		
		
	}
	void accept_connection_challenge(torque_connection_id the_connection)
	{
		pending_connection *conn = _find_pending_connection(the_connection);
		if(!conn || conn->get_state() != pending_connection::awaiting_local_challenge_accept)
			return;
		
		conn->set_state(pending_connection::computing_puzzle_solution);
		conn->_state_send_retry_count = 0;
		conn->_state_send_retry_interval = puzzle_solution_timeout;
		conn->_state_last_send_time = get_process_start_time();
		
		packet_stream s;
		core::write(s, conn->get_initiator_nonce());
		core::write(s, conn->get_host_nonce());
		core::write(s, conn->_puzzle_difficulty);
		core::write(s, conn->_client_identity);
		logprintf("Attempting to solve a client puzzle.");
		byte_buffer_ptr request = new byte_buffer(s.get_buffer(), s.get_next_byte_position());		
		conn->_puzzle_request_index = _puzzle_solver.post_request(request);
	}
	
	/// accept an incoming connection request.
	void accept_connection(torque_connection_id connection_id)
	{
		pending_connection* pending = _find_pending_connection(connection_id);
		if(!pending || pending->get_state() != pending_connection::awaiting_local_accept)
		{
			TorqueLogMessageFormatted(LogNettorque_socket, ("Trying to accept a non-pending connection."));
			return;
		}
		torque_connection *new_connection = new torque_connection(pending->_initiator_nonce, pending->_initial_send_sequence, pending->_connection_index, false);
		new_connection->set_torque_socket(this);
		new_connection->set_symmetric_cipher(pending->get_symmetric_cipher());
		new_connection->set_shared_secret(pending->get_shared_secret());
		new_connection->set_initial_recv_sequence(pending->_initial_recv_sequence);
		new_connection->_host_nonce = pending->_host_nonce;
		new_connection->set_address(pending->get_address());
		
		_remove_pending_connection(pending);
		_add_connection(new_connection);
		_post_connection_event(new_connection->_connection_index, torque_connection_established_event_type, 0);
		_send_connect_accept(new_connection);
	}
	
	void disconnect(torque_connection_id connection_id, uint8 *disconnect_data, uint32 disconnect_data_size)
	{
		_disconnect(connection_id, reason_disconnect_call, disconnect_data, disconnect_data_size);
		
	}
	
	
	/// Disconnects the given torque_connection and removes it from the torque_socket
	void _disconnect(torque_connection_id connection_id, uint32 reason_code, uint8 *disconnect_data, uint32 disconnect_data_size)
	{
		assert(disconnect_data_size <= torque_sockets_max_status_datagram_size);
		
		// see if it's a connected instance
		torque_connection *connection = _find_connection(connection_id);
		if(connection)
		{
			// send a disconnect packet...
			packet_stream out;
			core::write(out, uint8(disconnect_packet));
			core::write(out, connection->get_initiator_nonce());
			core::write(out, connection->get_host_nonce());
			uint32 encrypt_pos = out.get_next_byte_position();
			out.set_byte_position(encrypt_pos);
			
			core::write(out, reason_code);
			core::write(out, disconnect_data_size);
			out.write_bytes(disconnect_data, disconnect_data_size);
			
			symmetric_cipher the_cipher(connection->get_shared_secret());
			bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);
			
			out.send_to(_socket, connection->get_address());
			
			_remove_connection(connection);
			return;
		}
		pending_connection *pending = _find_pending_connection(connection_id);
		if(pending)
		{
			_remove_pending_connection(pending);
		}
	}
	/// Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.
	void send_to_connection(torque_connection_id connection_id, uint8 *data, uint32 data_size, uint32 *sequence = 0)
	{
		torque_connection *conn = _find_connection(connection_id);
		conn->send_packet(torque_connection::data_packet, data, data_size, sequence);
	}
	
	/// Sends a packet to the remote address over this torque_socket's socket.
	udp_socket::send_to_result send_to(const address &the_address, uint32 data_size, uint8 *data)
	{
		string addr_string = the_address.to_string();
		
		logprintf("send: %s %s", addr_string.c_str(), buffer_encode_base_16(data, data_size)->get_buffer());
		
		return _socket.send_to(the_address, data, data_size);
	}
	
	/// Sends a packet to the remote address after millisecond_delay time has elapsed.  This is used to simulate network latency on a LAN or single computer.
	void send_to_delayed(const address &the_address, bit_stream &stream, uint32 millisecond_delay)
	{
		uint32 data_size = stream.get_next_byte_position();
		
		// allocate the send packet, with the data size added on
		delay_send_packet *the_packet = (delay_send_packet *) memory_allocate(sizeof(delay_send_packet) + data_size);
		the_packet->remote_address = the_address;
		the_packet->send_time = get_process_start_time() + time(millisecond_delay);
		the_packet->packet_size = data_size;
		memcpy(the_packet->packet_data, stream.get_buffer(), data_size);
		
		// insert it into the delay_send_packet list, sorted by time
		delay_send_packet **list;
		for(list = &_send_packet_list; *list && ((*list)->send_time <= the_packet->send_time); list = &((*list)->next_packet))
			;
		the_packet->next_packet = *list;
		*list = the_packet;
	}
	
	/// Gets the next event on this socket; returns NULL if there are no events to be read.
	torque_socket_event *get_next_event()
	{
		process_connections();
		if(!_event_queue_head)
		{
			_event_queue_allocator.clear();
			// if there's nothing in the event queue, see if a new packet's come in.
			packet_stream stream;
			udp_socket::recv_from_result result;
			address sourceAddress;
			
			while((result = stream.recv_from(_socket, &sourceAddress)) == udp_socket::packet_received)
			{
				logprintf("Got a packet.");
				_process_start_time = time::get_current();
				_process_packet(sourceAddress, stream);
				if(_event_queue_head)
					break;
			}
		}
		if(_event_queue_head)
		{
			torque_socket_event *ret = _event_queue_head->event;
			_event_queue_head = _event_queue_head->next_event;
			logprintf("returning event of type %d", ret->event_type);
			if(!_event_queue_head)
				_event_queue_tail = 0;
			return ret;
		}
		else
			return 0;
	}	
	
	~torque_socket()
	{
		// gracefully close all the connections on this torque_socket:
		while(_connection_list)
			_disconnect(_connection_list->get_connection_index(), reason_self_disconnect, 0, 0);
	}
	
	/// @param bind_address Local network address to bind this torque_socket to.
	torque_socket(const address &bind_address) : _puzzle_manager(_random_generator, &_allocator), _event_queue_allocator(&_allocator)
	{
		_next_connection_index = 1;
		_random_generator.random_buffer(_random_hash_data, sizeof(_random_hash_data));

		_private_key = new asymmetric_key(20, _random_generator);
		
		_last_timeout_check_time = time(0);
		_allow_connections = true;
		
		_send_packet_list = NULL;
		_process_start_time = time::get_current();
		
		udp_socket::bind_result res = _socket.bind(bind_address);
		
		// Supply our own (small) unique private key for the time being.
		_private_key = new asymmetric_key(16, _random_generator);
		_challenge_response = new byte_buffer();
		_event_queue_head = _event_queue_tail = 0;
	}
		
	udp_socket _socket; ///< Network socket this torque_socket communicates over.
	random_generator _random_generator;	///< cryptographic random number generator for this socket
	puzzle_solver _puzzle_solver; ///< helper class for solving client puzzles
	zone_allocator _allocator; ///< memory allocator helper class for this socket

	pending_connection *_pending_connections; ///< Linked list of all the pending connections on this socket
	torque_connection *_connection_list; ///< Doubly-linked list of all the connections that are in a connected state on this torque_socket.
	hash_table_flat<torque_connection_id, torque_connection *> _connection_id_lookup_table; ///< quick lookup table for active connections by id.
	hash_table_flat<address, torque_connection *> _connection_address_lookup_table; ///< quick lookup table for active connections by address.
	uint32 _next_connection_index; ///< Next available connection id

	byte_buffer_ptr _challenge_response; ///< Challenge response set by the host as response to all incoming challenge requests on this socket.
	
	ref_ptr<asymmetric_key> _private_key; ///< The private key used by this torque_socket for secure key exchange.
	client_puzzle_manager _puzzle_manager; ///< The ref_object that tracks the current client puzzle difficulty, current puzzle and solutions for this torque_socket.

	time _process_start_time; ///< Current time tracked by this torque_socket.
	bool _requires_key_exchange; ///< True if all connections outgoing and incoming require key exchange.
	time _last_timeout_check_time; ///< Last time all the active connections were checked for timeouts.
	uint8  _random_hash_data[12]; ///< Data that gets hashed with connect challenge requests to prevent connection spoofing.
	bool _allow_connections; ///< Set if this torque_socket allows connections from remote instances.
	
	event_queue_entry *_event_queue_tail;
	event_queue_entry *_event_queue_head;
	page_allocator<16> _event_queue_allocator;
	hash_table_flat<uint32, torque_connection *> _connection_index_table;

	delay_send_packet *_send_packet_list; ///< List of delayed packets pending to send.
};
