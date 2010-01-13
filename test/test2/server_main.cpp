// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "torque_sockets_cpp.h"

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

bool handle_event(const torque::event& event)
{
	if (event.event_type() == torque::event::connection_requested_event_type)
	{
//		printf("Connection requested from %s\n", evt.source_address);
		printf("Connection message: %s\n", event.data().c_str());
		torque::connection my_connection = event.source_connection();
		std::string msg = "Hello Mr. Client";
		my_connection.accept(msg);
		return false;
	}
	
	if(event.event_type() == torque::event::connection_packet_event_type)
	{
		printf("Connection data: %s\n", event.data().c_str());
		return false;
	}
	
	if (event.event_type() == torque::event::connection_disconnected_event_type)
	{
		printf("Disconnection message: %s\n", event.data().c_str());
		return true;
	}
	
	printf("Unknown event type\n");
	return false;
}

int main(int argc, const char **argv)
{	
	torque::socket my_socket("localhost:31337");
	my_socket.allow_incoming_connections(true);
	my_socket.set_challenge_response_data("Welcome to the test server.");

	while(true)
	{
		torque::event evt = my_socket.get_next_event();
		bool end = false;
		while(evt.event_type())
		{
			if(handle_event(evt))
				end = true;
			evt = my_socket.get_next_event();
		}
		if(end)
			break;
		sleep(1);
	}

	return 0;
}
