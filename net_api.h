// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

// This file holds the high-level C++ API description for the network platform

class address
{
	uint32 host;
	uint16 port;

	address(name, port)
	address(name_string, allow_dns, port)
	operator==(address)
	operator!=(address)
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

interface tnp_create_interface();
void tnp_seed_entropy(interface, uint8 entropy[32]);
void tnp_read_entropy(interface, uint8 entropy[32]);
void tnp_bind_interface(interface, address);
bool tnp_get_next_event(interface, tnp_event &the_event);
connection tnp_connect(address remote_host, uint32 status_buffer_size, uint8 status_buffer[tnp_max_status_datagram_size]);
connection tnp_accept_connection(tnp_event &connect_request_event, uint32 status_buffer_size, uint8 status_buffer[tnp_max_status_datagram_size]);
void tnp_disconnect(connection the_connection, uint32 status_buffer_size, uint8 status_buffer[tnp_max_status_datagram_size]);
void tnp_send_to(connection the_connection, uint32 datagram_size, uint8 buffer[tnp_max_datagram_size]);
void tnp_get_connection_state(connection the_connection);

class interface
{
	interface();
	bind(port);
	bind(address bind_address);
	
	// a single interface is not thread safe, although you can have different
	// interfaces used by different threads
	
	typedef void (*packet_process_function)(interface, address, buffer);
	typedef void (*connect_request_process_function)(interface, address, buffer);
		
	void set_packet_process_function(packet_process_function the_func);
	void set_connect_request_process_function(connect_request_process_function the_func);
	
	connection *accept_connection(buffer);
	void reject_connection(buffer);

	void process()
		// checks and disptaches incoming packets
		// processes connection requests and timeouts
	    // continues puzzle solutions
	
	connection *connect(address remote_host, buffer)
	connection *connect_arranged(client_identity_buffer, connection *referring_connection, buffer)  
}

class connection
{
	result send(byte *buffer, uint32 size, uint32 delay=0, uint32 *sequence=0);
	bool can_send();
	
	typedef void (*on_connection_terminated_fn)(connection, termination_reason, buffer);
	typedef void (*on_packet_fn)(connection, uint32 sequence, buffer);
	typedef void (*on_connection_established_fn)(connection, buffer);
	typedef void (*on_send_notify_fn)(connection, uint32 sequence, bool recvd);
	
	statistics get_statistics();
	bool is_established();
	state get_state();
}