// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.
// torque sockets, connected unreliable datagram and notification protocol
// basic C API

typedef unsigned torque_connection_id;
static const torque_connection_id invalid_torque_connection = 0;
	
enum torque_sockets_constants {
	torque_sockets_max_datagram_size = 1480,
	torque_sockets_max_status_datagram_size = 511,
	torque_sockets_max_public_key_size = 512,
	torque_sockets_packet_window_size = 31,
	torque_sockets_info_packet_first_byte_min = 32,
	torque_sockets_info_packet_first_byte_max = 127,
};
enum torque_socket_event_type
{
	torque_connection_challenge_response_event_type = 1,
	torque_connection_requested_event_type,
	torque_connection_arranged_connection_request_event_type,
	torque_connection_accepted_event_type,
	torque_connection_rejected_event_type,
	torque_connection_timed_out_event_type,
	torque_connection_disconnected_event_type,
	torque_connection_established_event_type,
	torque_connection_packet_event_type,
	torque_connection_packet_notify_event_type,
	torque_socket_packet_event_type,
};

struct torque_socket_event
{
	unsigned event_type;
	torque_connection_id connection;
	torque_connection_id arranger_connection;
	unsigned client_identity;
	unsigned connection_token;
	unsigned public_key_size;
	unsigned char *public_key;
	unsigned data_size;
	unsigned char *data;
	unsigned packet_sequence;
	int delivered;
	sockaddr source_address;
};
