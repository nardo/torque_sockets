// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "tomcrypt.h"
#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

using namespace core;
struct net {
	#include "net/net.h"
};

int main(int argc, const char **argv)
{
	ltc_mp = ltm_desc;
	net::random_generator rnd = net::random_generator();
	net::address bind_address(net::address::localhost, 31338);

	ref_ptr<net::interface> my_interface = new net::interface(bind_address);
	my_interface->set_allows_connections(true);

	net::address addr = my_interface->get_first_bound_interface_address();
	printf("%s\n", addr.to_string().c_str());

	ref_ptr<net::connection> my_connection = new net::connection(rnd);
	my_connection->connect(my_interface, net::address(net::address::localhost, 31337));

	while(true)
	{
		my_interface->check_incoming_packets();
		my_interface->process_connections();
		sleep(1);
	}

	return 0;
}
