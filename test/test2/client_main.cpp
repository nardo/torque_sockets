// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "torque_sockets.h"

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

torque_socket my_socket;
torque_connection my_connection;

bool handle_event(torque_socket_event* event)
{
	if (event->event_type == torque_connection_challenge_response_event_type)
	{
		printf("Challenge response: %s\n", event->data);
		return false;
	}
	
	if (event->event_type == torque_connection_accepted_event_type)
	{
//		printf("Connection accepted from %s\n", evt.source_address);
		printf("Connection message: %s\n", event->data);
		return true;
	}
	
	printf("Unknown event type\n");
	return false;
}

int main(int argc, const char **argv)
{
	sockaddr_in ip4addr;
	memset(&ip4addr, 0, sizeof(ip4addr));
	
	ip4addr.sin_family = AF_INET;
	ip4addr.sin_port = htons(31338);
	inet_pton(AF_INET, "127.0.0.1", &ip4addr.sin_addr);
	my_socket = torque_socket_create((sockaddr*)&ip4addr);
	torque_socket_allow_incoming_connections(my_socket, false);
	
	const char* msg = "Hello Mr. Server";
	
	sockaddr_in server_addy;
	memset(&server_addy, 0, sizeof(server_addy));
	
	server_addy.sin_family = AF_INET;
	server_addy.sin_port = htons(31337);
	inet_pton(AF_INET, "127.0.0.1", &server_addy.sin_addr);
	
	my_connection = torque_socket_connect(my_socket, (sockaddr*)&server_addy, strlen(msg) + 1, (unsigned char*)msg);

	bool spam = false;
	while(true)
	{
		torque_socket_event* evt = torque_socket_get_next_event(my_socket);
		if(evt)
			if(handle_event(evt))
				spam = true;
		if(spam)
		{
			const char* msg = "It works!";
			torque_connection_send_to(my_connection, strlen(msg) + 1, (unsigned char*)msg, NULL);
		}
		sleep(1);
	}

	const char* dmsg = "Goodbye Mr. Bond";
	torque_connection_disconnect(my_connection, strlen(dmsg) + 1, (unsigned char*)dmsg);
	torque_socket_destroy(my_socket);

	return 0;
}
