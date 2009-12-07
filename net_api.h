// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

// This file holds the C API description for the network platform

class tnp_address
{
	uint32 host;
	uint16 port;

	address(host, port)
	operator==(tnp_address)
	operator!=(tnp_address)
	get_port()
	get_host()
	set_port(port)
	set_host(host)
	hash()
};

// C API:
enum {
	tnp_max_datagram_size = udp_socket::max_datagram_size - connection::packet_header_byte_size - connection::message_signature_bytes - 10,
	tnp_max_status_datagram_size = 512,
}

struct tnp_event
{
	enum event_types
	{
		connection_accepted_event,
		connection_rejected_event,
		connection_requested_event,
		connection_timed_out_event,
		connection_disconnected_event,
		connection_established_event,
		connection_packet_event,
		connection_packet_notify_event,
		interface_packet_event,
	};
	event_types event_type;
	connection *source_connection;
	address source_address;
	uint32 packet_sequence;
	uint32 data_size;
	uint8 data[tnp_max_datagram_size];
};

struct tnp_connection_state
{
	// round trip time
	// packet loss
	// throughput
	// connection state
};

interface tnp_create_interface(bind_address);
void tnp_seed_entropy(interface, uint8 entropy[32]);
void tnp_read_entropy(interface, uint8 entropy[32]);
void tnp_allow_incoming_connections(interface, bool allowed);
bool tnp_does_allow_incoming_connections(interface);
bool tnp_get_next_event(interface, tnp_event &the_event);
connection tnp_connect(address remote_host, uint32 connect_data_size, uint8 connect_data[tnp_max_status_datagram_size]);
connection tnp_accept_connection(tnp_event &connect_request_event, uint32 connect_accept_data_size, uint8 connect_accept_data[tnp_max_status_datagram_size]);
void tnp_reject_connection(tnp_event &connect_request_event, uint32 reject_data_size, uint8 reject_data[tnp_max_status_datagram_size]);
void tnp_disconnect(connection the_connection, uint32 disconnect_data_size, uint8 disconnect_data[tnp_max_status_datagram_size]);
void tnp_send_to(connection the_connection, uint32 datagram_size, uint8 buffer[tnp_max_datagram_size]);
void tnp_get_connection_state(connection the_connection);