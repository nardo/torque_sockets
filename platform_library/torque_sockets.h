// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

// TorqueSockets, connected unreliable datagram and notification protocol

// basic C API

#ifndef _TORQUE_SOCKETS_H_
#define _TORQUE_SOCKETS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque forward declares */

struct _torque_opaque_connection_t;
struct _torque_opaque_socket_t;

enum connection_constants {
	torque_max_datagram_size = 1516,
	torque_max_status_datagram_size = 512,	
};


typedef struct _torque_opaque_connection_t* torque_connection;
typedef struct _torque_opaque_socket_t* torque_socket;
	
struct torque_socket_event
{
	enum torque_socket_event_type
	{
		torque_connection_accepted_event,
		torque_connection_rejected_event,
		torque_connection_requested_event,
		torque_connection_timed_out_event,
		torque_connection_disconnected_event,
		torque_connection_established_event,
		torque_connection_packet_event,
		torque_connection_packet_notify_event,
		torque_socket_packet_event,
	} event_type;
	
	SOCKADDR source_address; ///< For data packets and connection status events, this holds the source address for the received packet.
	torque_connection *source_connection; ///< Connection object for this event
	uint32 packet_sequence; ///< Sequence number for this packet received or delivery notification.
	bool packet_delivered; ///< For a packet notify event, delivery status flag.
	uint32 data_size; ///< Size of the associated datagram or status message.
	uint8 data[torque_max_datagram_size]; ///< Datagram or status message associated with this event.
};
	
torque_socket torque_socket_create(const SOCKADDR *); ///< Create a torque socket and bind it to the specified socket address interface.
void torque_socket_destroy(torque_socket); ///< Close the specified socket; any open connections on this socket will be closed automatically.
void torque_socket_seed_entropy(torque_socket, uint8 entropy[32]); ///< Seed random entropy data for this socket (used in the generation of cryptographic keys).
void torque_socket_read_entropy(torque_socket, uint8 entropy[32]); ///< Read the current entropy state from this socket.
void torque_socket_allow_incoming_connections(torque_socket, bool allowed); ///< Sets whether or not this connection accepts incoming connections; if not, all incoming connection challenges and requests will be silently ignored.
bool torque_socket_does_allow_incoming_connections(torque_socket); ///< Returns true if this socket allows incoming connections.
bool torque_socket_get_next_event(torque_socket, torque_socket_event* the_event); ///< Gets the next event on this socket; returns true if there was an event pending or false if there are no events to be read.

torque_connection torque_socket_connect(torque_socket, const SOCKADDR *remote_host, uint32 connect_data_size, uint8 connect_data[tnp_max_status_datagram_size]); ///< open a connection to the remote host

void torque_connection_accept(torque_connection, uint32 connect_accept_data_size, uint8 connect_accept_data[tnp_max_status_datagram_size]); ///< accept an incoming connection request.

void torque_connection_reject(torque_connection, uint32 reject_data_size, uint8 reject_data[tnp_max_status_datagram_size]); ///< reject an incoming connection request

void torque_connection_disconnect(torque_connection, uint32 disconnect_data_size, uint8 disconnect_data[tnp_max_status_datagram_size]); ///< Close an open connection.

uint32 torque_connection_send_to(torque_connection, uint32 datagram_size, uint8 buffer[tnp_max_datagram_size]); ///< Send a datagram packet to the remote host on the other side of the connection.  Returns the sequence number of the packet sent.
	
void torque_connection_get_state(torque_connection the_connection, torque_connection_state *the_state);
	
#ifdef __cplusplus
}
#endif

