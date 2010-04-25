torque_socket_handle torque_socket_create(bool background_thread, void (*socket_notify)(void *), void *socket_notify_data)
{
	core::net::torque_socket *ret = new core::net::torque_socket(background_thread, socket_notify, socket_notify_data);
	return (void *) ret;

}

void torque_socket_destroy(torque_socket_handle the_socket)
{
	delete (core::net::torque_socket *) the_socket;
}

bind_result torque_socket_bind(torque_socket_handle the_socket, struct sockaddr *bound_interface_address)
{
	core::net::address a(*bound_interface_address);
	return ((core::net::torque_socket *) the_socket)->bind(a);
}

void torque_socket_allow_incoming_connections(torque_socket_handle the_socket, int allowed)
{
	((core::net::torque_socket *) the_socket)->set_allows_connections(allowed);
}

void torque_socket_set_key_pair(torque_socket_handle the_socket, unsigned key_data_size, unsigned char *the_key)
{
	core::net::asymmetric_key *key = new core::net::asymmetric_key(the_key, key_data_size);

	((core::net::torque_socket *) the_socket)->set_private_key(key);
}

void torque_socket_set_challenge_response(torque_socket_handle the_socket, unsigned challenge_response_size, unsigned char *challenge_response)
{

	((core::net::torque_socket *) the_socket)->set_challenge_response(new core::byte_buffer(challenge_response, challenge_response_size));
}

void torque_socket_write_entropy(torque_socket_handle the_socket, unsigned char entropy[32])
{
	core::net::random_generator &r = ((core::net::torque_socket *) the_socket)->random();
	r.add_entropy(entropy, 32);

}

void torque_socket_read_entropy(torque_socket_handle the_socket, unsigned char entropy[32])
{
	core::net::random_generator &r = ((core::net::torque_socket *) the_socket)->random();
	r.random_buffer(entropy, 32);
}

int torque_socket_send_to(torque_socket_handle the_socket, struct sockaddr* remote_host, unsigned data_size, unsigned char *data)
{
	core::net::address a(*remote_host);
	core::net::udp_socket::send_to_result r = ((core::net::torque_socket *) the_socket)->send_to(a, data_size, data);
	return r;
}

torque_connection_id torque_socket_connect(torque_socket_handle the_socket, struct sockaddr* remote_host, unsigned connect_data_size, unsigned char *connect_data)
{
	core::net::address a(*remote_host);
	return ((core::net::torque_socket *) the_socket)->connect(a, connect_data, connect_data_size);
}

torque_connection_id torque_socket_connect_introduced(torque_socket_handle the_socket, torque_connection_id introducer, torque_connection_id remote_client_identity, int is_host, unsigned connect_data_size, unsigned char *connect_data)
{
	return  ((core::net::torque_socket *) the_socket)->connect_introduced(introducer, remote_client_identity,  is_host, connect_data_size, connect_data);
}

void torque_socket_introduce(torque_socket_handle the_socket, torque_connection_id initiator, torque_connection_id host)
{
	 ((core::net::torque_socket *) the_socket)->introduce_connection(initiator, host);
}

void torque_socket_accept_challenge(torque_socket_handle the_socket, torque_connection_id pending_connection)
{
	 ((core::net::torque_socket *) the_socket)-> accept_connection_challenge(pending_connection);
}

void torque_socket_accept_connection(torque_socket_handle the_socket, torque_connection_id pending)
{
	((core::net::torque_socket *) the_socket)->accept_connection( pending);
}

void torque_socket_close_connection(torque_socket_handle the_socket, torque_connection_id connection_id, unsigned disconnect_data_size, unsigned char *disconnect_data)
{
	((core::net::torque_socket *) the_socket)->disconnect(connection_id, disconnect_data,  disconnect_data_size);
}

struct torque_socket_event *torque_socket_get_next_event(torque_socket_handle the_socket)
{
	return ((core::net::torque_socket *) the_socket)->get_next_event();
}

int torque_socket_send_to_connection(torque_socket_handle the_socket, torque_connection_id connection_id, unsigned datagram_size, unsigned char buffer[torque_sockets_max_datagram_size])
{
	unsigned sequence;
	((core::net::torque_socket *) the_socket)->send_to_connection(connection_id, buffer, datagram_size, &sequence);
	return sequence;
}
