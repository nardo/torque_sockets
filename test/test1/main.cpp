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

struct nat_discovery {
#include "nat_discovery.h"
};

int main(int argc, const char **argv)
{
	ltc_mp = ltm_desc;
	net::address bind_address(net::address::any, 0);
	
	ref_ptr<net::interface> my_interface = new net::interface(bind_address);

	return nat_discovery::run(argc, argv);
}
