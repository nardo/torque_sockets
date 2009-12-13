// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "net_api.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

tnp_interface my_interface;
tnp_connection my_connection;

void handle_event(tnp_event& evt)
{
	if (evt.event_type == tnp_event::tnp_connection_accepted_event)
	{
		printf("Connection accepted from %s\n", evt.source_address);
		printf("Connection message: %s\n", evt.data);
		return;
	}
	
	printf("Unknown event type\n");
}

int main(int argc, const char **argv)
{
	my_interface = tnp_create_interface("localhost:31338");
	tnp_allow_incoming_connections(my_interface, false);
	
	const char* msg = "Hello Mr. Server";
	my_connection = tnp_connect(my_interface, "localhost:31337", strlen(msg) + 1, (unsigned char*)msg);

	while(true)
	{
		tnp_event evt;
		if (tnp_get_next_event(my_interface, &evt))
		{
			handle_event(evt);
		}
		sleep(1);
	}

	return 0;
}
