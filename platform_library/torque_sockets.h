// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

// torque sockets, connected unreliable datagram and notification protocol

// basic C API

#ifndef _TORQUE_SOCKETS_H_
#define _TORQUE_SOCKETS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque forward declares */

struct _torque_opaque_socket_t;
typedef struct _torque_opaque_socket_t* torque_socket;
typedef uint32 torque_connection;
struct torque_socket_event;
	
enum connection_constants {
	torque_max_datagram_size = 1516,
	torque_max_status_datagram_size = 512,	
};
	
torque_socket torque_socket_create(const SOCKADDR *); ///< Create a torque socket and bind it to the specified socket address interface.
void torque_socket_destroy(torque_socket); ///< Close the specified socket; any open connections on this socket will be closed automatically.
void torque_socket_seed_entropy(torque_socket, uint8 entropy[32]); ///< Seed random entropy data for this socket (used in the generation of cryptographic keys).
void torque_socket_read_entropy(torque_socket, uint8 entropy[32]); ///< Read the current entropy state from this socket.
void torque_socket_set_private_key(torque_socket, uint32 key_data_size, uint8 *the_key); ///< Sets the private/public key pair to be used for this connection; these are formatted as libtomcrypt keys, and currently only ECC is supported.
void torque_socket_set_challenge_response_data(torque_socket, uint32 challenge_response_data_size, uint8 challenge_response_data[torque_max__status_datagram_size]); ///< Sets the data to be sent back upon challenge request along with the client puzzle and public key.  challenge_response_data_size must be <= torque_max_status_datagram_size
void torque_socket_allow_incoming_connections(torque_socket, bool allowed); ///< Sets whether or not this connection accepts incoming connections; if not, all incoming connection challenges and requests will be silently ignored.
bool torque_socket_does_allow_incoming_connections(torque_socket); ///< Returns true if this socket allows incoming connections.
torque_socket_event *torque_socket_get_next_event(torque_socket); ///< Gets the next event on this socket; returns NULL if there are no events to be read.

torque_connection torque_socket_connect(torque_socket, const SOCKADDR *remote_host, uint32 connect_data_size, uint8 connect_data[torque_max__status_datagram_size]); ///< open a connection to the remote host

void torque_connection_accept(torque_connection, uint32 connect_accept_data_size, uint8 connect_accept_data[torque_max__status_datagram_size]); ///< accept an incoming connection request.

void torque_connection_reject(torque_connection, uint32 reject_data_size, uint8 reject_data[torque_max__status_datagram_size]); ///< reject an incoming connection request.

void torque_connection_disconnect(torque_connection, uint32 disconnect_data_size, uint8 disconnect_data[torque_max__status_datagram_size]); ///< Close an open connection. 
send_to_result torque_connection_send_to(torque_connection, uint32 datagram_size, uint8 buffer[torque_max_datagram_size], uint32 *sequence_number); ///< Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.

struct torque_socket_event
{
	enum torque_socket_event_type
	{
		torque_connection_challenge_response_event_type,
		torque_connection_requested_event_type,
		torque_connection_accepted_event_type,
		torque_connection_rejected_event_type,
		torque_connection_timed_out_event_type,
		torque_connection_disconnected_event_type,
		torque_connection_established_event_type,
		torque_connection_packet_event_type,
		torque_connection_packet_notify_event_type,
		torque_socket_packet_event_type,
	};
	uint32 event_type;
};
	
struct torque_connection_accepted_event : torque_socket_event
{
	torque_connection the_connection;
	uint32 connection_accept_data_size;
	uint8 connection_accept_data[torque_max_status_datagram_size];
};
	
struct torque_connection_rejected_event : torque_socket_event
{
	torque_connection the_connection;
	uint32 connection_reject_data_size;
	uint8 connection_reject_data[torque_max_status_datagram_size];
};

struct torque_connection_requested_event : torque_socket_event
{
	SOCKADDR source_address;
	torque_connection the_connection;
	uint32 connection_request_data_size;
	uint8 connection_request_data[torque_max_status_datagram_size];
};

struct torque_connection_timed_out_event : torque_socket_event
{
	torque_connection the_connection;
};

struct torque_connection_disconnected_event : torque_socket_event
{
	torque_connection the_connection;
	uint32 connection_disconnected_data_size;
	uint8 connection_disconnected_data[torque_max_status_datagram_size];
};

struct torque_connection_established_event : torque_socket_event
{
	torque_connection the_connection;
};

struct torque_connection_packet_event : torque_socket_event
{
	torque_connection the_connection;
	uint32 packet_sequence;
	uint32 data_size;
	uint32 data[torque_max_datagram_size];
};

struct torque_connection_packet_notify_event : torque_socket_event
{
	torque_connection the_connection;
	uint32 send_sequence;
	bool delivered;
};

struct torque_socket_packet_event : torque_socket_event
{
	SOCKADDR source_address;
	uint32 data_size;
	uint32 data[torque_max_datagram_size];
};

#ifdef __cplusplus
}
#endif

