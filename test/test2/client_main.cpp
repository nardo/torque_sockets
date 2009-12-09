// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "net_api.h"

#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char **argv)
{
	tnp_interface my_interface = tnp_create_interface("localhost:31338");
	tnp_allow_incoming_connections(my_interface, false);
	
	tnp_connection my_connection = tnp_connect(my_interface, "localhost:31337", 0, NULL);

	while(true)
	{
		tnp_get_next_event(my_interface, NULL);
		sleep(1);
	}

	return 0;
}
