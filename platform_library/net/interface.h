/// interface class.
///
/// Manages all valid and pending notify protocol connections for a port/IP. If you are
/// providing multiple services or servicing multiple networks, you may have more than
/// one interface.
///
/// <b>Connection handshaking basic overview:</b>
///
/// TNP does a two phase connect handshake to prevent a several types of
/// Denial-of-Service (DoS) attacks.
///
/// The initiator of the connection (client) starts the connection by sending
/// a unique random nonce (number, used once) value to the server as part of
/// the connect_challenge_request packet.
/// C->S: connect_challenge_request, Nc
///
/// The server responds to the connect_challenge_request with a "Client Puzzle"
/// that has the property that verifying a solution to the puzzle is computationally
/// simple, but can be of a specified computational, brute-force difficulty to
/// compute the solution itself.  The client puzzle is of the form:
/// secureHash(Ic, Nc, Ns, X) = Y >> k, where Ic is the identity of the client,
/// and X is a value computed by the client such that the high k bits of the value
/// y are all zero.  The client identity is computed by the server as a partial hash
/// of the client's IP address and port and some random data on the server.
/// its current nonce (Ns), Nc, k, and the server's authentication certificate.
/// S->C: connect_challenge_response, Nc, Ns, Ic, Cs
///
/// The client, upon receipt of the connect_challenge_response, validates the packet
/// sent by the server and computes a solution to the puzzle the server sent.  If
/// the connection is to be authenticated, the client can also validate the server's
/// certificate (if it's been signed by a certificate Authority), and then generates
/// a shared secret from the client's key pair and the server's public key.  The client
/// response to the server consists of:
/// C->S: connect_request, Nc, Ns, X, Cc, shared_secret(key1, sequence1, NetConnectionClass, class-specific sendData)
///
/// The server then can validation the solution to the puzzle the client submitted, along
/// with the client identity (Ic).
/// Until this point the server has allocated no memory for the client and has
/// verified that the client is sending from a valid IP address, and that the client
/// has done some amount of work to prove its willingness to start a connection.
/// As the server load increases, the server may choose to increase the difficulty (k) of
/// the client puzzle, thereby making a resource depletion DoS attack successively more
/// difficult to launch.
///
/// If the server accepts the connection, it sends a connect accept packet that is
/// encrypted and hashed using the shared secret.  The contents of the packet are
/// another sequence number (sequence2) and another key (key2).  The sequence numbers 
/// are the initial send and receive sequence numbers for the connection, and the
/// key2 value becomes the IV of the symmetric cipher.  The connection subclass is
/// also allowed to write any connection specific data into this packet.
/// 
/// Connections using the secure key exchange are still vulnerable to Man-in-the-middle attacks.
/// Using a key signed by a trusted certificate authority (CA), makes the communications channel as securely
/// trusted as the trust in the CA.
///
/// <b>Arranged Connection handshaking:</b>
///
/// interface can also facilitate "arranged" connections.  Arranged connections are
/// necessary when both parties to the connection are behind firewalls or NAT routers.
/// Suppose there are two clients, A and B that want to esablish a direct connection with
/// one another.  If A and B are both logged into some common server S, then S can send
/// A and B the public (NAT'd) address, as well as the IP addresses each client detects
/// for itself.
///
/// A and B then both send "punch" packets to the known possible addresses of each other.
/// The punch packet client A sends enables the punch packets client B sends to be 
/// delivered through the router or firewall since it will appear as though it is a service
/// response to A's initial packet.
///
/// Upon receipt of the punch packet by the "initiator"
/// of the connection, an arranged_connect_request packet is sent.
/// if the non-initiator of the connection gets an ArrangedPunch
/// packet, it simply sends another punch packet to the
/// remote host, but narrows down its Address range to the address
/// it received the packet from.
/// The ArrangedPunch packet from the intiator contains the nonce 
/// for the non-initiator, and the nonce for the initiator encrypted
/// with the shared secret.
/// The ArrangedPunch packet for the receiver of the connection
/// contains all that, plus the public key/key_size of the receiver.

class connection;
struct connection_parameters;

class interface : public ref_object
{
	friend class connection;
public:
	enum termination_reason {
		reason_timed_out,
		reason_failed_connect_handshake,
		reason_remote_host_rejected_connection,
		reason_remote_disconnect_packet,
		reason_duplicate_connection_attempt,
		reason_self_disconnect,
		reason_error,
	};
		
	/// @param   bind_address    Local network address to bind this interface to.
	interface(const address &bind_address) : _puzzle_manager(_random_generator, &_allocator)
	{
		_old_connection_reason_buffer = new byte_buffer("OLD_CONNECTION");
		_new_connection_reason_buffer = new byte_buffer("NEW_CONNECTION");
		_reconnecting_reason_buffer = new byte_buffer("RECONNECTING");
		_shutdown_reason_buffer = new byte_buffer("SHUTDOWN");
		_timed_out_reason_buffer = new byte_buffer("TIMEDOUT");
		_private_key = new asymmetric_key(20);
		
		_last_timeout_check_time = time(0);
		_allow_connections = true;
		
		_random_generator.random_buffer(_random_hash_data, sizeof(_random_hash_data));
		
		_connection_hash_table.resize(129);
		for(uint32 i = 0; i < _connection_hash_table.size(); i++)
			_connection_hash_table[i] = NULL;
		_send_packet_list = NULL;
		_process_start_time = time::get_current();

		udp_socket::bind_result res = _socket.bind(bind_address);

		// Supply our own (small) unique private key for the time being.
		_private_key = new asymmetric_key(16, _random_generator);
		_challenge_response_buffer = new byte_buffer();
	}
	
	~interface()
	{
		// gracefully close all the connections on this interface:
		while(_connection_list.size())
		{
			connection *c = _connection_list[0];
			disconnect(c, reason_self_disconnect, _shutdown_reason_buffer);
		}
	}
	
	
	/// Returns the address of the first network interface in the list that the socket on this interface is bound to.
	address get_first_bound_interface_address()
	{
		address the_address = _socket.get_bound_address();
		
		if(the_address.is_same_host(address(address::any, 0)))
		{
			array<address> interface_addresses;
			address::get_interface_addresses(interface_addresses);
			uint16 save_port = the_address.get_port();
			if(interface_addresses.size())
			{
				the_address = interface_addresses[0];
				the_address.set_port(save_port);
			}
		}
		return the_address;
	}
	
	
	/// Sets the private key this interface will use for authentication and key exchange
	void set_private_key(asymmetric_key *the_key)
	{
		_private_key = the_key;
	}
	
	/// Returns whether or not this interface allows connections from remote hosts.
	bool does_allow_connections() { return _allow_connections; }
	
	/// Sets whether or not this interface allows connections from remote hosts.
	void set_allows_connections(bool conn) { _allow_connections = conn; }
	
	/// Returns the Socket associated with this interface
	udp_socket &get_socket() { return _socket; }
	
	/// Sends a packet to the remote address over this interface's socket.
	udp_socket::send_to_result send_to(const address &the_address, bit_stream &stream)
	{
		return _socket.send_to(the_address, stream.get_buffer(), stream.get_next_byte_position());
	}
	
	
	/// Sends a packet to the remote address after millisecond_delay time has elapsed.
	///
	/// This is used to simulate network latency on a LAN or single computer.
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
	
	void tnp_post_event(const torque_socket_event& data, const ref_ptr<net::connection>& source_conn)
	{
		torque_socket_event_ex evt;
		evt.data = data;
		evt.conn = source_conn;

		_event_queue.push(evt);
	}
	
	bool tnp_pop_event(torque_socket_event_ex& evt)
	{
		if(_event_queue.empty())
			return false;
			
		evt = _event_queue.front();
		_event_queue.pop();
		
		return true;
	}

	/// Dispatch function for processing all network packets through this interface.
	void check_incoming_packets()
	{
		packet_stream stream;
		udp_socket::recv_from_result result;
		address sourceAddress;
		
		_process_start_time = time::get_current();
		
		// read out all the available packets:
		while((result = stream.recv_from(_socket, &sourceAddress)) == udp_socket::packet_received)
      {
         printf("Handling a packet\n");
			process_packet(sourceAddress, stream);
      }
	}
	
	/// returns the current process time for this interface
	time get_process_start_time() { return _process_start_time; }
	
	/// Disconnects the given connection and removes it from the interface
	void disconnect(connection *conn, termination_reason reason, byte_buffer_ptr &reason_buffer)
	{
		if(conn->get_connection_state() == connection::awaiting_challenge_response ||
		   conn->get_connection_state() == connection::awaiting_connect_response)
		{
			conn->on_connect_terminated(reason, reason_buffer);
			remove_pending_connection(conn);
		}
		else if(conn->get_connection_state() == connection::connected)
		{
			conn->set_connection_state(connection::disconnected);
			conn->on_connection_terminated(reason, reason_buffer);
			
			// send a disconnect packet...
			packet_stream out;
			core::write(out, uint8(disconnect_packet));
			connection_parameters &the_params = conn->get_connection_parameters();
			core::write(out, the_params._nonce);
			core::write(out, the_params._server_nonce);
			uint32 encrypt_pos = out.get_next_byte_position();
			out.set_byte_position(encrypt_pos);
			core::write(out, reason_buffer);
			
			symmetric_cipher the_cipher(the_params._shared_secret);
			bit_stream_hash_and_encrypt(out, connection::message_signature_bytes, encrypt_pos, &the_cipher);
			
			out.send_to(_socket, conn->get_address());
			
			remove_connection(conn);
		}
	}
	/// Handles all packets that don't fall into the category of connection handshake or game data.
	virtual void handle_info_packet(const address &address, uint8 packet_type, bit_stream &stream)
	{
		torque_socket_event event;
		event.event_type = torque_socket_packet_event_type;
		event.data_size = stream.get_stream_byte_size();
		stream.read_bytes(event.data, event.data_size);
		address.to_sockaddr(&event.source_address);
		tnp_post_event(event, NULL);
	}
	
	/// Checks all connections on this interface for packet sends, and for timeouts and all valid
	/// and pending connections.
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
				connection *pending = _pending_connections[i];
				
				if(pending->outstanding_request())
				{
					if(!pending->retry_request())
					{
						pending->set_connection_state(connection::connect_timed_out);
						pending->on_connect_terminated(reason_timed_out, _timed_out_reason_buffer);
						remove_pending_connection(pending);
					}
				}
				
				if(pending->get_connection_state() == connection::sending_punch_packets &&
						get_process_start_time() > pending->_connect_last_send_time + time(punch_retry_time))
				{
					if(pending->_connect_send_count > punch_retry_count)
					{
						pending->set_connection_state(connection::connect_timed_out);
						pending->on_connect_terminated(reason_timed_out, _timed_out_reason_buffer);
						remove_pending_connection(pending);
						continue;
					}
					else
						send_punch_packets(pending);
				}
				else if(pending->get_connection_state() == connection::computing_puzzle_solution &&
						get_process_start_time() > pending->_connect_last_send_time + 
						time(puzzle_solution_timeout))
				{
					pending->set_connection_state(connection::connect_timed_out);
					pending->on_connect_terminated(reason_timed_out, _timed_out_reason_buffer);
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
						_connection_list[i]->set_connection_state(connection::connect_timed_out);
						_connection_list[i]->on_connect_terminated(reason_timed_out, _timed_out_reason_buffer);
						remove_connection(_connection_list[i]);
					}
				}
				
				if(_connection_list[i]->check_timeout(get_process_start_time()))
				{
					_connection_list[i]->set_connection_state(connection::timed_out);
					_connection_list[i]->on_connection_terminated(reason_timed_out, _timed_out_reason_buffer);
					
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
				connection *conn = _pending_connections[i];
				if(conn->get_connection_state() == connection::computing_puzzle_solution)
				{
					connection_parameters &the_params = conn->get_connection_parameters();
					if(the_params._puzzle_request_index != request_index)
						continue;
					// this was the solution for this client...
					the_params._puzzle_solution = solution;
					
					conn->set_connection_state(connection::awaiting_connect_response);
					send_connect_request(conn);
					break;
				}
			}
		}
	}
	
	random_generator &random() {
		return _random_generator;
	}
	
	/// packet_type is encoded as the first byte of each packet.
	///
	/// Subclasses of interface can add custom, non-connected data
	/// packet types starting at first_valid_info_packet_id, and overriding 
	/// handle_info_packet to process them.
	///
	/// Packets that arrive with the high bit of the first byte set
	/// (i.e. the first unsigned byte is greater than 127), are
	/// assumed to be connected protocol packets, and are dispatched to
	/// the appropriate connection for further processing.
public:
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
		first_valid_info_packet_id, ///< The first valid ID for a interface subclass's info packets.
	};
protected:
	array<ref_ptr<connection> > _connection_list; ///< List of all the connections that are in a connected state on this interface.
	array<connection *> _connection_hash_table; ///< A resizable hash table for all connected connections.  This is a flat hash table (no buckets).
	
	array<ref_ptr<connection> > _pending_connections; ///< List of connections that are in the startup state, where the remote host has not fully
	///  validated the connection.
	
	ref_ptr<asymmetric_key> _private_key; ///< The private key used by this interface for secure key exchange.
	zone_allocator _allocator;
	client_puzzle_manager _puzzle_manager; ///< The ref_object that tracks the current client puzzle difficulty, current puzzle and solutions for this interface.
	
	/// @name NetInterfaceSocket Socket
	///
	/// State regarding the socket this interface controls.
	///
	/// @{
	
	///
	udp_socket _socket; ///< Network socket this interface communicates over.
	random_generator _random_generator;
	byte_buffer_ptr _old_connection_reason_buffer;
	byte_buffer_ptr _new_connection_reason_buffer;
	byte_buffer_ptr _reconnecting_reason_buffer;
	byte_buffer_ptr _shutdown_reason_buffer;
	byte_buffer_ptr _timed_out_reason_buffer;

	byte_buffer_ptr _challenge_response_buffer;
	
	/// @}
	
	time _process_start_time; ///< Current time tracked by this interface.
	bool _requires_key_exchange; ///< True if all connections outgoing and incoming require key exchange.
	time _last_timeout_check_time; ///< Last time all the active connections were checked for timeouts.
	uint8  _random_hash_data[12]; ///< Data that gets hashed with connect challenge requests to prevent connection spoofing.
	bool _allow_connections; ///< Set if this interface allows connections from remote instances.
	
	/// Structure used to track packets that are delayed in sending for simulating a high-latency connection.
	///
	/// The delay_send_packet is allocated as sizeof(delay_send_packet) + packet_size;
	struct delay_send_packet
	{
		delay_send_packet *next_packet; ///< The next packet in the list of delayed packets.
		address remote_address; ///< The address to send this packet to.
		time send_time; ///< time when we should send the packet.
		uint32 packet_size; ///< Size, in bytes, of the packet data.
		uint8 packet_data[1]; ///< Packet data.
	};
	delay_send_packet *_send_packet_list; ///< List of delayed packets pending to send.
	
	
	enum interface_constants {
		challenge_retry_count = 4, ///< Number of times to send connect challenge requests before giving up.
		challenge_retry_time = 2500, ///< Timeout interval in milliseconds before retrying connect challenge.
		
		connect_retry_count = 4, ///< Number of times to send connect requests before giving up.
		connect_retry_time = 2500, ///< Timeout interval in milliseconds before retrying connect request.
		
		punch_retry_count = 6, ///< Number of times to send groups of firewall punch packets before giving up.
		punch_retry_time = 2500, ///< Timeout interval in milliseconds before retrying punch sends.
		
		timeout_check_interval = 1500, ///< Interval in milliseconds between checking for connection timeouts.
		puzzle_solution_timeout = 30000, ///< If the server gives us a puzzle that takes more than 30 seconds, time out.
	};
	
	std::queue<torque_socket_event_ex> _event_queue;
	
public:
	/// Computes an identity token for the connecting client based on the address of the client and the
	/// client's unique nonce value.
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
	
	
	/// Finds a connection instance that this interface has initiated.
	connection *find_pending_connection(const address &the_address)
	{
		// Loop through all the pending connections and compare the NetAddresses
		for(uint32 i = 0; i < _pending_connections.size(); i++)
			if(the_address == _pending_connections[i]->get_address())
				return _pending_connections[i];
		return NULL;
	}
	
	/// Adds a connection the list of pending connections.
	void add_pending_connection(connection *the_connection)
	{
		// make sure we're not already connected to the host at the
		// connection's address
		find_and_remove_pending_connection(the_connection->get_address());
		connection *temp = find_connection(the_connection->get_address());
		if(temp)
			disconnect(temp, reason_self_disconnect, _reconnecting_reason_buffer);
		
		// hang on to the connection and add it to the pending connection list
		_pending_connections.push_back(the_connection);
	}
	
	/// Removes a connection from the list of pending connections.
	void remove_pending_connection(connection *the_connection)
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
	void add_connection(connection *the_connection)
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
	void remove_connection(connection *the_connection)
	{
		// hold the reference to the ref_object until the function exits.
		ref_ptr<connection> hold = the_connection;
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
			connection *rehashed_connection = _connection_hash_table[index];
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
	
	
	/// Begins the connection handshaking process for a connection.  Called from connection::connect()
	void start_connection(const ref_ptr<connection>& the_connection)
	{
		assert(the_connection->get_connection_state() == connection::not_connected); // Cannot start unless it is in the not_connected state.
		
		if(find_pending_connection(the_connection->get_address()))
			remove_pending_connection(the_connection);
		
		add_pending_connection(the_connection);
		the_connection->_connect_send_count = 0;
		the_connection->set_connection_state(connection::awaiting_challenge_response);
		send_connect_challenge_request(the_connection);
	}
	
	/// Sends a connect challenge request on behalf of the connection to the remote host.
	void send_connect_challenge_request(connection *the_connection)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Connect Challenge Request to %s", the_connection->get_address().to_string().c_str()));
		packet_stream out;
		core::write(out, uint8(connect_challenge_request_packet));
		connection_parameters &params = the_connection->get_connection_parameters();
		core::write(out, params._nonce);
		the_connection->submit_request(out);
	}
	
	
	/// Handles a connect challenge request by replying to the requestor of a connection with a
	/// unique token for that connection, as well as (possibly) a client puzzle (for DoS prevention),
	/// or this interface's public key.
	void handle_connect_challenge_request(const address &addr, bit_stream &stream)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Challenge Request from %s", addr.to_string().c_str()));
		
		if(!_allow_connections)
			return;
		
		// In the case of an arranged connection we will already have a pending
		// connenection for this address.  Clear its request and remove it
		// from our list, to be recreated later.
		connection* conn = find_pending_connection(addr);
		if(conn)
		{
			conn->clear_request();
			remove_pending_connection(conn);
		}
		nonce client_nonce;
		core::read(stream, client_nonce);
		send_connect_challenge_response(addr, client_nonce);
	}
	
	void set_challenge_response_data(byte_buffer_ptr data)
	{
		_challenge_response_buffer = data;
	}
	
	/// Sends a connect challenge request to the specified address.  This can happen as a result
	/// of receiving a connect challenge request, or during an "arranged" connection for the non-initiator
	/// of the connection.
	void send_connect_challenge_response(const address &addr, nonce &client_nonce)
	{
		packet_stream out;
		core::write(out, uint8(connect_challenge_response_packet));
		core::write(out, client_nonce);
		
		uint32 identity_token = compute_client_identity_token(addr, client_nonce);
		core::write(out, identity_token);
		
		// write out a client puzzle
		nonce server_nonce = _puzzle_manager.get_current_nonce();
		uint32 difficulty = _puzzle_manager.get_current_difficulty();
		core::write(out, server_nonce);
		core::write(out, difficulty);
		core::write(out, _private_key->get_public_key());
		core::write(out, _challenge_response_buffer);

		TorqueLogMessageFormatted(LogNetInterface, ("Sending Challenge Response: %8x", identity_token));
		
		out.send_to(_socket, addr);
	}
	
	
	/// Processes a connect_challenge_response, by issueing a connect request if it was for
	/// a connection this interface has pending.
	void handle_connect_challenge_response(const address &the_address, bit_stream &stream)
	{
		connection *conn = find_pending_connection(the_address);
		if(!conn || conn->get_connection_state() != connection::awaiting_challenge_response)
			return;
		
		nonce the_nonce;
		core::read(stream, the_nonce);
		
		connection_parameters &the_params = conn->get_connection_parameters();
		if(the_nonce != the_params._nonce)
			return;
		
		core::read(stream, the_params._client_identity);
		
		// see if the server wants us to solve a client puzzle
		core::read(stream, the_params._server_nonce);
		core::read(stream, the_params._puzzle_difficulty);
		
		if(the_params._puzzle_difficulty > client_puzzle_manager::max_puzzle_difficulty)
			return;
		
		the_params._public_key = new asymmetric_key(stream);
		if(!the_params._public_key->is_valid())
			return;

		if(_private_key.is_null() || _private_key->get_key_size() != the_params._public_key->get_key_size())
		{
			// we don't have a private key, so generate one for this connection
			the_params._private_key = new asymmetric_key(the_params._public_key->get_key_size());
		}
		else
			the_params._private_key = _private_key;
		the_params._shared_secret = the_params._private_key->compute_shared_secret_key(the_params._public_key);
		//logprintf("shared secret (client) %s", the_params._shared_secret->encodeBase64()->get_buffer());
		_random_generator.random_buffer(the_params._symmetric_key, symmetric_cipher::key_size);

		TorqueLogMessageFormatted(LogNetInterface, ("Received Challenge Response: %8x", the_params._client_identity ));

		byte_buffer_ptr response_data;
		core::read(stream, response_data);

		torque_socket_event event;
		event.event_type = torque_connection_challenge_response_event_type;

		event.public_key_size = the_params._public_key->get_public_key()->get_buffer_size();
		memcpy(event.public_key, the_params._public_key->get_public_key()->get_buffer(), event.public_key_size);

		event.data_size = response_data->get_buffer_size();
		memcpy(event.data, response_data->get_buffer(), event.data_size);

		tnp_post_event(event, conn);

		conn->clear_request();
		conn->set_connection_state(connection::computing_puzzle_solution);
		conn->_connect_send_count = 0;

		the_params._puzzle_solution = 0;
		conn->_connect_last_send_time = get_process_start_time();
		packet_stream s;
		core::write(s, the_params._nonce);
		core::write(s, the_params._server_nonce);
		core::write(s, the_params._puzzle_difficulty);
		core::write(s, the_params._client_identity);
		byte_buffer_ptr request = new byte_buffer(s.get_buffer(), s.get_next_byte_position());
		
		the_params._puzzle_request_index = _puzzle_solver.post_request(request);
	}
	class puzzle_solver : public thread_queue
	{
	public:
		puzzle_solver() : thread_queue(1) { }
		void process_request(const byte_buffer_ptr &the_request, byte_buffer_ptr &the_response, bool *cancelled, float *progress)
		{
			nonce the_nonce;
			nonce remote_nonce;
			uint32 puzzle_difficulty;
			uint32 client_identity;
			bit_stream s(the_request->get_buffer(), the_request->get_buffer_size());
			core::read(s, the_nonce);
			core::read(s, remote_nonce);
			core::read(s, puzzle_difficulty);
			core::read(s, client_identity);
			uint32 solution = 0;
			time start = time::get_current();
			for(;;)
			{
				if(*cancelled)
					break;
				if(client_puzzle_manager::check_one_solution(solution, the_nonce, remote_nonce, puzzle_difficulty, client_identity))
					break;

				++solution;
			}
			if(!*cancelled)
			{
				byte_buffer_ptr response = new byte_buffer(sizeof(solution));
				bit_stream s(response->get_buffer(), response->get_buffer_size());
				core::write(s, solution);
				the_response = response;
				TorqueLogMessageFormatted(LogNetInterface, ("Client puzzle solved in %lli ms.", (time::get_current() - start).get_milliseconds()));
			}
		}
	};
	puzzle_solver _puzzle_solver;
	
	/// Sends a connect request on behalf of a pending connection.
	void send_connect_request(connection *conn)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Connect Request"));
		packet_stream out;
		connection_parameters &the_params = conn->get_connection_parameters();
		
		core::write(out, uint8(connect_request_packet));
		core::write(out, the_params._nonce);
		core::write(out, the_params._server_nonce);
		core::write(out, the_params._client_identity);
		core::write(out, the_params._puzzle_difficulty);
		core::write(out, the_params._puzzle_solution);
		
		uint32 encrypt_pos = 0;
	
		core::write(out, the_params._private_key->get_public_key());
		encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(encrypt_pos);
		out.write_bytes(the_params._symmetric_key, symmetric_cipher::key_size);

		core::write(out, conn->get_initial_send_sequence());
		conn->write_connect_request(out);
		
		// if we're using crypto on this connection,
		// then write a hash of everything we wrote into the packet
		// key.  Then we'll symmetrically encrypt the packet from
		// the end of the public key to the end of the signature.
		
		symmetric_cipher the_cipher(the_params._shared_secret);
		bit_stream_hash_and_encrypt(out, connection::message_signature_bytes, encrypt_pos, &the_cipher);      
		
		conn->submit_request(out);
	}
	
	
	/// Handles a connection request from a remote host.
	///
	/// This will verify the validity of the connection token, as well as any solution
	/// to a client puzzle this interface sent to the remote host.  If those tests
	/// pass, it will construct a local connection instance to handle the rest of the
	/// connection negotiation.
	void handle_connect_request(const address &the_address, bit_stream &stream)
	{
		if(!_allow_connections)
			return;
		
		connection_parameters the_params;
		core::read(stream, the_params._nonce);
		core::read(stream, the_params._server_nonce);
		core::read(stream, the_params._client_identity);
		
		if(the_params._client_identity != compute_client_identity_token(the_address, the_params._nonce))
		{
			TorqueLogMessageFormatted(LogNetInterface, ("Client identity disagreement, params say %i, I say %i", the_params._client_identity, compute_client_identity_token(the_address, the_params._nonce)));
			return;
		}
		
		core::read(stream, the_params._puzzle_difficulty);
		core::read(stream, the_params._puzzle_solution);
		
		// see if the connection is in the main connection table.
		// If the connection is in the connection table and it has
		// the same initiatorSequence, we'll just resend the connect
		// acceptance packet, assuming that the last time we sent it
		// it was dropped.
		connection *connect = find_connection(the_address);
		if(connect)
		{
			connection_parameters &cp = connect->get_connection_parameters();
			if(cp._nonce == the_params._nonce && cp._server_nonce == the_params._server_nonce)
			{
				send_connect_accept(connect);
				return;
			}
		}
		
		// check the puzzle solution
		client_puzzle_manager::result_code result = _puzzle_manager.check_solution(the_params._puzzle_solution, the_params._nonce, the_params._server_nonce, the_params._puzzle_difficulty, the_params._client_identity);
		
		if(result != client_puzzle_manager::success)
		{
			const char *reason_buffer = "Puzzle";
			byte_buffer_ptr reason = new byte_buffer( (const uint8 *)reason_buffer, strlen(reason_buffer) + 1);
			send_connect_reject(&the_params, the_address, reason);
			return;
		}
		
		if(_private_key.is_null())
			return;
		
		the_params._public_key = new asymmetric_key(stream);
		the_params._private_key = _private_key;
		
		uint32 decrypt_pos = stream.get_next_byte_position();
		
		stream.set_byte_position(decrypt_pos);
		the_params._shared_secret = the_params._private_key->compute_shared_secret_key(the_params._public_key);
		//logprintf("shared secret (server) %s", the_params._shared_secret->encodeBase64()->get_buffer());
		
		symmetric_cipher the_cipher(the_params._shared_secret);
		
		if(!bit_stream_decrypt_and_check_hash(stream, connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;
		
		// now read the first part of the connection's symmetric key
		stream.read_bytes(the_params._symmetric_key, symmetric_cipher::key_size);
		_random_generator.random_buffer(the_params._init_vector, symmetric_cipher::key_size);
		
		uint32 connect_sequence;
		core::read(stream, connect_sequence);
		TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Request %8x", the_params._client_identity));
		
		if(connect)
			disconnect(connect, reason_self_disconnect, _new_connection_reason_buffer);
		
		connection *conn = new connection(_random_generator);
		
		if(!conn)
			return;
		
		ref_ptr<connection> the_connection = conn;
		conn->get_connection_parameters() = the_params;
		
		conn->set_address(the_address);
		conn->set_initial_recv_sequence(connect_sequence);
		conn->set_interface(this);
		
		conn->set_symmetric_cipher(new symmetric_cipher(the_params._symmetric_key, the_params._init_vector));
		
		byte_buffer_ptr reason;
		if(!conn->read_connect_request(stream, reason))
		{
			send_connect_reject(&the_params, the_address, reason);
			return;
		}

		add_pending_connection(conn);

		torque_socket_event event;
		event.event_type = torque_connection_requested_event_type;

		event.public_key_size = the_params._public_key->get_public_key()->get_buffer_size();
		memcpy(event.public_key, the_params._public_key->get_public_key()->get_buffer(), event.public_key_size);

		event.data_size = reason->get_buffer_size();
		memcpy(event.data, reason->get_buffer(), event.data_size);

		tnp_post_event(event, the_connection);
	}
	
	void tnp_accept_connection(const ref_ptr<connection>& conn, const byte_buffer_ptr& data)
	{
		connection* c = find_pending_connection(conn->get_address());
		if(!c)
		{
			TorqueLogMessageFormatted(LogNetInterface, ("Trying to accept a non-pending connection."));
			return;
		}
		
		conn->get_connection_parameters().connect_data = data;
		
		add_connection(conn);
		remove_pending_connection(conn);
		conn->set_connection_state(connection::connected);
		conn->on_connection_established();
		send_connect_accept(conn);
	}
	
	/// Sends a connect accept packet to acknowledge the successful acceptance of a connect request.
	void send_connect_accept(connection *conn)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Connect Accept - connection established."));
		
		packet_stream out;
		core::write(out, uint8(connect_accept_packet));
		connection_parameters &the_params = conn->get_connection_parameters();
		
		core::write(out, the_params._nonce);
		core::write(out, the_params._server_nonce);
		uint32 encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(encrypt_pos);
		
		core::write(out, conn->get_initial_send_sequence());
		conn->write_connect_accept(out);
		
		out.write_bytes(the_params._init_vector, symmetric_cipher::key_size);
		symmetric_cipher the_cipher(the_params._shared_secret);
		bit_stream_hash_and_encrypt(out, connection::message_signature_bytes, encrypt_pos, &the_cipher);

		out.send_to(_socket, conn->get_address());
	}
	
	/// Handles a connect accept packet, putting the connection associated with the
	/// remote host (if there is one) into an active state.
	void handle_connect_accept(const address &the_address, bit_stream &stream)
	{
		nonce nonce, server_nonce;
		
		core::read(stream, nonce);
		core::read(stream, server_nonce);
		uint32 decrypt_pos = stream.get_next_byte_position();
		stream.set_byte_position(decrypt_pos);
		
		connection *conn = find_pending_connection(the_address);
		if(!conn || conn->get_connection_state() != connection::awaiting_connect_response)
			return;
		
		connection_parameters &the_params = conn->get_connection_parameters();
		
		if(the_params._nonce != nonce || the_params._server_nonce != server_nonce)
			return;
		
		symmetric_cipher the_cipher(the_params._shared_secret);
		if(!bit_stream_decrypt_and_check_hash(stream, connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;

		uint32 recv_sequence;
		core::read(stream, recv_sequence);
		conn->set_initial_recv_sequence(recv_sequence);
		
		byte_buffer_ptr error_buffer;
		if(!conn->read_connect_accept(stream, error_buffer))
		{
			remove_pending_connection(conn);
			return;
		}

		stream.read_bytes(the_params._init_vector, symmetric_cipher::key_size);
		conn->set_symmetric_cipher(new symmetric_cipher(the_params._symmetric_key, the_params._init_vector));
		
		add_connection(conn); // first, add it as a regular connection
		remove_pending_connection(conn); // remove from the pending connection list
		
		conn->clear_request();
		conn->set_connection_state(connection::connected);
		conn->on_connection_established(); // notify the connection that it has been established
		TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Accept - connection established."));

		torque_socket_event event;
		event.event_type = torque_connection_accepted_event_type;
		event.client_identity = the_params._client_identity;
		event.data_size = error_buffer->get_buffer_size();
		memcpy(event.data, error_buffer->get_buffer(), event.data_size);

		tnp_post_event(event, conn);
	}
	
	void tnp_reject_connection(const address& the_address, const byte_buffer_ptr& data)
	{
		connection* conn = find_pending_connection(the_address);
		if(!conn)
		{
			TorqueLogMessageFormatted(LogNetInterface, ("Trying to rejected a non-pending connection."));
			return;
		}
		
		send_connect_reject(&(conn->get_connection_parameters()), the_address, data);
		remove_pending_connection(conn);
	}
	
	/// Sends a connect rejection to a valid connect request in response to possible error
	/// conditions (server full, wrong password, etc).
	void send_connect_reject(connection_parameters *the_params, const address &the_address, const byte_buffer_ptr &reason)
	{
		packet_stream out;
		core::write(out, uint8(connect_reject_packet));
		core::write(out, the_params->_nonce);
		core::write(out, the_params->_server_nonce);
		core::write(out, reason);
		out.send_to(_socket, the_address);
	}
	
	
	/// Handles a connect rejection packet by notifying the connection ref_object
	/// that the connection was rejected.
	void handle_connect_reject(const address &the_address, bit_stream &stream)
	{
		nonce the_nonce;
		nonce server_nonce;
		
		core::read(stream, the_nonce);
		core::read(stream, server_nonce);
		
		connection *conn = find_pending_connection(the_address);
		if(!conn || (conn->get_connection_state() != connection::awaiting_challenge_response &&
					 conn->get_connection_state() != connection::awaiting_connect_response))
			return;
		connection_parameters &p = conn->get_connection_parameters();
		if(p._nonce != the_nonce || p._server_nonce != server_nonce)
			return;
		
		byte_buffer_ptr reason;
		core::read(stream, reason);
		
		TorqueLogMessageFormatted(LogNetInterface, ("Received Connect Reject - reason %s", reason->get_buffer()));
		// if the reason is a bad puzzle solution, try once more with a
		// new nonce.
		if(!strcmp((const char *) reason->get_buffer(), "Puzzle") && !p._puzzle_retried)
		{
			p._puzzle_retried = true;
			conn->set_connection_state(connection::awaiting_challenge_response);
			conn->_connect_send_count = 0;
			_random_generator.random_buffer((uint8 *) &(p._nonce), sizeof(nonce));
			send_connect_challenge_request(conn);
			return;
		}
		
		conn->clear_request();
		conn->set_connection_state(connection::connect_rejected);
		conn->on_connect_terminated(reason_remote_host_rejected_connection, reason);
		remove_pending_connection(conn);

		torque_socket_event event;
		event.event_type = torque_connection_rejected_event_type;
		event.data_size = reason->get_buffer_size();
		memcpy(event.data, reason->get_buffer(), event.data_size);

		tnp_post_event(event, conn);
	}
	
	
	/// Begins the connection handshaking process for an arranged connection.
	void start_arranged_connection(connection *conn)
	{
		
		conn->set_connection_state(connection::sending_punch_packets);
		add_pending_connection(conn);
		conn->_connect_send_count = 0;
		conn->_connect_last_send_time = get_process_start_time();
		send_punch_packets(conn);
	}
	
	void request_introduction(connection* conn, uint32 identity)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Introduction Request to %s", conn->get_address().to_string().c_str()));
		packet_stream out;
		core::write(out, uint8(introduction_request_packet));
		core::write(out, identity);
		conn->submit_request(out);
	}
	
	void handle_introduction_request(const address& addr, bit_stream& stream)
	{
		connection* our_conn = find_connection(addr);
		if(!our_conn)
		{
			TorqueLogMessageFormatted(LogNetInterface, ("Failed to find connection"));
			return;
		}
				
		TorqueLogMessageFormatted(LogNetInterface, ("Received Introduction Request from %s", addr.to_string().c_str()));
		uint32 identity;
		core::read(stream, identity);
		connection* remote_conn = NULL;
		for(int32 i = 0; i < _connection_list.size(); ++i)
		{
			TorqueLogMessageFormatted(LogNetInterface, ("Checking connection with identity %u", _connection_list[i]->get_connection_parameters()._client_identity));
			if(_connection_list[i]->get_connection_parameters()._client_identity == identity)
			{
				remote_conn = _connection_list[i];
				break;
			}
		}
		
		if(!remote_conn)
		{
			TorqueLogMessageFormatted(LogNetInterface, ("No connection found with identity %u", identity));
			return;
		}
		
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Punch Request to %s", remote_conn->get_address().to_string().c_str()));
		packet_stream out;
		bool initiator = false;
		core::write(out, uint8(send_punch_packet));
		core::write(out, addr);
		core::write(out, initiator);
		out.send_to(_socket, remote_conn->get_address());
		
		
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Punch Request to %s", addr.to_string().c_str()));
		packet_stream ret;
		initiator = true;
		core::write(ret, uint8(send_punch_packet));
		core::write(ret, remote_conn->get_address());
		core::write(ret, initiator);
		ret.send_to(_socket, our_conn->get_address());
	}
	
	void handle_send_punch_packet(const address& addr, bit_stream& stream)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Received request to send punch packets from %s", addr.to_string().c_str()));
		
		connection* our_conn = find_connection(addr);
		if(!our_conn)
		{
			TorqueLogMessageFormatted(LogNetInterface, ("handle_send_punch_packet: No connection"));
			return;
		}
		
		our_conn->clear_request();
		
		address remote_address;
		core::read(stream, remote_address);
		TorqueLogMessageFormatted(LogNetInterface, ("Sending punch packets to %s", remote_address.to_string().c_str()));
		
		bool initiator;
		core::read(stream, initiator);
		
		ref_ptr<connection> conn = new connection(random());
		
		conn->set_address(remote_address);
		conn->set_interface(this);
		add_pending_connection(conn);
		
		// This packet is going to the client we are attempting to connect to,
		// so if we are the initiator then it is not.
		initiator = !initiator;
		
		packet_stream out;
		core::write(out, uint8(punch_packet));
		core::write(out, initiator);
		conn->submit_request(out);
	}
	
	void handle_punch_packet(const address& addr, bit_stream& stream)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Received punch packet from %s", addr.to_string().c_str()));
		
		bool initiator;
		core::read(stream, initiator);
		if(initiator)
		{
			connection* conn = find_pending_connection(addr);
			if(!conn)
			{
				TorqueLogMessageFormatted(LogNetInterface, ("No pending connection for %s", addr.to_string().c_str()));
				return;
			}
			conn->clear_request();
			conn->connect(this, addr, new byte_buffer((uint8*)"", 1));
		}
	}
	
	/// Sends punch packets to each address in the possible connection address list.
	void send_punch_packets(connection *conn)
	{
		connection_parameters &the_params = conn->get_connection_parameters();
		packet_stream out;
		core::write(out, uint8(punch_packet));
		
		if(the_params._is_initiator)
			core::write(out, the_params._nonce);
		else
			core::write(out, the_params._server_nonce);
		
		uint32 encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(encrypt_pos);
		
		if(the_params._is_initiator)
			core::write(out, the_params._server_nonce);
		else
		{
			core::write(out, the_params._nonce);
			core::write(out, _private_key->get_public_key());
		}
		symmetric_cipher the_cipher(the_params._arranged_secret);
		bit_stream_hash_and_encrypt(out, connection::message_signature_bytes, encrypt_pos, &the_cipher);
		
		for(uint32 i = 0; i < the_params._possible_addresses.size(); i++)
		{
			out.send_to(_socket, the_params._possible_addresses[i]);
			
//			TorqueLogMessageFormatted(LogNetInterface, ("Sending punch packet (%s, %s) to %s",
//														BufferEncodeBase64(byte_buffer(the_params._nonce.data, nonce::NonceSize))->get_buffer(),
//														BufferEncodeBase64(byte_buffer(the_params._server_nonce.data, nonce::NonceSize))->get_buffer(),
//														the_params._possible_addresses[i].toString()));
		}
		conn->_connect_send_count++;
		conn->_connect_last_send_time = get_process_start_time();
	}
	
	
	/// Handles an incoming punch packet from a remote host.
	void handle_punch(const address &the_address, bit_stream &stream)
	{
		uint32 i, j;
		connection *conn;
		
		nonce first_nonce;
		core::read(stream, first_nonce);
		
		byte_buffer b((uint8 *) &first_nonce, sizeof(nonce));
		
//		TorqueLogMessageFormatted(LogNetInterface, ("Received punch packet from %s - %s", the_address.toString(), BufferEncodeBase64(b)->get_buffer()));
		
		for(i = 0; i < _pending_connections.size(); i++)
		{
			conn = _pending_connections[i];
			connection_parameters &the_params = conn->get_connection_parameters();
			
			if(conn->get_connection_state() != connection::sending_punch_packets)
				continue;
			
			if((the_params._is_initiator && first_nonce != the_params._server_nonce) ||
			   (!the_params._is_initiator && first_nonce != the_params._nonce))
				continue;
			
			// first see if the address is in the possible addresses list:
			
			for(j = 0; j < the_params._possible_addresses.size(); j++)
				if(the_address == the_params._possible_addresses[j])
					break;
			
			// if there was an exact match, just exit the loop, or
			// continue on to the next pending if this is not an initiator:
			if(j != the_params._possible_addresses.size())
			{
				if(the_params._is_initiator)
					break;
				else
					continue;
			}
			
			// if there was no exact match, we may have a funny NAT in the
			// middle.  But since a packet got through from the remote host
			// we'll want to send a punch to the address it came from, as long
			// as only the port is not an exact match:
			for(j = 0; j < the_params._possible_addresses.size(); j++)
				if(the_address.is_same_host(the_params._possible_addresses[j]))
					break;
			
			// if the address wasn't even partially in the list, just exit out
			if(j == the_params._possible_addresses.size())
				continue;
			
			// otherwise, as long as we don't have too many ping addresses,
			// add this one to the list:
			if(the_params._possible_addresses.size() < 5)
				the_params._possible_addresses.push_back(the_address);      
			
			// if this is the initiator of the arranged connection, then
			// process the punch packet from the remote host by issueing a
			// connection request.
			if(the_params._is_initiator)
				break;
		}
		if(i == _pending_connections.size())
			return;
		
		connection_parameters &the_params = conn->get_connection_parameters();
		symmetric_cipher the_cipher(the_params._arranged_secret);
		if(!bit_stream_decrypt_and_check_hash(stream, connection::message_signature_bytes, stream.get_next_byte_position(), &the_cipher))
			return;
		
		nonce next_nonce;
		core::read(stream, next_nonce);
		
		if(next_nonce != the_params._nonce)
			return;
		
		// see if the connection needs to be authenticated or uses key exchange
		if(the_params._is_initiator)
		{
			the_params._public_key = new asymmetric_key(stream);
			if(!the_params._public_key->is_valid())
				return;
		}

		if(_private_key.is_null() || _private_key->get_key_size() != the_params._public_key->get_key_size())
		{
			// we don't have a private key, so generate one for this connection
			the_params._private_key = new asymmetric_key(the_params._public_key->get_key_size());
		}
		else
			the_params._private_key = _private_key;
		the_params._shared_secret = the_params._private_key->compute_shared_secret_key(the_params._public_key);
		//logprintf("shared secret (client) %s", the_params._shared_secret->encodeBase64()->get_buffer());
		_random_generator.random_buffer(the_params._symmetric_key, symmetric_cipher::key_size);

		conn->set_address(the_address);
		TorqueLogMessageFormatted(LogNetInterface, ("punch from %s matched nonces - connecting...", the_address.to_string().c_str()));
		
		conn->set_connection_state(connection::awaiting_connect_response);
		conn->_connect_send_count = 0;
		conn->_connect_last_send_time = get_process_start_time();
		
		send_arranged_connect_request(conn);
	}
	
	/// Sends an arranged connect request.
	void send_arranged_connect_request(connection *conn)
	{
		TorqueLogMessageFormatted(LogNetInterface, ("Sending Arranged Connect Request"));
		packet_stream out;
		
		connection_parameters &the_params = conn->get_connection_parameters();
		
		core::write(out, uint8(arranged_connect_request_packet));
		core::write(out, the_params._nonce);
		uint32 encrypt_pos = out.get_next_byte_position();
		
		out.set_byte_position(encrypt_pos);
		
		core::write(out, the_params._server_nonce);
		core::write(out, the_params._private_key->get_public_key());

		uint32 inner_encrypt_pos = out.get_next_byte_position();
		out.set_byte_position(inner_encrypt_pos);
		out.write_bytes(the_params._symmetric_key, symmetric_cipher::key_size);

		core::write(out, conn->get_initial_send_sequence());
		conn->write_connect_request(out);
		
		symmetric_cipher shared_secret_cipher(the_params._shared_secret);
		bit_stream_hash_and_encrypt(out, connection::message_signature_bytes, inner_encrypt_pos, &shared_secret_cipher);

		symmetric_cipher arranged_secret_cipher(the_params._arranged_secret);
		bit_stream_hash_and_encrypt(out, connection::message_signature_bytes, encrypt_pos, &arranged_secret_cipher);
		
		conn->_connect_send_count++;
		conn->_connect_last_send_time = get_process_start_time();
		
		out.send_to(_socket, conn->get_address());
	}
	
	/// Handles an incoming connect request from an arranged connection.
	void handle_arranged_connect_request(const address &the_address, bit_stream &stream)
	{
		uint32 i, j;
		connection *conn;
		nonce nonce, server_nonce;
		core::read(stream, nonce);
		
		// see if the connection is in the main connection table.
		// If the connection is in the connection table and it has
		// the same initiatorSequence, we'll just resend the connect
		// acceptance packet, assuming that the last time we sent it
		// it was dropped.
		connection *old_connection = find_connection(the_address);
		if(old_connection)
		{
			connection_parameters &cp = old_connection->get_connection_parameters();
			if(cp._nonce == nonce)
			{
				send_connect_accept(old_connection);
				return;
			}
		}
		
		for(i = 0; i < _pending_connections.size(); i++)
		{
			conn = _pending_connections[i];
			connection_parameters &the_params = conn->get_connection_parameters();
			
			if(conn->get_connection_state() != connection::sending_punch_packets || the_params._is_initiator)
				continue;
			
			if(nonce != the_params._nonce)
				continue;
			
			for(j = 0; j < the_params._possible_addresses.size(); j++)
				if(the_address.is_same_host(the_params._possible_addresses[j]))
					break;
			if(j != the_params._possible_addresses.size())
				break;
		}
		if(i == _pending_connections.size())
			return;
		
		connection_parameters &the_params = conn->get_connection_parameters();
		symmetric_cipher arraged_secret_cipher(the_params._arranged_secret);
		if(!bit_stream_decrypt_and_check_hash(stream, connection::message_signature_bytes, stream.get_next_byte_position(), &arraged_secret_cipher))
			return;
		
		stream.set_byte_position(stream.get_next_byte_position());
		
		core::read(stream, server_nonce);
		if(server_nonce != the_params._server_nonce)
			return;
		
		if(_private_key.is_null())
			return;
		the_params._public_key = new asymmetric_key(stream);
		the_params._private_key = _private_key;
		
		uint32 decrypt_pos = stream.get_next_byte_position();
		stream.set_byte_position(decrypt_pos);
		the_params._shared_secret = the_params._private_key->compute_shared_secret_key(the_params._public_key);
		symmetric_cipher shared_secret_cipher(the_params._shared_secret);
		
		if(!bit_stream_decrypt_and_check_hash(stream, connection::message_signature_bytes, decrypt_pos, &shared_secret_cipher))
			return;
		
		// now read the first part of the connection's session (symmetric) key
		stream.read_bytes(the_params._symmetric_key, symmetric_cipher::key_size);
		_random_generator.random_buffer(the_params._init_vector, symmetric_cipher::key_size);
		
		uint32 connect_sequence;
		core::read(stream, connect_sequence);
		TorqueLogMessageFormatted(LogNetInterface, ("Received Arranged Connect Request"));
		
		if(old_connection)
			disconnect(old_connection, reason_self_disconnect, _old_connection_reason_buffer);
		
		conn->set_address(the_address);
		conn->set_initial_recv_sequence(connect_sequence);

		conn->set_symmetric_cipher(new symmetric_cipher(the_params._symmetric_key, the_params._init_vector));
		
		byte_buffer_ptr reason;
		if(!conn->read_connect_request(stream, reason))
		{
			send_connect_reject(&the_params, the_address, reason);
			remove_pending_connection(conn);
			return;
		}
		add_connection(conn);
		remove_pending_connection(conn);
		conn->set_connection_state(connection::connected);
		conn->on_connection_established();
		send_connect_accept(conn);
	}
	
	
	/// Dispatches a disconnect packet for a specified connection.
	void handle_disconnect(const address &the_address, bit_stream &stream)
	{
		connection *conn = find_connection(the_address);
		if(!conn)
			return;
		
		connection_parameters &the_params = conn->get_connection_parameters();
		
		nonce nonce, server_nonce;
		byte_buffer_ptr reason;
		
		core::read(stream, nonce);
		core::read(stream, server_nonce);
		
		if(nonce != the_params._nonce || server_nonce != the_params._server_nonce)
			return;
		
		uint32 decrypt_pos = stream.get_next_byte_position();
		stream.set_byte_position(decrypt_pos);
		
		symmetric_cipher the_cipher(the_params._shared_secret);
		if(!bit_stream_decrypt_and_check_hash(stream, connection::message_signature_bytes, decrypt_pos, &the_cipher))
			return;

		core::read(stream, reason);
		
		conn->set_connection_state(connection::disconnected);
		conn->on_connection_terminated(reason_remote_disconnect_packet, reason);
		remove_connection(conn);
	}
	
	/// Handles an error reported while reading a packet from this remote connection.
	void handle_connection_error(connection *the_connection, byte_buffer_ptr &reason)
	{
		disconnect(the_connection, reason_error, reason);
	}
	
	/// @}

	/// Processes a single packet, and dispatches either to handle_info_packet or to
	/// the connection associated with the remote address.
	void process_packet(const address &the_address, bit_stream &packet_stream)
	{
		
		// Determine what to do with this packet:
		
		if(packet_stream.get_buffer()[0] & 0x80) // it's a protocol packet...
		{
			// if the LSB of the first byte is set, it's a game data packet
			// so pass it to the appropriate connection.
			
			// lookup the connection in the addressTable
			// if this packet causes a disconnection, keep the conn around until this function exits
			ref_ptr<connection> conn = find_connection(the_address);
			if(conn)
				conn->read_raw_packet(packet_stream);
		}
		else
		{
			// Otherwise, it's either a game info packet or a
			// connection handshake packet.
			
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
	
	
	
	/// Returns the list of connections on this interface.
	array<ref_ptr<connection> > &get_connection_list() { return _connection_list; }
	
	/// looks up a connected connection on this interface
	connection *find_connection(const address &remote_address)
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
	
};


