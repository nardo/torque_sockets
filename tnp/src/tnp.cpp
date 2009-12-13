#include "net_api.h"

#include "tomcrypt.h"
#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

using namespace core;
struct net {
	#include "net/net.h"
};

struct _tnp_opaque_connection_t
{
	ref_ptr<net::connection> c;
};

struct _tnp_opaque_interface_t
{
	ref_ptr<net::interface> i;
};

tnp_interface tnp_create_interface(const tnp_address addr)
{
	fprintf(stderr, "Creatinging interface bound to %s\n", addr);
	ltc_mp = ltm_desc;

	assert(addr);
	assert(addr[0]);
	
	_tnp_opaque_interface_t* ret = new _tnp_opaque_interface_t;
	if (!ret)
		return 0;
	
	ret->i = new net::interface(addr);
	return ret;
}

void tnp_destroy_interface(tnp_interface* iface)
{
	fprintf(stderr, "Destroying interface, maybe?\n");
	if (!iface || !*iface)
		return;
	
	fprintf(stderr, "Destroying interface\n");
	delete *iface;
	*iface = 0;
}

void tnp_seed_entropy(tnp_interface, unsigned char entropy[32])
{
}

void tnp_read_entropy(tnp_interface, unsigned char entropy[32])
{
}

void tnp_allow_incoming_connections(tnp_interface iface, int allowed)
{
	assert(iface)
	
	iface->i->set_allows_connections(allowed);
}

int tnp_does_allow_incoming_connections(tnp_interface iface)
{
	assert(iface);
	
	return iface->i->does_allow_connections();
}

int tnp_get_next_event(tnp_interface iface, tnp_event* the_event)
{
	assert(iface)
	assert(the_event);
		
	uint8 packet_type;
	net::address source_address;
	byte_buffer_ptr data;
	
	bool recv = iface->i->tnp_get_next_packet(source_address, packet_type, data);

	if (!recv)
		return 0;

	fprintf(stderr, "Got an event\n");
	the_event->event_type = (tnp_event::tnp_event_types)packet_type;
	the_event->data_size = data->get_buffer_size();
	memcpy(the_event->data, data->get_buffer(), the_event->data_size);
	
	core::string addr_s = source_address.to_string();
	strncpy(the_event->source_address, addr_s.c_str(), MAX_ADDR);
	
	return 1;
}

tnp_connection tnp_connect(tnp_interface iface, const tnp_address remote_host, unsigned int connect_data_size,
					   unsigned char connect_data[tnp_max_status_datagram_size])
{
	assert(iface);
	assert(remote_host);
	assert(remote_host[0]);
	
	tnp_connection ret = new _tnp_opaque_connection_t;
	ret->c = new net::connection(iface->i->random());
	
	ret->c->set_address(remote_host);
	ret->c->set_interface(iface->i);
	ret->c->_connect_send_count = 0;
	ret->c->set_connection_state(net::connection::awaiting_connect_response);
	
	byte_buffer_ptr data = new byte_buffer(connect_data, connect_data_size);
	
	iface->i->tnp_send_packet(ret->c, tnp_event::tnp_connection_requested_event, data);
	
	return ret;
}

tnp_connection tnp_accept_connection(tnp_interface iface, tnp_event* connect_request_event,
					unsigned int connect_accept_data_size,
					unsigned char connect_accept_data[tnp_max_status_datagram_size])
{
	assert(iface);
	assert(connect_request_event);
	
	tnp_connection ret = new _tnp_opaque_connection_t;
	ret->c = new net::connection(iface->i->random());
	
	ret->c->set_address(connect_request_event->source_address);
	ret->c->set_interface(iface->i);
	
	byte_buffer_ptr data = new byte_buffer(connect_accept_data, connect_accept_data_size);
	
	iface->i->tnp_send_packet(ret->c, tnp_event::tnp_connection_accepted_event, data);

	return ret;
}

void tnp_reject_connection(tnp_interface, tnp_event* connect_request_event,
						   unsigned int reject_data_size,
						   unsigned char reject_data[tnp_max_status_datagram_size])
{
}

void tnp_disconnect(tnp_connection* conn, unsigned int disconnect_data_size, 
					unsigned char disconnect_data[tnp_max_status_datagram_size])
{
	if (!conn || !*conn)
		return;
	
	byte_buffer_ptr data = new byte_buffer(disconnect_data, disconnect_data_size);
	
	(*conn)->c->get_interface()->tnp_send_packet((*conn)->c, tnp_event::tnp_connection_disconnected_event, data);
	
	delete *conn;
	*conn = NULL;
}

void tnp_send_to(tnp_connection conn, unsigned int datagram_size, 
				unsigned char buffer[tnp_max_datagram_size])
{
	byte_buffer_ptr data = new byte_buffer(buffer, datagram_size);
	conn->c->get_interface()->tnp_send_packet(conn->c, tnp_event::tnp_interface_packet_event + 2, data);
}

void tnp_get_connection_state(tnp_connection the_connection)
{
}
