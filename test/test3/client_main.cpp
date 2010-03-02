// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "torque_sockets.h"
#include <unistd.h>

int main()
{
	init_client("test-ts-client", "test-ts-server");
	while(true)
		sleep(100000);
	return 0;
}
