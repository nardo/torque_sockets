#include "torque_sockets.h"

#include "tomcrypt.h"
#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

#include <queue>

#include "net/tnp_event.h"

using namespace core;
struct net {
	#include "net/net.h"
};

#include <vector>

static std::vector<ref_ptr<net::interface> > interfaces;
static std::vector<ref_ptr<net::connection> > connections;

torque_socket torque_socket_create(struct sockaddr* socket_address)
{
	ltc_mp = ltm_desc;
	assert(socket_address);
	
	if(!socket_address)
		return 0;

	ref_ptr<net::interface> i(new net::interface(*socket_address));
	interfaces.push_back(i);
	
	return interfaces.size();
}

void torque_socket_destroy(torque_socket socket)
{
	assert(socket > 0)
	if(socket <= 0)
		return;
	
	interfaces[socket - 1] = NULL;
}

torque_connection torque_socket_connect(torque_socket socket, struct sockaddr* remote_host, unsigned connect_data_size, status_response connect_data)
{
	assert(socket > 0);
	assert(interfaces[socket - 1] != NULL);
	
	if(socket <= 0 || interfaces[socket - 1] == NULL)
		return 0;
		
	assert(remote_host);
	
	if(!remote_host)
		return 0;
	
	ref_ptr<net::interface>& interface = interfaces[socket - 1];
	ref_ptr<net::connection> c(new net::connection(interface->random()));
	connections.push_back(c);
	byte_buffer_ptr data = new byte_buffer(connect_data, connect_data_size);
	c->connect(interface, *remote_host, data);
	
	return connections.size();
}

void torque_connection_disconnect(torque_connection conn, unsigned disconnect_data_size, status_response disconnect_data)
{
	assert(conn > 0);
	assert(connections[conn - 1] != NULL);
	
	if(conn <= 0 || connections[conn - 1] == NULL)
		return;
	
	ref_ptr<net::connection>& connection = connections[conn - 1];
	byte_buffer_ptr data = new byte_buffer(disconnect_data, disconnect_data_size);
	connection->disconnect(data);
	connection = NULL;
}

void torque_socket_allow_incoming_connections(torque_socket socket, int allow)
{
	assert(socket > 0);
	assert(interfaces[socket - 1] != NULL);
	
	if(socket <= 0 || interfaces[socket - 1] == NULL)
		return;
	
	interfaces[socket - 1]->set_allows_connections(allow);
}

int torque_socket_does_allow_incoming_connections(torque_socket socket)
{
	assert(socket > 0);
	assert(interfaces[socket - 1] != NULL);
	
	if(socket <= 0 || interfaces[socket - 1] == NULL)
		return 0;
	
	return interfaces[socket - 1]->does_allow_connections();
}
