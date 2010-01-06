// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

// torque sockets, connected unreliable datagram and notification protocol

// basic C API

#ifndef _TORQUE_SOCKETS_H_
#define _TORQUE_SOCKETS_H_

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#if defined(__MSVC__)
	typedef signed _int64 int64; ///< Signed 64 bit integer
	typedef unsigned _int64 uint64; ///< Unsigned 64 bit integer
#else
	typedef signed long long int64; ///< Signed 64 bit integer
	typedef unsigned long long uint64; ///< Unsigned 64 bit integer
#endif

#ifdef __cplusplus
extern "C" {
#endif
	
/* Opaque forward declares */

typedef int torque_socket;
typedef int torque_connection;
static const torque_connection invalid_torque_connection = 0;
struct torque_socket_event;
	
enum connection_constants {
	torque_max_datagram_size = 1480,
	torque_max_status_datagram_size = 512,
	torque_max_public_key_size = 512,
};

typedef unsigned char status_response[torque_max_status_datagram_size];

torque_socket torque_socket_create(struct sockaddr*);
///< Create a torque socket and bind it to the specified socket address interface.

void torque_socket_destroy(torque_socket);
///< Close the specified socket; any open connections on this socket will be closed automatically.

void torque_socket_seed_entropy(torque_socket, unsigned char entropy[32]);
///< Seed random entropy data for this socket (used in the generation of cryptographic keys).

void torque_socket_read_entropy(torque_socket, unsigned char entropy[32]);
///< Read the current entropy state from this socket.

void torque_socket_set_private_key(torque_socket, unsigned key_data_size, unsigned char *the_key);
///< Sets the private/public key pair to be used for this connection; these are formatted as libtomcrypt keys, and currently only ECC is supported.

void torque_socket_set_challenge_response_data(torque_socket, unsigned challenge_response_data_size, status_response challenge_response_data);
///< Sets the data to be sent back upon challenge request along with the client puzzle and public key.  challenge_response_data_size must be <= torque_max_status_datagram_size
	
void torque_socket_allow_incoming_connections(torque_socket, int allowed);
///< Sets whether or not this connection accepts incoming connections; if not, all incoming connection challenges and requests will be silently ignored.

int torque_socket_does_allow_incoming_connections(torque_socket);
///< Returns true if this socket allows incoming connections.

void torque_socket_allow_arranged_connections(torque_socket, int allowed);
///< if true, this socket will arrange connections between hosts connected to this socket.

int torque_socket_does_allow_arranged_connections(torque_socket);
///< Returns true if this socket will arrange connections between hosts connected to this socket.	

struct torque_socket_event *torque_socket_get_next_event(torque_socket);
///< Gets the next event on this socket; returns NULL if there are no events to be read.

torque_connection torque_socket_connect(torque_socket, struct sockaddr* remote_host, unsigned connect_data_size, status_response connect_data);
///< open a connection to the remote host

torque_connection torque_connection_connect_arranged(torque_connection middle_man, uint64 remote_client_identity, unsigned connect_data_size, status_response connect_data);
///< Connect to a client connected to the host at middle_man.

void torque_connection_accept(torque_connection, unsigned connect_accept_data_size, status_response connect_accept_data);
///< accept an incoming connection request.

void torque_connection_reject(torque_connection, unsigned reject_data_size, status_response reject_data);
///< reject an incoming connection request.

void torque_connection_disconnect(torque_connection, unsigned disconnect_data_size, status_response disconnect_data);
///< Close an open connection. 

int torque_connection_send_to(torque_connection, unsigned datagram_size, unsigned char buffer[torque_max_datagram_size], unsigned *sequence_number);
///< Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.

enum torque_socket_event_type
{
	torque_connection_challenge_response_event_type,
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
};
	
struct torque_connection_challenge_response_event
{
	unsigned event_type;
	torque_connection connection;
	unsigned public_key_size;
	unsigned char public_key[torque_max_public_key_size];
	unsigned challenge_response_data_size;
	status_response challenge_response_data;
};
	
struct torque_connection_accepted_event
{
	unsigned event_type;
	torque_connection connection;
	uint64 client_identity;
	unsigned connection_accept_data_size;
	status_response connection_accept_data;
};
	
struct torque_connection_rejected_event
{
	unsigned event_type;
	torque_connection connection;
	unsigned connection_reject_data_size;
	status_response connection_reject_data;
};

struct torque_connection_requested_event
{
	unsigned event_type;
	torque_connection connection;
	unsigned public_key_size;
	unsigned char public_key[torque_max_public_key_size];
	unsigned connection_request_data_size;
	status_response connection_request_data;
	struct sockaddr source_address;
};

struct torque_connection_arranged_connection_request_event
{
	unsigned event_type;
	uint64 source_client_identity;
	torque_connection connection;
	torque_connection arranger_connection;
	unsigned connection_request_data_size;
	status_response connection_request_data;
};	
	
struct torque_connection_timed_out_event
{
	unsigned event_type;
	torque_connection connection;
};

struct torque_connection_disconnected_event
{
	unsigned event_type;
	torque_connection connection;
	unsigned connection_disconnected_data_size;
	status_response connection_disconnected_data;
};

struct torque_connection_established_event
{
	unsigned event_type;
	torque_connection connection;
};

struct torque_connection_packet_event
{
	unsigned event_type;
	torque_connection connection;
	unsigned packet_sequence;
	unsigned data_size;
	unsigned char data[torque_max_datagram_size];
};

struct torque_connection_packet_notify_event
{
	unsigned event_type;
	torque_connection connection;
	unsigned send_sequence;
	int delivered;
};

struct torque_socket_packet_event
{
	unsigned event_type;
	unsigned data_size;
	unsigned char data[torque_max_datagram_size];
	struct sockaddr source_address;
};

#ifdef __cplusplus
}
#endif
#endif
