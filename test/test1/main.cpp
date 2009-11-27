// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "tomcrypt.h"
#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

namespace net
{
	using namespace core;
	struct internal {
		#include "crypto/crypto.h"
		#include "net/net.h"
	};
	typedef internal::address address;
	typedef internal::udp_socket udp_socket;
	typedef internal::time time;
};

using namespace core;
struct nat_discovery {
#include "nat_discovery.h"
};

int main(int argc, const char **argv)
{
	return nat_discovery::run(argc, argv);
}
