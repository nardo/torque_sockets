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
	
	void set_challenge_response_data(byte_buffer_ptr data)
	{
		_challenge_response_buffer = data;
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
	
	
	
	/// open a connection to the remote host
	torque_connection_id connect(const address &remote_host, uint8 *connect_data, uint32 connect_data_size)
	{
		_disconnect_existing_connection(remote_host);
		uint32 initial_send_sequence = _random_generator.random_integer();
		
		pending_connection *new_connection = new pending_connection(this, _random_generator.random_nonce(), initial_send_sequence, _next_connection_index++, pending_connection::connection_initiator, new byte_buffer(connect_data, connect_data_size));
		
		new_connection->address = remote_host;
		new_connection->_state_send_try_count = challenge_retry_count;
		new_connection->_state_send_try_interval = challenge_retry_time;
		
		add_pending_connection(new_connection);
		_send_connect_challenge_request(new_connection);
	}
	
		torque_connection *the_connection = new torque_connection(_random_generator.random_nonce(), _random_generator.random_integer(), _next_connection_index++, true);
		
		connection_parameters &params = the_connection->get_connection_parameters();
		
		find_and_remove_pending_connection(remote_host);
		add_pending_connection(the_connection);
		
		the_connection->_connect_send_count = 0;
		the_connection->set_connection_state(torque_connection::awaiting_challenge_response);
		send_connect_challenge_request(the_connection);

	
	/// This is called on the middleman of an introduced connection and will allow this host to broker a connection start between the remote hosts at either connection point.
	void introduce_connection(torque_connection_id connection_a, torque_connection_id connection_b, unsigned connection_token)
	{
		
	}
	
	/// Connect to a client connected to the host at middle_man.
	torque_connection_id connect_introduced(torque_connection_id introducer, unsigned remote_client_identity, unsigned connection_token, bit_stream &connect_data)
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
	
	/// accept an incoming connection request.
	void accept_connection(torque_connection_id connection_id, bit_stream &accept_data)
	{
		torque_connection* c = find_connection_by_id(connection_id);
		if(!c)
		{
			TorqueLogMessageFormatted(LogNettorque_socket, ("Trying to accept a non-pending connection."));
			return;
		}
		
		c->get_connection_parameters().connect_data = new byte_buffer(accept_data.get_buffer(), accept_data.get_next_byte_position());
		
		add_connection(c);
		remove_pending_connection(c);
		c->set_connection_state(torque_connection::connected);
		post_connection_event(c, torque_connection_established_event_type, 0);
		send_connect_accept(c);
	}
	
	/// reject an incoming connection request.
	void reject_connection(torque_connection_id, bit_stream &reject_data)
	{
		
	}
	void tnp_reject_connection(const address& the_address, const byte_buffer_ptr& data)
	{
		torque_connection* conn = find_pending_connection(the_address);
		if(!conn)
		{
			TorqueLogMessageFormatted(LogNettorque_socket, ("Trying to rejected a non-pending connection."));
			return;
		}
		
		send_connect_reject(conn->get_initiator_nonce(), conn->get_host_nonce(), the_address, data);
		remove_pending_connection(conn);
	}
	
	
	/// Disconnects the given torque_connection and removes it from the torque_socket
	void disconnect(torque_connection *conn, termination_reason reason, byte_buffer_ptr &reason_buffer)
	{
		/// Sends a disconnect packet to notify the remote host that this side is terminating the torque_connection for the specified reason.  This will remove the torque_connection from its torque_socket, and may have the side effect that the torque_connection is deleted, if there are no other objects with RefPtrs to the torque_connection.
		void disconnect(byte_buffer_ptr &reason)
		{
			//_torque_socket->disconnect(this, torque_socket::reason_self_disconnect, reason);
		}
		
		/// Close an open connection. 
		void disconnect(torque_connection_id, bit_stream &disconnect_data)
		{
			
		}
		
		if(conn->get_connection_state() == torque_connection::awaiting_challenge_response ||
		   conn->get_connection_state() == torque_connection::awaiting_connect_response)
		{
			post_connection_event(conn, torque_connection_disconnected_event_type, reason_buffer);
			remove_pending_connection(conn);
		}
		else if(conn->get_connection_state() == torque_connection::connected)
		{
			conn->set_connection_state(torque_connection::disconnected);
			post_connection_event(conn, torque_connection_disconnected_event_type, reason_buffer);
			
			// send a disconnect packet...
			packet_stream out;
			core::write(out, uint8(disconnect_packet));
			connection_parameters &the_params = conn->get_connection_parameters();
			core::write(out, conn->get_initiator_nonce());
			core::write(out, conn->get_host_nonce());
			uint32 encrypt_pos = out.get_next_byte_position();
			out.set_byte_position(encrypt_pos);
			core::write(out, reason_buffer);
			
			symmetric_cipher the_cipher(conn->get_shared_secret());
			bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);
			
			out.send_to(_socket, conn->get_address());
			
			remove_connection(conn);
		}
	}
	/// Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.
	void send_to_connection(torque_connection_id connection_id, bit_stream &data, uint32 *sequence = 0)
	{
		torque_connection *conn = find_connection_by_id(connection_id);
		conn->send_packet(torque_connection::data_packet, &data, sequence);
	}
	
	/// Sends a packet to the remote address over this torque_socket's socket.
	udp_socket::send_to_result send_to(const address &the_address, bit_stream &stream)
	{
		ref_ptr<byte_buffer> send_buf = buffer_encode_base_16(stream.get_buffer(), stream.get_next_byte_position());
		string addr_string = the_address.to_string();
		
		logprintf("send: %s %s", addr_string.c_str(), send_buf->get_buffer());
		
		return _socket.send_to(the_address, stream.get_buffer(), stream.get_next_byte_position());
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
				process_packet(sourceAddress, stream);
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

/// packet_type is encoded as the first byte of each packet.
	///
	/// Packets received on a socket with a first byte >= first_valid_info_packet_id and < 128 will be passed along in the form of torque_socket_packet_event events.  All other first byte values are reserved by the implementation of torque_sockets.
	
	/// Packets that arrive with the high bit of the first byte set (i.e. the first unsigned byte is greater than 127), are assumed to be connected protocol packets, and are dispatched to the appropriate connection for further processing.
	enum packet_type
	{
		connect_challenge_request_packet, ///< Initial packet of the two-phase connect process
		connect_challenge_response_packet, ///< Response packet to the ChallengeRequest containing client identity, a client puzzle, and possibly the server's public key.
		connect_request_packet, ///< A connect request packet, including all puzzle solution data and connection initiation data.
		connect_reject_packet, ///< A packet sent to notify a host that a connect_request was rejected.
		connect_accept_packet, ///< A packet sent to notify a host that a connection was accepted.
		disconnect_packet, ///< A packet sent to notify a host that the specified connection has terminated.
		punch_packet, ///< A packet sent in order to create a hole in a firewall or NAT so packets from the remote host can be received.
		arranged_connect_request_packet, ///< A connection request for an "arranged" connection.
		introduction_request_packet, ///< A connection request to be introduced to a different connection.
		send_punch_packet, ///< A request to send a punch packet to another address
		first_valid_info_packet_id, ///< The first valid ID for a torque_socket subclass's info packets.
	};
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
	};
	
	enum termination_reason
	{
		reason_timed_out,
		reason_failed_connect_handshake,
		reason_remote_host_rejected_connection,
		reason_failed_puzzle,
		reason_remote_disconnect_packet,
		reason_duplicate_connection_attempt,
		reason_self_disconnect,
		reason_error,
	};
	_old_connection_reason_buffer = new byte_buffer("OLD_CONNECTION");
	_new_connection_reason_buffer = new byte_buffer("NEW_CONNECTION");
	_reconnecting_reason_buffer = new byte_buffer("RECONNECTING");
	_shutdown_reason_buffer = new byte_buffer("SHUTDOWN");
	_timed_out_reason_buffer = new byte_buffer("TIMEDOUT");
	
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
	time get_process_start_time() { return _process_start_time; }
	
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
			for(uint32 i = 0; i < _pending_connections.size();)
			{
				torque_connection *pending = _pending_connections[i];
				
				if(pending->outstanding_request())
				{
					if(!pending->retry_request())
					{
						pending->set_connection_state(torque_connection::connect_timed_out);
						post_connection_event(pending, torque_connection_timed_out_event_type, _timed_out_reason_buffer);
						remove_pending_connection(pending);
					}
				}
				
				if(pending->get_connection_state() == torque_connection::sending_punch_packets &&
						get_process_start_time() > pending->_connect_last_send_time + time(punch_retry_time))
				{
					if(pending->_connect_send_count > punch_retry_count)
					{
						pending->set_connection_state(torque_connection::connect_timed_out);
						post_connection_event(pending, torque_connection_timed_out_event_type, _timed_out_reason_buffer);
						remove_pending_connection(pending);
						continue;
					}
					else
						send_punch_packets(pending);
				}
				else if(pending->get_connection_state() == torque_connection::computing_puzzle_solution &&
						get_process_start_time() > pending->_connect_last_send_time + 
						time(puzzle_solution_timeout))
				{
					pending->set_connection_state(torque_connection::connect_timed_out);
					post_connection_event(pending, torque_connection_timed_out_event_type, _timed_out_reason_buffer);

					remove_pending_connection(pending);
				}
				i++;
			}
			_last_timeout_check_time = get_process_start_time();
			
			for(uint32 i = 0; i < _connection_list.size();)
			{
				if(_connection_list[i]->outstanding_request())
				{
					if(!_connection_list[i]->retry_request())
					{
						_connection_list[i]->set_connection_state(torque_connection::connect_timed_out);
						post_connection_event(_connection_list[i], torque_connection_timed_out_event_type, _timed_out_reason_buffer);
						remove_connection(_connection_list[i]);
					}
				}
				
				if(_connection_list[i]->check_timeout(get_process_start_time()))
				{
					_connection_list[i]->set_connection_state(torque_connection::timed_out);
					post_connection_event(_connection_list[i], torque_connection_timed_out_event_type, _timed_out_reason_buffer);
					
					remove_connection(_connection_list[i]);
				}
				else
					i++;
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

			for(uint32 i = 0; i < _pending_connections.size(); i++)
			{
				torque_connection *conn = _pending_connections[i];
				if(conn->get_connection_state() == torque_connection::computing_puzzle_solution)
				{
					connection_parameters &the_params = conn->get_connection_parameters();
					if(the_params._puzzle_request_index != request_index)
						continue;
					// this was the solution for this client...
					the_params._puzzle_solution = solution;
					
					conn->set_connection_state(torque_connection::awaiting_connect_response);
					send_connect_request(conn);
					break;
				}
			}
		}
	}

	/// Sends a connect challenge request on behalf of the connection to the remote host.
	void _send_connect_challenge_request(pending_connection *the_connection)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Connect Challenge Request to %s", the_connection->get_address().to_string().c_str()));
		packet_stream out;
		core::write(out, uint8(connect_challenge_request_packet));
		core::write(out, the_connection->get_initiator_nonce());
		the_connection->state_send_try_count--;
		out.send_to(_socket, the_connection->get_address());
		_process_later(the_connection);
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
			if(conn->get_type() == pending_connection::introduced_connection_host() && conn->get_state() == pending_connection::sending_punch_packets)
			{
				_remove_from_process_queue(conn);
				conn->set_state(pending_connection::awaiting_connect_request);
				conn->_state_send_retry_count = 0;
				conn->_state_send_retry_interval = introduced_connection_connect_timeout;
				_process_later(conn);
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
		core::write(out, _challenge_response_buffer);

		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Challenge Response: %8x", identity_token));
		out.send_to(_socket, addr);
	}
	
	/// Processes a connect_challenge_response; if it's correctly formed and for a pending connection that is requesting_challenge_response, post a challenge_response event and awayt a local_challenge_accept.
	
	void _handle_connect_challenge_response(const address &the_address, bit_stream &stream)
	{
		pending_connection *conn = find_pending_connection(the_address);
		if(!conn || conn->get_state() != pending_connection::requesting_challenge_response)
			return;
		
		nonce initiator_nonce, host_nonce;
		core::read(stream, initiator_nonce);
		
		if(initiator_nonce != conn->get_initiator_nonce())
			return;
		
		core::read(stream, conn->_client_identity);
		
		// see if the server wants us to solve a client puzzle
		core::read(stream, conn->host_nonce);		
		core::read(stream, conn->_puzzle_difficulty);
		
		if(conn->_puzzle_difficulty > client_puzzle_manager::max_puzzle_difficulty)
			return;
		
		conn->_public_key = new asymmetric_key(stream);
		if(!conn->_public_key->is_valid())
			return;

		if(_private_key.is_null() || _private_key->get_key_size() != conn->_public_key->get_key_size())
		{
			// we don't have a private key, so generate one for this connection
			the_params._private_key = new asymmetric_key(conn->_public_key->get_key_size());
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

		event->data_size = response_data->get_buffer_size();
		event->data = _allocate_queue_data(event->data_size);
		
		memcpy(event->data, response_data->get_buffer(), event->data_size);
		conn->set_state(pending_connection::awaiting_local_challenge_accept);
		_remove_from_process_queue(conn);
	}
	
	void accept_connection_challenge(torque_connection_id the_connection)
	{
		pending_connection *conn = find_pending_connection(the_connection);
		if(!conn || conn->get_state() != pending_connection::awaiting_local_challenge_accept)
			return;
		
		conn->set_connection_state(torque_connection::computing_puzzle_solution);
		conn->_state_send_retry_count = 0;
		conn->_state_send_retry_interval = puzzle_solution_timeout;
		_process_later(conn);

		the_params._puzzle_solution = 0;
		packet_stream s;
		core::write(s, conn->get_initiator_nonce());
		core::write(s, conn->get_host_nonce());
		core::write(s, the_params._puzzle_difficulty);
		core::write(s, the_params._client_identity);
		byte_buffer_ptr request = new byte_buffer(s.get_buffer(), s.get_next_byte_position());		
		conn->_puzzle_request_index = _puzzle_solver.post_request(request);
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
		core::write(out, conn->connect_data);
		
		// Write a hash of everything written into the packet, then  symmetrically encrypt the packet from the end of the public key to the end of the signature.
		symmetric_cipher the_cipher(conn->get_shared_secret());
		bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);      
		
		conn->submit_request(out);
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
		
		pending_connection *pending = find_pending_connection(the_address);
		if(pending && pending->get_state() == pending_connection::connected && pending->get_initiator_nonce() == initiator_nonce && pending->get_host_nonce() == host_nonce)
			_send_connect_accept(pending);
		
		if(!_allow_connections && !(pending && pending->get_state() == pending_connection::awaiting_connect_request))
			return;

		uint32 client_identity;
		core::read(stream, client_identity);
		
		if(the_params._client_identity != compute_client_identity_token(the_address, initiator_nonce))
		{
			TorqueLogMessageFormatted(LogNettorque_socket, ("Client identity disagreement, params say %i, I say %i", client_identity, compute_client_identity_token(the_address, initiator_nonce)));
			return;
		}
		if(pending && (pending->get_initiator_nonce() != initiator_nonce || pending->get_host_nonce() != host_nonce))
			return;
		
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
			pending = new pending_connection(
		
		// now read the first part of the connection's symmetric key
		stream.read_bytes(the_params._symmetric_key, symmetric_cipher::key_size);
		_random_generator.random_buffer(the_params._init_vector, symmetric_cipher::key_size);
		
		uint32 connect_sequence;
		core::read(stream, connect_sequence);
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Request %8x", the_params._client_identity));
		
		if(connect)
			disconnect(connect, reason_self_disconnect, _new_connection_reason_buffer);
		
		torque_connection *conn = new torque_connection(initiator_nonce, _random_generator.random_integer(), _next_connection_index++, false);
		
		if(!conn)
			return;

		conn->set_host_nonce(host_nonce);
		conn->set_shared_secret(shared_secret);
		ref_ptr<torque_connection> the_connection = conn;
		conn->get_connection_parameters() = the_params;
		
		conn->set_address(the_address);
		conn->set_initial_recv_sequence(connect_sequence);
		conn->set_torque_socket(this);
		
		conn->set_symmetric_cipher(new symmetric_cipher(the_params._symmetric_key, the_params._init_vector));
		
		byte_buffer_ptr connect_request_data;
		core::read(stream, connect_request_data);

		add_pending_connection(conn);

		torque_socket_event *event;
		event = _allocate_queued_event();
		
		event->event_type = torque_connection_requested_event_type;

		event->public_key_size = the_params._public_key->get_public_key()->get_buffer_size();
		event->public_key = _allocate_queue_data(event->public_key_size);
		
		memcpy(event->public_key, the_params._public_key->get_public_key()->get_buffer(), event->public_key_size);

		event->connection = conn->get_connection_index();
		event->data_size = connect_request_data->get_buffer_size();
		event->data = _allocate_queue_data(event->data_size);
		memcpy(event->data, connect_request_data->get_buffer(), event->data_size);
	}
	
	/// Sends a connect accept packet to acknowledge the successful acceptance of a connect request.
	void send_connect_accept(torque_connection *conn)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Connect Accept - connection established."));
		
		packet_stream out;
		core::write(out, uint8(connect_accept_packet));
		
		core::write(out, conn->get_initiator_nonce());
		core::write(out, conn->get_host_nonce());
		uint32 encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(encrypt_pos);
		
		core::write(out, conn->get_initial_send_sequence());

		connection_parameters &the_params = conn->get_connection_parameters();
		core::write(out, the_params.connect_data);
		
		out.write_bytes(the_params._init_vector, symmetric_cipher::key_size);
		symmetric_cipher the_cipher(conn->get_shared_secret());
		bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);

		out.send_to(_socket, conn->get_address());
	}
	
	/// Handles a connect accept packet, putting the connection associated with the remote host (if there is one) into an active state.
	void handle_connect_accept(const address &the_address, bit_stream &stream)
	{
		nonce initiator_nonce, host_nonce;
		
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		uint32 decrypt_pos = stream.get_next_byte_position();
		stream.set_byte_position(decrypt_pos);
		
		torque_connection *conn = find_pending_connection(the_address);
		if(!conn || conn->get_connection_state() != torque_connection::awaiting_connect_response)
			return;
		
		if(conn->get_initiator_nonce() != initiator_nonce || conn->get_host_nonce() != host_nonce)
			return;
		
		connection_parameters &the_params = conn->get_connection_parameters();
		
		symmetric_cipher the_cipher(conn->get_shared_secret());
		if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;

		uint32 recv_sequence;
		core::read(stream, recv_sequence);
		conn->set_initial_recv_sequence(recv_sequence);
		
		byte_buffer_ptr connect_accept_data;
		core::read(stream, connect_accept_data);

		stream.read_bytes(the_params._init_vector, symmetric_cipher::key_size);
		conn->set_symmetric_cipher(new symmetric_cipher(the_params._symmetric_key, the_params._init_vector));
		
		add_connection(conn); // first, add it as a regular connection
		remove_pending_connection(conn); // remove from the pending connection list
		
		conn->clear_request();
		conn->set_connection_state(torque_connection::connected);
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Accept - connection established."));

		torque_socket_event *event = _allocate_queued_event();
		event->event_type = torque_connection_accepted_event_type;
		event->connection = conn->get_connection_index();
		event->client_identity = the_params._client_identity;
		event->data_size = connect_accept_data->get_buffer_size();
		event->data = _allocate_queue_data(event->data_size);
		memcpy(event->data, connect_accept_data->get_buffer(), event->data_size);
	
		post_connection_event(conn, torque_connection_established_event_type, 0);
	}
	
	/// Sends a connect rejection to a valid connect request in response to possible error conditions (server full, wrong password, etc).
	void send_connect_reject(nonce &initiator_nonce, nonce &host_nonce, const address &the_address, const byte_buffer_ptr &reason)
	{
		packet_stream out;
		core::write(out, uint8(connect_reject_packet));
		core::write(out, initiator_nonce);
		core::write(out, host_nonce);
		core::write(out, reason);
		out.send_to(_socket, the_address);
	}
	
	
	/// Handles a connect rejection packet by notifying the connection ref_object that the connection was rejected.
	void handle_connect_reject(const address &the_address, bit_stream &stream)
	{
		nonce initiator_nonce;
		nonce host_nonce;
		
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		
		torque_connection *conn = find_pending_connection(the_address);
		if(!conn || (conn->get_connection_state() != torque_connection::awaiting_challenge_response &&
					 conn->get_connection_state() != torque_connection::awaiting_connect_response))
			return;
		if(conn->get_initiator_nonce() != initiator_nonce || conn->get_host_nonce() != host_nonce)
			return;
		
		byte_buffer_ptr reason;
		core::read(stream, reason);
		
		TorqueLogMessageFormatted(LogNettorque_socket, ("Received Connect Reject - reason %s", reason->get_buffer()));

		connection_parameters &p = conn->get_connection_parameters();
		// if the reason is a bad puzzle solution, try once more with a
		// new nonce.
		if(!strcmp((const char *) reason->get_buffer(), "Puzzle") && !p._puzzle_retried)
		{
			p._puzzle_retried = true;
			conn->set_connection_state(torque_connection::awaiting_challenge_response);
			conn->_connect_send_count = 0;
			conn->set_initiator_nonce(_random_generator.random_nonce());
			
			send_connect_challenge_request(conn);
			return;
		}
		
		conn->clear_request();
		conn->set_connection_state(torque_connection::connect_rejected);
		post_connection_event(conn, torque_connection_rejected_event_type, reason);
		remove_pending_connection(conn);

		torque_socket_event *event = _allocate_queued_event();
		event->event_type = torque_connection_rejected_event_type;
		event->data_size = reason->get_buffer_size();
		event->data = _allocate_queue_data(event->data_size);
		memcpy(event->data, reason->get_buffer(), event->data_size);
	}
	
	
	/// Dispatches a disconnect packet for a specified connection.
	void handle_disconnect(const address &the_address, bit_stream &stream)
	{
		torque_connection *conn = find_connection(the_address);
		if(!conn)
			return;
				
		nonce initiator_nonce, host_nonce;
		byte_buffer_ptr reason;
		
		core::read(stream, initiator_nonce);
		core::read(stream, host_nonce);
		
		if(initiator_nonce != conn->get_initiator_nonce() || host_nonce != conn->get_host_nonce())
			return;
		
		uint32 decrypt_pos = stream.get_next_byte_position();
		stream.set_byte_position(decrypt_pos);
		
		symmetric_cipher the_cipher(conn->get_shared_secret());
		if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;

		core::read(stream, reason);
		conn->set_connection_state(torque_connection::disconnected);
		post_connection_event(conn, torque_connection_disconnected_event_type, reason);
		remove_connection(conn);
	}
	
	/// Handles all packets that don't fall into the category of torque_connection handshake or game data.
	void handle_info_packet(const address &address, uint8 packet_type, bit_stream &stream)
	{
		torque_socket_event *event = _allocate_queued_event();
		event->event_type = torque_socket_packet_event_type;
		event->data_size = stream.get_stream_byte_size();
		event->data = (uint8 *) _allocate_queue_data(event->data_size);
		memcpy(event->data, stream.get_buffer(), event->data_size);
		address.to_sockaddr(&event->source_address);
	}
	
	/// Processes a single packet, and dispatches either to handle_info_packet or to the connection associated with the remote address.
	void process_packet(const address &the_address, bit_stream &packet_stream)
	{
		
		// Determine what to do with this packet:
		
		if(packet_stream.get_buffer()[0] & 0x80) // it's a protocol packet...
		{
			// if the MSB of the first byte is set, it's a protocol data packet so pass it to the appropriate connection.
			
			// lookup the connection in the addressTable if this packet causes a disconnection, keep the conn around until this function exits
			ref_ptr<torque_connection> conn = find_connection(the_address);
			if(conn)
				conn->read_raw_packet(packet_stream);
		}
		else
		{
			// Otherwise, it's either a game info packet or a connection handshake packet.
			
			uint8 packet_type;
			core::read(packet_stream, packet_type);
			
			if(packet_type >= first_valid_info_packet_id)
				handle_info_packet(the_address, packet_type, packet_stream);
			else
			{
				// check if there's a connection already:
				switch(packet_type)
				{
					case connect_challenge_request_packet:
						handle_connect_challenge_request(the_address, packet_stream);
						break;
					case connect_challenge_response_packet:
						handle_connect_challenge_response(the_address, packet_stream);
						break;
					case connect_request_packet:
						handle_connect_request(the_address, packet_stream);
						break;
					case connect_reject_packet:
						handle_connect_reject(the_address, packet_stream);
						break;
					case connect_accept_packet:
						handle_connect_accept(the_address, packet_stream);
						break;
					case disconnect_packet:
						handle_disconnect(the_address, packet_stream);
						break;
					case punch_packet:
						//handle_punch(the_address, packet_stream);
						handle_punch_packet(the_address, packet_stream);
						break;
					case arranged_connect_request_packet:
						handle_arranged_connect_request(the_address, packet_stream);
						break;
					case introduction_request_packet:
						handle_introduction_request(the_address, packet_stream);
						break;
					case send_punch_packet:
						handle_send_punch_packet(the_address, packet_stream);
				}
			}
		}
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
	
	torque_socket_event *post_connection_event(torque_connection *conn, uint32 event_type, byte_buffer_ptr data)
	{
		torque_socket_event *event = _allocate_queued_event();
		event->event_type = event_type;
		event->connection = conn->get_connection_index();
		if(data)
		{
			event->data_size = data->get_buffer_size();
			event->data = _allocate_queue_data(event->data_size);
			memcpy(event->data, data->get_buffer(), event->data_size);
		}
		return event;
	}
	
	
	/// Returns the list of connections on this torque_socket.
	array<ref_ptr<torque_connection> > &get_connection_list()
	{
		return _connection_list;
	}
	
	/// looks up a connected connection on this torque_socket
	torque_connection *find_connection(const address &remote_address)
	{
		// The connection hash table is a single vector, with hash collisions
		// resolved to the next open space in the table.
		
		// Compute the hash index based on the network address
		uint32 hash_index = remote_address.hash() % _connection_hash_table.size();
		
		// Search through the table for an address that matches the source
		// address.  If the connection pointer is NULL, we've found an
		// empty space and a connection with that address is not in the table
		while(_connection_hash_table[hash_index] != NULL)
		{
			if(remote_address == _connection_hash_table[hash_index]->get_address())
				return _connection_hash_table[hash_index];
			hash_index++;
			if(hash_index >= (uint32) _connection_hash_table.size())
				hash_index = 0;
		}
		return NULL;
	}
	
	torque_connection *find_connection_by_id(uint32 id)
	{
		hash_table_flat<uint32, torque_connection *>::pointer p = _connection_index_table.find(id);
		if(p)
			return *(p.value());
		return 0;
	}
	
	/// Finds a connection instance that this torque_socket has initiated.
	torque_connection *find_pending_connection(const address &the_address)
	{
		// Loop through all the pending connections and compare the NetAddresses
		for(uint32 i = 0; i < _pending_connections.size(); i++)
			if(the_address == _pending_connections[i]->get_address())
				return _pending_connections[i];
		return NULL;
	}
	
	/// Adds a torque_connection the list of pending connections.
	void add_pending_connection(torque_connection *the_connection)
	{
		_connection_index_table.insert(the_connection->get_connection_index(), the_connection);
		// make sure we're not already connected to the host at the
		// connection's address
		find_and_remove_pending_connection(the_connection->get_address());
		torque_connection *temp = find_connection(the_connection->get_address());
		if(temp)
			disconnect(temp, reason_self_disconnect, _reconnecting_reason_buffer);
		
		// hang on to the connection and add it to the pending connection list
		_pending_connections.push_back(the_connection);
	}
	
	/// Removes a connection from the list of pending connections.
	void remove_pending_connection(torque_connection *the_connection)
	{
		// search the pending connection list for the specified connection
		// and remove it.
		for(uint32 i = 0; i < _pending_connections.size(); i++)
			if(_pending_connections[i] == the_connection)
			{
				_pending_connections.erase(i);
				return;
			}
	}
	
	
	/// Finds a connection by address from the pending list and removes it.
	void find_and_remove_pending_connection(const address &the_address)
	{
		// Search through the list by address and remove any connection
		// that matches.
		for(uint32 i = 0; i < _pending_connections.size(); i++)
			if(the_address == _pending_connections[i]->get_address())
			{
				_pending_connections.erase(i);
				return;
			}
	}
	
	/// Adds a connection to the internal connection list.
	void add_connection(torque_connection *the_connection)
	{
		_connection_list.push_back(the_connection);
		uint32 num_connections = _connection_list.size();
		if(num_connections > _connection_hash_table.size() / 2)
		{
			_connection_hash_table.resize(num_connections * 4 - 1);
			for(uint32 i = 0; i < _connection_hash_table.size(); i++)
				_connection_hash_table[i] = NULL;
			for(uint32 i = 0; i < num_connections; i++)
			{
				uint32 index = _connection_list[i]->get_address().hash() % _connection_hash_table.size();
				while(_connection_hash_table[index] != NULL)
				{
					index++;
					if(index >= (uint32) _connection_hash_table.size())
						index = 0;
				}
				_connection_hash_table[index] = _connection_list[i];
			}
		}
		else
		{
			uint32 index = _connection_list[num_connections - 1]->get_address().hash() % _connection_hash_table.size();
			while(_connection_hash_table[index] != NULL)
			{
				index++;
				if(index >= (uint32) _connection_hash_table.size())
					index = 0;
			}
			_connection_hash_table[index] = _connection_list[num_connections - 1];
		}
	}
	
	
	/// Remove a connection from the list.
	void remove_connection(torque_connection *the_connection)
	{
		_connection_index_table.remove(the_connection->get_connection_index());
		// hold the reference to the ref_object until the function exits.
		ref_ptr<torque_connection> hold = the_connection;
		for(uint32 i = 0; i < _connection_list.size(); i++)
		{
			if(_connection_list[i] == the_connection)
			{
				_connection_list.erase_unstable(i);
				break;
			}
		}
		uint32 index = the_connection->get_address().hash() % _connection_hash_table.size();
		uint32 start_index = index;
		
		while(_connection_hash_table[index] != the_connection)
		{
			index++;
			if(index >= (uint32) _connection_hash_table.size())
				index = 0;
			assert(index != start_index); // Attempting to remove a connection that is not in the table.
			if(index == start_index)
				return;
		}
		_connection_hash_table[index] = NULL;
		
		// rehash all subsequent entries until we find a NULL entry:
		for(;;)
		{
			index++;
			if(index >= (uint32) _connection_hash_table.size())
				index = 0;
			if(!_connection_hash_table[index])
				break;
			torque_connection *rehashed_connection = _connection_hash_table[index];
			_connection_hash_table[index] = NULL;
			uint32 real_index = rehashed_connection->get_address().hash() % _connection_hash_table.size();
			while(_connection_hash_table[real_index] != NULL)
			{
				real_index++;
				if(real_index >= (uint32) _connection_hash_table.size())
					real_index = 0;
			}
			_connection_hash_table[real_index] = rehashed_connection;
		}
	}
	
public:
	~torque_socket()
	{
		// gracefully close all the connections on this torque_socket:
		while(_connection_list.size())
		{
			torque_connection *c = _connection_list[0];
			disconnect(c, reason_self_disconnect, _shutdown_reason_buffer);
		}
	}
	
	/// @param bind_address Local network address to bind this torque_socket to.
	torque_socket(const address &bind_address) : _puzzle_manager(_random_generator, &_allocator), _event_queue_allocator(&_allocator)
	{
		_next_connection_index = 1;
		_random_generator.random_buffer(_random_hash_data, sizeof(_random_hash_data));

		_private_key = new asymmetric_key(20, _random_generator);
		
		_last_timeout_check_time = time(0);
		_allow_connections = true;
		
		_connection_hash_table.resize(129);
		for(uint32 i = 0; i < _connection_hash_table.size(); i++)
			_connection_hash_table[i] = NULL;
		_send_packet_list = NULL;
		_process_start_time = time::get_current();
		
		udp_socket::bind_result res = _socket.bind(bind_address);
		
		// Supply our own (small) unique private key for the time being.
		_private_key = new asymmetric_key(16, _random_generator);
		_challenge_response_buffer = new byte_buffer();
		_event_queue_head = _event_queue_tail = 0;
	}
	
	puzzle_solver _puzzle_solver;
	
	udp_socket _socket; ///< Network socket this torque_socket communicates over.
	random_generator _random_generator;	
	array<ref_ptr<torque_connection> > _connection_list; ///< List of all the connections that are in a connected state on this torque_socket.
	array<torque_connection *> _connection_hash_table; ///< A resizable hash table for all connected connections.  This is a flat hash table (no buckets).	
	array<ref_ptr<torque_connection> > _pending_connections; ///< List of connections that are in the startup state, where the remote host has not fully
	
	ref_ptr<asymmetric_key> _private_key; ///< The private key used by this torque_socket for secure key exchange.
	zone_allocator _allocator;
	client_puzzle_manager _puzzle_manager; ///< The ref_object that tracks the current client puzzle difficulty, current puzzle and solutions for this torque_socket.
	time _process_start_time; ///< Current time tracked by this torque_socket.
	bool _requires_key_exchange; ///< True if all connections outgoing and incoming require key exchange.
	time _last_timeout_check_time; ///< Last time all the active connections were checked for timeouts.
	uint8  _random_hash_data[12]; ///< Data that gets hashed with connect challenge requests to prevent connection spoofing.
	bool _allow_connections; ///< Set if this torque_socket allows connections from remote instances.
	delay_send_packet *_send_packet_list; ///< List of delayed packets pending to send.
	
	event_queue_entry *_event_queue_tail;
	event_queue_entry *_event_queue_head;
	uint32 _next_connection_index;
	page_allocator<16> _event_queue_allocator;
	hash_table_flat<uint32, torque_connection *> _connection_index_table;
};
