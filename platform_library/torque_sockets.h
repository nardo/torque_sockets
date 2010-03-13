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

#ifdef __cplusplus
extern "C" {
#endif

typedef int torque_socket;
typedef int torque_connection;
static const torque_socket invalid_torque_socket = 0;
static const torque_connection invalid_torque_connection = 0;
struct torque_socket_event;
	
enum connection_constants {
	torque_max_datagram_size = 1480,
	torque_max_status_datagram_size = 512,
	torque_max_public_key_size = 512,
	torque_packet_window_size = 31,
};

typedef unsigned char status_response[torque_max_status_datagram_size];

#ifndef TORQUE_SOCKETS_STANDALONE
void init_server(const char* server_address);
void init_client(const char* client_address, const char* server_address);
void close_client();
#endif

torque_socket torque_socket_create(struct sockaddr*);
///< Create a torque socket and bind it to the specified socket address interface.

void torque_socket_destroy(torque_socket);
///< Close the specified socket; any open connections on this socket will be closed automatically.

void torque_socket_seed_entropy(torque_socket, unsigned char entropy[32]);
///< Seed random entropy data for this socket (used in the generation of cryptographic keys).

void torque_socket_set_private_key(torque_socket, unsigned key_data_size, unsigned char *the_key);
///< Sets the private/public key pair to be used for this connection; these are formatted as libtomcrypt keys, and currently only ECC is supported.

void torque_socket_set_challenge_response_data(torque_socket, unsigned challenge_response_data_size, unsigned char *challenge_response_data);
///< Sets the data to be sent back upon challenge request along with the client puzzle and public key.  challenge_response_data_size must be <= torque_max_status_datagram_size
	
void torque_socket_allow_incoming_connections(torque_socket, int allowed, ?from_domains?);
///< Sets whether or not this connection accepts incoming connections; if not, all incoming connection challenges and requests will be silently ignored.
	
int torque_socket_does_allow_incoming_connections(torque_socket);
///< Returns true if this socket allows incoming connections.

void torque_socket_introduce(torque_socket, torque_connection, torque_connection, unsigned connection_token);
///< This is called on the middleman of an introduced connection and will allow this host to broker a connection start between the remote hosts at either connection point.
	
torque_connection torque_connection_connect_introduced(torque_connection introducer, unsigned remote_client_identity, unsigned connection_token, unsigned connect_data_size, unsigned char *connect_data);
///< Connect to a client connected to the host at middle_man.

struct torque_socket_event *torque_socket_get_next_event(torque_socket);
///< Gets the next event on this socket; returns NULL if there are no events to be read.

int torque_socket_send_to(torque_socket, struct sockaddr* remote_host, unsigned data_size, unsigned char *data);
///< sends an unconnected datagram to the remote_host from the specified socket.  This function is not available for security reasons in the plugin version of the API.
	
torque_connection torque_socket_connect(torque_socket, struct sockaddr* remote_host, unsigned connect_data_size, unsigned char *connect_data);
///< open a connection to the remote host

void torque_connection_accept(torque_connection, unsigned connect_accept_data_size, unsigned char *connect_accept_data);
///< accept an incoming connection request.

void torque_connection_reject(torque_connection, unsigned reject_data_size, unsigned char *reject_data);
///< reject an incoming connection request.

void torque_connection_disconnect(torque_connection, unsigned disconnect_data_size, unsigned char *disconnect_data);
///< Close an open connection. 

int torque_connection_send_to(torque_connection, unsigned datagram_size, unsigned char buffer[torque_max_datagram_size], unsigned *sequence_number);
///< Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.

enum torque_socket_rpc_functions
{
	torque_socket_create_rpc,
	torque_socket_destroy_rpc,
	torque_socket_seed_entropy_rpc,
	torque_socket_set_private_key_rpc,
	torque_socket_set_challenge_response_data_rpc,
	torque_socket_allow_incoming_connections_rpc,
	torque_socket_does_allow_incoming_connections_rpc,
	torque_socket_can_be_introducer_rpc,
	torque_socket_is_introducer_rpc,
	torque_socket_get_next_event_rpc,
	torque_socket_connect_rpc,
	torque_connection_introduce_me_rpc,
	torque_connection_accept_rpc,
	torque_connection_reject_rpc,
	torque_connection_disconnect_rpc,
	torque_connection_send_to_rpc,
	torque_max_rpc
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
	torque_connection connection;
	torque_connection arranger_connection;
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

#ifdef __cplusplus
}
#endif
#endif
