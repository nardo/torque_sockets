/* Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms. */

/* This file holds the C API description for the network platform */

#ifndef TNP_NET_API_H_
#define TNP_NET_API_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque forward declares */
struct _tnp_opaque_connection_t;
struct _tnp_opaque_interface_t;

/* C API: */
enum {
	tnp_max_datagram_size = 1536/*udp_socket::max_datagram_size*/ -
							4/*connection::packet_header_byte_size*/ -
							5/*connection::message_signature_bytes*/ - 10,
	tnp_max_status_datagram_size = 512
};

typedef struct _tnp_opaque_connection_t* tnp_connection;
typedef struct _tnp_opaque_interface_t* tnp_interface;

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
	} event_type;

	tnp_address source_address;
	unsigned int packet_sequence;
	unsigned int data_size;
	unsigned char data[tnp_max_datagram_size];
} tnp_event;

typedef struct tnp_connection_state
{
	/* round trip time */
	/* packet loss */
	/* throughput */
	/* connection state */
} tnp_connection_state;

tnp_interface tnp_create_interface(const tnp_address);
void tnp_destroy_interface(tnp_interface*);
void tnp_seed_entropy(tnp_interface, unsigned char entropy[32]);
void tnp_read_entropy(tnp_interface, unsigned char entropy[32]);
void tnp_allow_incoming_connections(tnp_interface, int allowed);
int tnp_does_allow_incoming_connections(tnp_interface);
int tnp_get_next_event(tnp_interface, tnp_event* the_event);

tnp_connection tnp_get_connection(tnp_interface, tnp_address);
void tnp_release_connection(tnp_connection);

tnp_connection tnp_connect(tnp_interface, const tnp_address remote_host, unsigned int connect_data_size,
					   unsigned char connect_data[tnp_max_status_datagram_size]);

tnp_connection tnp_accept_connection(tnp_interface, tnp_event* connect_request_event,
					unsigned int connect_accept_data_size,
					unsigned char connect_accept_data[tnp_max_status_datagram_size]);

void tnp_reject_connection(tnp_interface, tnp_event* connect_request_event,
						   unsigned int reject_data_size,
						   unsigned char reject_data[tnp_max_status_datagram_size]);

void tnp_disconnect(tnp_connection*, unsigned int disconnect_data_size, 
					unsigned char disconnect_data[tnp_max_status_datagram_size]);

void tnp_send_to(tnp_connection, unsigned int datagram_size, 
				unsigned char buffer[tnp_max_datagram_size]);
void tnp_get_connection_state(tnp_connection the_connection);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <string>
#include <vector>

namespace tnp
{
	enum EventTypes
	{
		ConnectionAcceptedEvent,
		ConnectioRrejectedEvent,
		ConnectionRequestedEvent,
		ConnectionTimedOutEvent,
		ConnectionDisconnectedEvent,
		ConnectionEstablishedEvent,
		ConnectionPacketEvent,
		ConnectionPacketNotifyEvent,
		InterfacePacketEvent,
		UserBarrier
	};

	class connection
	{
	public:
		connection(tnp_connection c_) : c(c_)
		{
		}
		
		connection()
		{
		}
		
		void send(std::string data)
		{
			tnp_send_to(c, data.length() + 1, (unsigned char*)data.c_str());
		}
		
	private:
		tnp_connection c;
	};
	
	class event
	{
	public:
		event() { }
		tnp_event e;
		bool recv_;
		
		int event_type() const
		{
			return e.event_type;
		}
		
		std::string source_address() const
		{
			return e.source_address;
		}
		
		std::string data() const
		{
			return std::string((const char*)e.data);
		}
		
		bool recv() const
		{
			return recv_;
		}
	};

	class interface
	{
	public:
		interface(const std::string& addy)
		{
			i = tnp_create_interface(addy.c_str());
		}
		
		~interface()
		{
			tnp_destroy_interface(&i);
		}
		
		void set_allows_connections(bool b)
		{
			tnp_allow_incoming_connections(i, b);
		}
		
		bool allows_connections()
		{
			return tnp_does_allow_incoming_connections(i);
		}
		
		connection connect(std::string remote_host, std::string data)
		{
			connection ret(tnp_connect(i, remote_host.c_str(), data.length() + 1, (unsigned char*)data.c_str()));
			return ret;
		}
		
		event get_next_event()
		{
			event ret;
			ret.recv_ = tnp_get_next_event(i, &ret.e);
			return ret;
		}
		
		connection accept_connection(event e, std::string data)
		{
			connection ret(tnp_accept_connection(i, &e.e, data.length() + 1, (unsigned char*)data.c_str()));
			return ret;
		}
	
	private:
		tnp_interface i;
	};
}

#endif

#endif
