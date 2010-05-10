// torque sockets_c_api.h - C declarations for torque sockets API structures, constants, entry points
// Copyright GarageGames.  torque sockets API and prototype implementation are released under the MIT license.  See /license/info.txt in this distribution for specific details.


typedef void *torque_socket_handle;
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
	torque_connection_timed_out_event_type,
	torque_connection_disconnected_event_type,
	torque_connection_established_event_type,
	torque_connection_packet_event_type,
	torque_connection_packet_notify_event_type,
	torque_socket_packet_event_type,
};

enum bind_result
{
	bind_success,
	address_in_use, ///< A process on this computer is already bound to this address.
	address_invalid, ///< The specified address is not valid for this computer
	initialization_failure,
	socket_allocation_failure,
	generic_failure,
};

struct torque_socket_event
{
	unsigned event_type;
	torque_connection_id connection;
	torque_connection_id arranger_connection;
	unsigned client_identity;
	unsigned connection_token;
	unsigned key_size;
	unsigned char *key;
	unsigned data_size;
	unsigned char *data;
	unsigned packet_sequence;
	int delivered;
	struct sockaddr source_address;
};

struct torque_socket_interface
{
	torque_socket_handle (*create)(bool background_thread, void (*socket_notify)(void *), void *socket_notify_data); ///< Creates an unbound torque socket.  If background_thread is true, the socket will be created with a background socket process thread.  Periodically socket_notify will be called _from_the_background_thread_ to signal that processing is necessary.
	
	void (*destroy)(torque_socket_handle); ///< Close the specified socket; any open connections on this socket will be closed automatically.
	
	bind_result (*bind)(torque_socket_handle the_socket, struct sockaddr *bound_interface_address); ///< Binds the socket to the specified socket address interface.
	
	void (*allow_incoming_connections)(torque_socket_handle, int allowed); ///< Sets whether or not this connection accepts incoming connections; if not, all incoming connection challenges and requests will be silently ignored.  FUTURE: add a list of domain referrers that are allowed to refer a client to this domain.
	
	void (*set_key_pair)(torque_socket_handle, unsigned key_data_size, unsigned char *the_key); ///< Sets the private/public key pair to be used for this connection;  In the prototype implementation these are formatted as libtomcrypt keys, and currently only ECC key format is supported.
	
	void (*set_challenge_response)(torque_socket_handle, unsigned challenge_response_size, unsigned char *challenge_response); ///< Sets the data to be sent back upon challenge request along with the client puzzle and public key.  challenge_response_data_size must be <= torque_max_status_datagram_size
	
	void (*write_entropy)(torque_socket_handle, unsigned char entropy[32]); ///< Seed random entropy data for this socket (used in the generation of cryptographic keys).
	
	void (*read_entropy)(torque_socket_handle, unsigned char entropy[32]); ///< Read some random data from the socket
	
	int (*send_to)(torque_socket_handle, struct sockaddr* remote_host, unsigned data_size, unsigned char *data); ///< sends an unconnected datagram to the remote_host from the specified socket.  This function is not available for security reasons in the plugin version of the API.
	
	torque_connection_id (*connect)(torque_socket_handle, struct sockaddr* remote_host, unsigned connect_data_size, unsigned char *connect_data); ///< open a connection to the remote host
	
	torque_connection_id (*connect_introduced)(torque_socket_handle socket, torque_connection_id introducer, torque_connection_id remote_client_identity, int is_host, unsigned connect_data_size, unsigned char *connect_data); ///< Connect to a client connected to the host at middle_man.
	
	void (*introduce)(torque_socket_handle, torque_connection_id initiator, torque_connection_id host); ///< This is called on the middleman of an introduced connection and will allow this host to broker a connection start between the remote hosts at either connection point.
	
	void (*accept_challenge)(torque_socket_handle, torque_connection_id pending_connection); ///< Accept the challenge response from the host.
	
	void (*accept_connection)(torque_socket_handle, torque_connection_id); ///< accept an incoming connection request.
	
	void (*close_connection)(torque_socket_handle, torque_connection_id, unsigned disconnect_data_size, unsigned char *disconnect_data); ///< Close an open connection or rejects a pending connection
	
	int (*send_to_connection)(torque_socket_handle, torque_connection_id, unsigned datagram_size, unsigned char buffer[torque_sockets_max_datagram_size]); ///< Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.
	struct torque_socket_event *(*get_next_event)(torque_socket_handle); ///< Gets the next event on this socket; returns NULL if there are no events to be read.
};
