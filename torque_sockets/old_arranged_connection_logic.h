/// Begins the connection handshaking process for an arranged connection.
void start_arranged_connection(torque_connection *conn)
{
	conn->set_connection_state(torque_connection::sending_punch_packets);
	add_pending_connection(conn);
	conn->_connect_send_count = 0;
	conn->_connect_last_send_time = get_process_start_time();
	send_punch_packets(conn);
}

void request_introduction(torque_connection* conn, uint32 identity)
{
	TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Introduction Request to %s", conn->get_address().to_string().c_str()));
	packet_stream out;
	core::write(out, uint8(introduction_request_packet));
	core::write(out, identity);
	conn->submit_request(out);
}

void handle_introduction_request(const address& addr, bit_stream& stream)
{
	torque_connection* our_conn = find_connection(addr);
	if(!our_conn)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Failed to find connection"));
		return;
	}
	
	TorqueLogMessageFormatted(LogNettorque_socket, ("Received Introduction Request from %s", addr.to_string().c_str()));
	uint32 identity;
	core::read(stream, identity);
	torque_connection* remote_conn = NULL;
	for(int32 i = 0; i < _connection_list.size(); ++i)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("Checking connection with identity %u", _connection_list[i]->get_connection_parameters()._client_identity));
		if(_connection_list[i]->get_connection_parameters()._client_identity == identity)
		{
			remote_conn = _connection_list[i];
			break;
		}
	}
	
	if(!remote_conn)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("No connection found with identity %u", identity));
		return;
	}
	
	TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Punch Request to %s", remote_conn->get_address().to_string().c_str()));
	packet_stream out;
	bool initiator = false;
	core::write(out, uint8(send_punch_packet));
	core::write(out, addr);
	core::write(out, initiator);
	out.send_to(_socket, remote_conn->get_address());
	
	
	TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Punch Request to %s", addr.to_string().c_str()));
	packet_stream ret;
	initiator = true;
	core::write(ret, uint8(send_punch_packet));
	core::write(ret, remote_conn->get_address());
	core::write(ret, initiator);
	ret.send_to(_socket, our_conn->get_address());
}

void handle_send_punch_packet(const address& addr, bit_stream& stream)
{
	TorqueLogMessageFormatted(LogNettorque_socket, ("Received request to send punch packets from %s", addr.to_string().c_str()));
	
	torque_connection* our_conn = find_connection(addr);
	if(!our_conn)
	{
		TorqueLogMessageFormatted(LogNettorque_socket, ("handle_send_punch_packet: No connection"));
		return;
	}
	
	our_conn->clear_request();
	
	address remote_address;
	core::read(stream, remote_address);
	TorqueLogMessageFormatted(LogNettorque_socket, ("Sending punch packets to %s", remote_address.to_string().c_str()));
	
	bool initiator;
	core::read(stream, initiator);
	
	ref_ptr<torque_connection> conn = new torque_connection(_random_generator.random_nonce(), _random_generator.random_integer(), _next_connection_index, !initiator);
	
	conn->set_address(remote_address);
	conn->set_torque_socket(this);
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
	TorqueLogMessageFormatted(LogNettorque_socket, ("Received punch packet from %s", addr.to_string().c_str()));
	
	bool initiator;
	core::read(stream, initiator);
	if(initiator)
	{
		torque_connection* conn = find_pending_connection(addr);
		if(!conn)
		{
			TorqueLogMessageFormatted(LogNettorque_socket, ("No pending connection for %s", addr.to_string().c_str()));
			return;
		}
		conn->clear_request();
		//conn->connect(this, addr, new byte_buffer((uint8*)"", 1));
	}
}

/// Sends punch packets to each address in the possible connection address list.
void send_punch_packets(torque_connection *conn)
{
	packet_stream out;
	core::write(out, uint8(punch_packet));
	
	if(conn->is_initiator())
		core::write(out, conn->get_initiator_nonce());
	else
		core::write(out, conn->get_host_nonce());
	
	uint32 encrypt_pos = out.get_next_byte_position();
	out.set_byte_position(encrypt_pos);
	
	if(conn->is_initiator())
		core::write(out, conn->get_host_nonce());
	else
	{
		core::write(out, conn->get_initiator_nonce());
		core::write(out, _private_key->get_public_key());
	}
	connection_parameters &the_params = conn->get_connection_parameters();
	symmetric_cipher the_cipher(the_params._arranged_secret);
	bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &the_cipher);
	
	for(uint32 i = 0; i < the_params._possible_addresses.size(); i++)
	{
		out.send_to(_socket, the_params._possible_addresses[i]);
		
		//			TorqueLogMessageFormatted(LogNettorque_socket, ("Sending punch packet (%s, %s) to %s",
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
	torque_connection *conn;
	
	nonce first_nonce;
	core::read(stream, first_nonce);
	
	//		byte_buffer b((uint8 *) &first_nonce, sizeof(nonce));
	
	//		TorqueLogMessageFormatted(LogNettorque_socket, ("Received punch packet from %s - %s", the_address.toString(), BufferEncodeBase64(b)->get_buffer()));
	
	for(i = 0; i < _pending_connections.size(); i++)
	{
		conn = _pending_connections[i];
		if(conn->get_connection_state() != torque_connection::sending_punch_packets)
			continue;
		
		if((conn->is_initiator() && first_nonce != conn->get_host_nonce()) ||
		   (!conn->is_initiator() && first_nonce != conn->get_initiator_nonce()))
			continue;
		
		// first see if the address is in the possible addresses list:
		connection_parameters &the_params = conn->get_connection_parameters();
		for(j = 0; j < the_params._possible_addresses.size(); j++)
			if(the_address == the_params._possible_addresses[j])
				break;
		
		// if there was an exact match, just exit the loop, or continue on to the next pending if this is not an initiator:
		if(j != the_params._possible_addresses.size())
		{
			if(conn->is_initiator())
				break;
			else
				continue;
		}
		
		// if there was no exact match, we may have a funny NAT in the middle.  But since a packet got through from the remote host  we'll want to send a punch to the address it came from, as long
		// as only the port is not an exact match:
		for(j = 0; j < the_params._possible_addresses.size(); j++)
			if(the_address.is_same_host(the_params._possible_addresses[j]))
				break;
		
		// if the address wasn't even partially in the list, just exit out
		if(j == the_params._possible_addresses.size())
			continue;
		
		// otherwise, as long as we don't have too many ping addresses, add this one to the list:
		if(the_params._possible_addresses.size() < 5)
			the_params._possible_addresses.push_back(the_address);      
		
		// if this is the initiator of the arranged connection, then process the punch packet from the remote host by issueing a connection request.
		if(conn->is_initiator())
			break;
	}
	if(i == _pending_connections.size())
		return;
	
	connection_parameters &the_params = conn->get_connection_parameters();
	symmetric_cipher the_cipher(the_params._arranged_secret);
	if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, stream.get_next_byte_position(), &the_cipher))
		return;
	
	nonce next_nonce;
	core::read(stream, next_nonce);
	
	if(next_nonce != conn->get_initiator_nonce())
		return;
	
	// see if the connection needs to be authenticated or uses key exchange
	if(conn->is_initiator())
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
	conn->set_shared_secret(the_params._private_key->compute_shared_secret_key(the_params._public_key));
	//logprintf("shared secret (client) %s", conn->get_shared_secret()->encodeBase64()->get_buffer());
	_random_generator.random_buffer(the_params._symmetric_key, symmetric_cipher::key_size);
	
	conn->set_address(the_address);
	TorqueLogMessageFormatted(LogNettorque_socket, ("punch from %s matched nonces - connecting...", the_address.to_string().c_str()));
	
	conn->set_connection_state(torque_connection::awaiting_connect_response);
	conn->_connect_send_count = 0;
	conn->_connect_last_send_time = get_process_start_time();
	
	send_arranged_connect_request(conn);
}

/// Sends an arranged connect request.
void send_arranged_connect_request(torque_connection *conn)
{
	TorqueLogMessageFormatted(LogNettorque_socket, ("Sending Arranged Connect Request"));
	packet_stream out;
	
	connection_parameters &the_params = conn->get_connection_parameters();
	
	core::write(out, uint8(arranged_connect_request_packet));
	core::write(out, conn->get_initiator_nonce());
	uint32 encrypt_pos = out.get_next_byte_position();
	
	out.set_byte_position(encrypt_pos);
	
	core::write(out, conn->get_host_nonce());
	core::write(out, the_params._private_key->get_public_key());
	
	uint32 inner_encrypt_pos = out.get_next_byte_position();
	out.set_byte_position(inner_encrypt_pos);
	out.write_bytes(the_params._symmetric_key, symmetric_cipher::key_size);
	
	core::write(out, conn->get_initial_send_sequence());
	core::write(out, the_params.connect_data);
	
	symmetric_cipher shared_secret_cipher(conn->get_shared_secret());
	bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, inner_encrypt_pos, &shared_secret_cipher);
	
	symmetric_cipher arranged_secret_cipher(the_params._arranged_secret);
	bit_stream_hash_and_encrypt(out, torque_connection::message_signature_bytes, encrypt_pos, &arranged_secret_cipher);
	
	conn->_connect_send_count++;
	conn->_connect_last_send_time = get_process_start_time();
	
	out.send_to(_socket, conn->get_address());
}

/// Handles an incoming connect request from an arranged connection.
void handle_arranged_connect_request(const address &the_address, bit_stream &stream)
{
	uint32 i, j;
	torque_connection *conn;
	nonce initiator_nonce, host_nonce;
	core::read(stream, initiator_nonce);
	
	// see if the connection is in the main connection table.  If the connection is in the connection table and it has the same initiator nonce, we'll just resend the connect acceptance packet, assuming that the last time we sent it it was dropped.
	torque_connection *old_connection = find_connection(the_address);
	if(old_connection && old_connection->get_initiator_nonce() == initiator_nonce)
	{
		send_connect_accept(old_connection);
		return;
	}
	
	for(i = 0; i < _pending_connections.size(); i++)
	{
		conn = _pending_connections[i];
		connection_parameters &the_params = conn->get_connection_parameters();
		
		if(conn->get_connection_state() != torque_connection::sending_punch_packets || conn->is_initiator())
			continue;
		
		if(initiator_nonce != conn->get_initiator_nonce())
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
	if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, stream.get_next_byte_position(), &arraged_secret_cipher))
		return;
	
	stream.set_byte_position(stream.get_next_byte_position());
	
	core::read(stream, host_nonce);
	if(host_nonce != conn->get_host_nonce())
		return;
	
	if(_private_key.is_null())
		return;
	the_params._public_key = new asymmetric_key(stream);
	the_params._private_key = _private_key;
	
	uint32 decrypt_pos = stream.get_next_byte_position();
	stream.set_byte_position(decrypt_pos);
	conn->set_shared_secret(the_params._private_key->compute_shared_secret_key(the_params._public_key));
	symmetric_cipher shared_secret_cipher(conn->get_shared_secret());
	
	if(!bit_stream_decrypt_and_check_hash(stream, torque_connection::message_signature_bytes, decrypt_pos, &shared_secret_cipher))
		return;
	
	// now read the first part of the connection's session (symmetric) key
	stream.read_bytes(the_params._symmetric_key, symmetric_cipher::key_size);
	_random_generator.random_buffer(the_params._init_vector, symmetric_cipher::key_size);
	
	uint32 connect_sequence;
	core::read(stream, connect_sequence);
	TorqueLogMessageFormatted(LogNettorque_socket, ("Received Arranged Connect Request"));
	
	if(old_connection)
		disconnect(old_connection, reason_self_disconnect, _old_connection_reason_buffer);
	
	conn->set_address(the_address);
	conn->set_initial_recv_sequence(connect_sequence);
	
	conn->set_symmetric_cipher(new symmetric_cipher(the_params._symmetric_key, the_params._init_vector));
	
	byte_buffer_ptr connect_request_data;
	core::read(stream, connect_request_data);
	
	// this is wrong now, FIXME -- should post a connect request event
	/*
	 if(!conn->read_connect_request(stream, reason))
	 {
	 send_connect_reject(initiator_nonce, host_nonce, the_address, reason);
	 remove_pending_connection(conn);
	 return;
	 }
	 add_connection(conn);
	 remove_pending_connection(conn);
	 conn->set_connection_state(torque_connection::connected);
	 post_connection_event(conn, torque_connection_established_event_type, 0);
	 send_connect_accept(conn);*/
}

