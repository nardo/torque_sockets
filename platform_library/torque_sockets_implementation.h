// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.
// torque sockets, connected unreliable datagram and notification protocol
// basic C API

struct torque_socket_opaque
{
	
	
};

torque_socket torque_socket_create(struct sockaddr*)
{
	return 0;
}

void torque_socket_destroy(torque_socket)
{
	
}

void torque_socket_seed_entropy(torque_socket, unsigned char entropy[32])
{
	
}

void torque_socket_set_private_key(torque_socket, unsigned key_data_size, unsigned char *the_key)
{
	
}

void torque_socket_set_challenge_response_data(torque_socket, unsigned challenge_response_data_size, unsigned char *challenge_response_data)
{
	
}

void torque_socket_allow_incoming_connections(torque_socket, int allowed)
{
	
}

int torque_socket_does_allow_incoming_connections(torque_socket)
{
	
}

void torque_socket_introduce(torque_socket, torque_connection, torque_connection, unsigned connection_token)
{
	
}

torque_connection torque_connection_connect_introduced(torque_connection introducer, unsigned remote_client_identity, unsigned connection_token, unsigned connect_data_size, unsigned char *connect_data)
{
	
}

struct torque_socket_event *torque_socket_get_next_event(torque_socket)
{
	
}

int torque_socket_send_to(torque_socket, struct sockaddr* remote_host, unsigned data_size, unsigned char *data)
{
	
}

torque_connection torque_socket_connect(torque_socket, struct sockaddr* remote_host, unsigned connect_data_size, unsigned char *connect_data)
{
	
}

void torque_connection_accept(torque_connection, unsigned connect_accept_data_size, unsigned char *connect_accept_data)
{
	
}

void torque_connection_reject(torque_connection, unsigned reject_data_size, unsigned char *reject_data)
{
	
}

void torque_connection_disconnect(torque_connection, unsigned disconnect_data_size, unsigned char *disconnect_data)
{
	
}

int torque_connection_send_to(torque_connection, unsigned datagram_size, unsigned char buffer[torque_max_datagram_size], unsigned *sequence_number)
{
	
}
