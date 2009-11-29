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
	interface();
	bind(port);
	bind(address bind_address);
	
	// a single interface is not thread safe, although you can have different
	// interfaces used by different threads
	
	set_certificate(certificate *the_cert)
	
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
	
	connection *coonect(address remote_host, buffer)
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