#include "net_api.h"

#include "tomcrypt.h"
#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

#include <queue>

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
	if (!iface || !*iface)
		return;
	
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
	
	iface->i->check_incoming_packets();
	iface->i->process_connections();
	
	bool recv = iface->i->tnp_pop_event(*the_event);

	if (!recv)
		return 0;

	fprintf(stderr, "Got an event\n");
	
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
	byte_buffer_ptr data = new byte_buffer(connect_data, connect_data_size);
	ret->c->connect(iface->i, remote_host, data);
	
	return ret;
}

tnp_connection tnp_accept_connection(tnp_interface iface, tnp_event* connect_request_event,
					unsigned int connect_accept_data_size,
					unsigned char connect_accept_data[tnp_max_status_datagram_size])
{
	assert(iface);
	assert(connect_request_event);
	
	tnp_connection ret = new _tnp_opaque_connection_t;
	
	byte_buffer_ptr data = new byte_buffer(connect_accept_data, connect_accept_data_size);
	
	ret->c = iface->i->tnp_accept_connection(connect_request_event->source_address, data);

	return ret;
}

void tnp_reject_connection(tnp_interface iface, tnp_event* connect_request_event,
						   unsigned int reject_data_size,
						   unsigned char reject_data[tnp_max_status_datagram_size])
{
	assert(iface)
	assert(connect_request_event);
	
	byte_buffer_ptr data = new byte_buffer(reject_data, reject_data_size);
	iface->i->tnp_reject_connection(connect_request_event->source_address, data);
}

void tnp_disconnect(tnp_connection* conn, unsigned int disconnect_data_size, 
					unsigned char disconnect_data[tnp_max_status_datagram_size])
{
	if (!conn || !*conn)
		return;
	
	byte_buffer_ptr data = new byte_buffer(disconnect_data, disconnect_data_size);
	
	(*conn)->c->disconnect(data);
	
	delete *conn;
	*conn = NULL;
}

void tnp_send_to(tnp_connection conn, unsigned int datagram_size, 
				unsigned char buffer[tnp_max_datagram_size])
{
	assert(conn);
	
	byte_buffer_ptr data = new byte_buffer(buffer, datagram_size);
	conn->c->tnp_send_data_packet(data);
}

void tnp_get_connection_state(tnp_connection the_connection)
{
}
