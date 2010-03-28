#include "core/platform.h"
#include "torque_sockets/torque_sockets_c_api.h"

enum world_type
{
	introducer,
	host,
	initiator,
	introduced_initiator,
	introduced_host,
	world_count,
};

enum state
{
	waiting,
	connecting_to_remote,
	connecting_to_introducer,
};

typedef struct
{
	int state;
	torque_socket_handle _socket;	
	SOCKADDR_IN connect_address;
	torque_connection_id connection;
	torque_connection_id introduced_connection;
} world;

world g_worlds[world_count];
int world_ports[world_count] = {
 28000, 28001, 0, 0, 0
};

int get_current_time_ms()
{
	struct timeval t;
	gettimeofday(&t, NULL);
	int ms = (int) t.tv_sec  * 1000 + t.tv_usec / 1000;
	return ms;
}

void init_world(int index)
{
	const char *address_string = "127.0.0.1";	
	int port, host;
	host = htonl(inet_addr(address_string));
	port = world_ports[index];
	
	SOCKADDR_IN address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(host);
	
	g_worlds[index] torque_socket_create(&address);	
}

void process_world(int index)
{
	
}
	
int main(int argc, const char **argv)
{
	
	
}