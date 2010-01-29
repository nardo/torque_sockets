#include "torque_sockets.h"
#include "nacl_imc.h"

#include <stdio.h>
#include <string.h>

void PrintError(const char* message) {
	char buffer[256];

	if (nacl::GetLastErrorString(buffer, sizeof buffer) == 0) {
		fprintf(stderr, "%s: %s\n", message, buffer);
	}
}

struct
{
	nacl::Handle g_front;
	nacl::Handle pair[2];
} client_sockets;

void init_client(const char* client_addr, const char* server_addr)
{
	nacl::SocketAddress client;
	strncpy(client.path, client_addr, nacl::kPathMax);
	client_sockets.g_front = nacl::BoundSocket(&client);
	nacl::SocketPair(client_sockets.pair);

	nacl::SocketAddress server;
	strncpy(server.path, server_addr, nacl::kPathMax);

	nacl::MessageHeader header;
	header.iov = NULL;
	header.iov_length = 0;
	header.handles = &client_sockets.pair[1];
	header.handle_count = 1;
	nacl::SendDatagramTo(client_sockets.g_front, &header, 0, &server);
	
	nacl::Close(client_sockets.pair[1]);
}
