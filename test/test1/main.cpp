// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "tomcrypt.h"
#include "platform/platform.h"
#include "core/core.h"

struct net {
#include "net/net.h"
#include "crypto/crypto.h"
};

struct nat_discovery {
#include "nat_discovery.h"
};

int main(int argc, const char **argv)
{
	return nat_discovery::run(argc, argv);
}
