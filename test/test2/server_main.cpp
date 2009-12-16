// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "net_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

tnp_interface my_interface;
tnp_connection my_connection;

bool handle_event(tnp_event& evt)
{
	if (evt.event_type == tnp_event::tnp_connection_requested_event)
	{
		printf("Connection requested from %s\n", evt.source_address);
		printf("Connection message: %s\n", evt.data);
		const char* msg = "Hello Mr. Client";
		my_connection = tnp_accept_connection(my_interface, &evt, strlen(msg) + 1, (unsigned char*)msg);
		return false;
	}
	
	printf("Unknown event type\n");
	return false;
}

int main(int argc, const char **argv)
{
	my_interface = tnp_create_interface("localhost:31337");
	tnp_allow_incoming_connections(my_interface, true);

	while(true)
	{
		tnp_event evt;
		if (tnp_get_next_event(my_interface, &evt))
		{
			if(!handle_event(evt))
				break;
		}
		
		sleep(1);
	}
	
	tnp_destroy_interface(&my_interface);

	return 0;
}
