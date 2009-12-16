/* Event struct, for use in pure C modules */

#ifndef TNP_EVENT_H_
#define TNP_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
	tnp_max_datagram_size = 1536/*udp_socket::max_datagram_size*/ -
							4/*connection::packet_header_byte_size*/ -
							5/*connection::message_signature_bytes*/ - 10,
	tnp_max_status_datagram_size = 512
};

#ifndef MAX_ADDR
#define MAX_ADDR 128
#endif

typedef char tnp_address[MAX_ADDR];

typedef struct tnp_event
{
	enum tnp_event_types
	{
		tnp_connection_accepted_event,
		tnp_connection_rejected_event,
		tnp_connection_requested_event,
		tnp_connection_timed_out_event,
		tnp_connection_disconnected_event,
		tnp_connection_established_event,
		tnp_connection_packet_event,
		tnp_connection_packet_notify_event,
		tnp_interface_packet_event,
	};

	/* One of tnp_event_types or a custom defined type */
	int event_type;
	tnp_address source_address;
	unsigned int packet_sequence;
	unsigned int data_size;
	unsigned char data[tnp_max_datagram_size];
} tnp_event;

#ifdef __cplusplus
}
#endif

#endif
