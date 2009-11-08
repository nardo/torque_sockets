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

class interface
{
	open? network adapter binding
	open? crypto API
	// a single interface is not thread safe, although you can have different
	// interfaces used by different threads
	
	set_private_key(key_pair *the_key)
	set_requires_key_exchange(bool)
	set_certificate()
	bool allows_coonections()
	set_allows_connections(bool)
	
	result sendto(address, byte *buffer, uint32 size, uint32 delay=0)
	
	typedef void (*packet_process_function)(interface, address, byte *buffer, uint32 packet_size);
	typedef void (*connect_request_process_function)(interface, address, byte *request_buffer, uint32 request_size);
	
		
	void set_packet_process_function(packet_process_function the_func);
	void set_connect_request_process_function(connect_request_process_function the_func);
	
	connection *accept_connection(byte *response, uint32 response_len);
	void reject_connection(byte *response, uint32 response_len);

	void process()
		// checks and disptaches incoming packets
		// processes connection requests and timeouts
	    // continues puzzle solutions
	
	connection *coonect(address remote_host, byte *buffer, uint32 size)
}

class connection
{
	result send(byte *buffer, uint32 size, uint32 delay=0, uint32 *sequence=0);
	bool can_send();
	
	typedef void (*on_connection_terminated_fn)(connection, termination_reason, byte *buffer, uint32 size);
	typedef void (*on_packet_fn)(connection, uint32 sequence, byte *buffer, uint32 size);
	typedef void (*on_connection_established_fn)(connection);
	typedef void (*on_send_notify_fn)(connection, uint32 sequence, bool recvd);
	
	statistics get_statistics();
	bool is_established();
	state get_state();
	
	void set_fixed_rate_parameters(min_send_period, min_recv_period, max_send_bandwidth, max_recv_bandwidth, window_size=64)
}