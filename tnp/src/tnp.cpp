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

tnp_interface tnp_create_interface(tnp_address addr)
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

int tnp_get_next_event(tnp_interface, tnp_event* the_event)
{
	return 0;
}

tnp_connection tnp_connect(tnp_interface iface, tnp_address remote_host, unsigned int connect_data_size,
					   unsigned char connect_data[tnp_max_status_datagram_size])
{
	assert(iface);
	assert(remote_host);
	assert(remote_host[0]);
	
	tnp_connection ret = new _tnp_opaque_connection_t;
	
	net::random_generator rnd = net::random_generator();
	ret->c = new net::connection(rnd);
	
	ret->c->connect(iface->i, remote_host); 
}

tnp_connection tnp_accept_connection(tnp_event* connect_request_event,
					unsigned int connect_accept_data_size,
					unsigned char connect_accept_data[tnp_max_status_datagram_size])
{
}

void tnp_reject_connection(tnp_event* connect_request_event,
						   unsigned int reject_data_size,
						   unsigned char reject_data[tnp_max_status_datagram_size])
{
}

void tnp_disconnect(tnp_connection* conn, unsigned int disconnect_data_size, 
					unsigned char disconnect_data[tnp_max_status_datagram_size])
{
	if (!conn || !*conn)
		return;
	
	byte_buffer_ptr b = new byte_buffer(disconnect_data, disconnect_data_size);
	(*conn)->c->disconnect(b);
	
	delete *conn;
	*conn = NULL;
}

void tnp_send_to(tnp_connection, unsigned int datagram_size, 
				unsigned char buffer[tnp_max_datagram_size])
{
}

void tnp_get_connection_state(tnp_connection the_connection)
{
}
