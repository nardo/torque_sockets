#include "core/platform.h"
#include "torque_sockets/torque_sockets_c_api.h"

int main(int argc, const char **argv)
{
	const char *address_string = "127.0.0.1";
	const char *port_string = "28000";
	
	int port, host;
	host = htonl(inet_addr(address_string));
	port = atoi(port_string);
	
	SOCKADDR_IN address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(host);
	
	torque_socket_handle h;
	h = torque_socket_create(&address);
}