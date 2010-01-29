#include "torque_sockets.h"
#include "nacl_imc.h"

#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

#include <stdio.h>
#include <string.h>

void PrintError(const char* message) {
	char buffer[256];

	if (nacl::GetLastErrorString(buffer, sizeof buffer) == 0) {
		fprintf(stderr, "%s: %s\n", message, buffer);
	}
}

nacl::Handle g_front;

class server_thread : public core::thread
{
public:
	server_thread()
	{
		client_handle = nacl::kInvalidHandle;
		memset(buffer, 0, sizeof buffer);
	}

	virtual core::uint32 run()
	{
		while(true)
		{
			vec[0].base = buffer;
			vec[0].length = sizeof buffer;
			header.iov = vec;
			header.iov_length = 1;
			header.handles = handles;
			header.handle_count = sizeof handles / sizeof handles[0];

			int result;
			if(client_handle == nacl::kInvalidHandle)
			{
				result = nacl::ReceiveDatagram(g_front, &header, 0);
				if(result == 0 && header.handle_count == 1)
				{
					client_handle = handles[0];
					printf("%d\n", client_handle);
				}
			}
			else
				result = nacl::ReceiveDatagram(client_handle, &header, 0);
			if(result != 0)
				PrintError("ReceiveDatagram");
		}
		nacl::Close(g_front);
	}

	nacl::Handle client_handle;
	char buffer[4096];
	nacl::Handle handles[2];
	nacl::IOVec vec[1];
	nacl::MessageHeader header;
};

server_thread runner;

void init_server(const char* server_address)
{
	nacl::SocketAddress addr;
	strncpy(addr.path, server_address, nacl::kPathMax);
	g_front = nacl::BoundSocket(&addr);
	if(g_front == nacl::kInvalidHandle)
	{
		PrintError("BoundSocket");
		return;
	}

	runner.start();
}
