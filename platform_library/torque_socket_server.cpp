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

typedef void(*shim_func)(core::bit_stream&);
shim_func shim_table[torque_max_rpc];

nacl::Handle g_front;
nacl::Handle handles[2];
nacl::Handle client_handle;

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
				}
			}
			else
			{
				result = nacl::ReceiveDatagram(client_handle, &header, 0);
				if(result > 0)
				{
					core::bit_stream buf((core::byte*)header.iov[0].base, header.iov[0].length);
					core::uint8 shim;
					core::read(buf, shim);
					shim_table[shim](buf);
				}
				if(result == 0)
					break;
			}
			if(result < 0)
				PrintError("ReceiveDatagram");
		}
		nacl::Close(g_front);
	}

	char buffer[4096];
	nacl::IOVec vec[1];
	nacl::MessageHeader header;
};

void rpc_reply(core::bit_stream& buf)
{
	nacl::MessageHeader header;
	nacl::IOVec vec[1];
	vec[0].base = buf.get_buffer();
	vec[0].length = buf.get_next_byte_position();
	header.iov = vec;
	header.iov_length = 1;
	header.handles = NULL;
	header.handle_count = 0;
	nacl::SendDatagram(client_handle, &header, 0);
}

void torque_socket_create_shim(core::bit_stream& buffer)
{
	printf("torque_socket_create_shim\n");
	core::byte_buffer_ptr addr;
	core::read(buffer, addr);
	printf("address: %s\n", addr->get_buffer());
	
	torque_socket ret = torque_socket_create((const char*)addr->get_buffer());
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	core::write(rpl, ret);
	rpc_reply(rpl);
}

void torque_socket_destroy_shim(core::bit_stream& buffer)
{
	printf("torque_socket_destroy_shim\n");
	torque_socket s;
	core::read(buffer, s);
	torque_socket_destroy(s);
}

void torque_socket_seed_entropy_shim(core::bit_stream& buffer)
{
	printf("torque_socket_seed_entropy_shim\n");
	torque_socket s;
	core::read(buffer, s);
	core::byte_buffer_ptr e;
	core::read(buffer, e);
	torque_socket_seed_entropy(s, e->get_buffer());
}

void torque_socket_set_private_key_shim(core::bit_stream& buffer)
{
	printf("torque_socket_set_private_key_shim\n");
	torque_socket s;
	core::read(buffer, s);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_socket_set_private_key(s, b->get_buffer_size(), b->get_buffer());
}

void torque_socket_set_challenge_response_data_shim(core::bit_stream& buffer)
{
	printf("torque_socket_set_challenge_response_data_shim\n");
	torque_socket s;
	core::read(buffer, s);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_socket_set_challenge_response_data(s, b->get_buffer_size(), b->get_buffer());
}

void torque_socket_allow_incoming_connections_shim(core::bit_stream& buffer)
{
	printf("torque_socket_allow_incoming_connections_shim\n");
	torque_socket s;
	core::read(buffer, s);
	int a;
	core::read(buffer, a);
	torque_socket_allow_incoming_connections(s, a);
}

void torque_socket_does_allow_incoming_connections_shim(core::bit_stream& buffer)
{
	printf("torque_socket_does_allow_incoming_connections_shim\n");
	torque_socket s;
	core::read(buffer, s);
	int ret = torque_socket_does_allow_incoming_connections(s);
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	core::write(rpl, ret);
	rpc_reply(rpl);
}

void torque_socket_can_be_introducer_shim(core::bit_stream& buffer)
{
	printf("torque_socket_can_be_introducer_shim\n");
	torque_socket s;
	core::read(buffer, s);
	int a;
	core::read(buffer, a);
	torque_socket_can_be_introducer(s, a);
}

void torque_socket_is_introducer_shim(core::bit_stream& buffer)
{
	printf("torque_socket_is_introducer_shim\n");
	torque_socket s;
	core::read(buffer, s);
	int ret = torque_socket_is_introducer(s);
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	core::write(rpl, ret);
	rpc_reply(rpl);
}

void torque_socket_get_next_event_shim(core::bit_stream& buffer)
{
	printf("torque_socket_get_next_event_shim\n");
	torque_socket s;
	core::read(buffer, s);
	
	torque_socket_event* event = torque_socket_get_next_event(s);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	if(!event)
	{
		core::write(rpl, 0);
		rpc_reply(rpl);
		return;
	}
	
	core::write(rpl, event->event_type);
	core::write(rpl, event->connection);
	core::write(rpl, event->arranger_connection);
	core::write(rpl, event->client_identity);
	core::byte_buffer_ptr b = new core::byte_buffer(event->public_key, event->public_key_size);
	core::write(rpl, b);
	b = new core::byte_buffer(event->data, event->data_size);
	core::write(rpl, b);
	core::write(rpl, event->packet_sequence);
	core::write(rpl, event->delivered);
	b = new core::byte_buffer((core::uint8*)event->source_address, strlen(event->source_address) + 1);
	core::write(rpl, b);
	rpc_reply(rpl);
}

void torque_socket_connect_shim(core::bit_stream& buffer)
{
	printf("torque_socket_connect_shim\n");
	torque_socket s;
	core::read(buffer, s);
	core::byte_buffer_ptr addr;
	core::read(buffer, addr);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_connection ret = torque_socket_connect(s, (const char*)addr->get_buffer(), b->get_buffer_size(), b->get_buffer());
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	core::write(rpl, ret);
	rpc_reply(rpl);
}

void torque_connection_introduce_me_shim(core::bit_stream& buffer)
{
	printf("torque_connection_introduce_me_shim\n");
	torque_connection c;
	core::read(buffer, c);
	unsigned r;
	core::read(buffer, r);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_connection ret = torque_connection_introduce_me(c, r, b->get_buffer_size(), b->get_buffer());
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	core::write(rpl, ret);
	rpc_reply(rpl);
}

void torque_connection_accept_shim(core::bit_stream& buffer)
{
	printf("torque_connection_accept_shim\n");
	torque_connection c;
	core::read(buffer, c);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_connection_accept(c, b->get_buffer_size(), b->get_buffer());
}

void torque_connection_reject_shim(core::bit_stream& buffer)
{
	printf("torque_connection_reject_shim\n");
	torque_connection c;
	core::read(buffer, c);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_connection_reject(c, b->get_buffer_size(), b->get_buffer());
}

void torque_connection_disconnect_shim(core::bit_stream& buffer)
{
	printf("torque_connection_disconnect_shim\n");
	torque_connection c;
	core::read(buffer, c);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	torque_connection_disconnect(c, b->get_buffer_size(), b->get_buffer());
}

void torque_connection_send_to_shim(core::bit_stream& buffer)
{
	printf("torque_connection_send_to_shim\n");
	torque_connection c;
	core::read(buffer, c);
	core::byte_buffer_ptr b;
	core::read(buffer, b);
	unsigned s;
	int ret = torque_connection_send_to(c, b->get_buffer_size(), b->get_buffer(), &s);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	core::write(rpl, ret);
	core::write(rpl, s);
	rpc_reply(rpl);
}

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
	
	shim_table[torque_socket_create_rpc] = &torque_socket_create_shim;
	shim_table[torque_socket_destroy_rpc] = &torque_socket_destroy_shim;
	shim_table[torque_socket_seed_entropy_rpc] = &torque_socket_seed_entropy_shim;
	shim_table[torque_socket_set_private_key_rpc] = &torque_socket_set_private_key_shim;
	shim_table[torque_socket_set_challenge_response_data_rpc] = &torque_socket_set_challenge_response_data_shim;
	shim_table[torque_socket_allow_incoming_connections_rpc] = &torque_socket_allow_incoming_connections_shim;
	shim_table[torque_socket_does_allow_incoming_connections_rpc] = &torque_socket_does_allow_incoming_connections_shim;
	shim_table[torque_socket_can_be_introducer_rpc] = &torque_socket_can_be_introducer_shim;
	shim_table[torque_socket_is_introducer_rpc] = &torque_socket_is_introducer_shim;
	shim_table[torque_socket_get_next_event_rpc] = &torque_socket_get_next_event_shim;
	shim_table[torque_connection_introduce_me_rpc] = &torque_connection_introduce_me_shim;
	shim_table[torque_connection_accept_rpc] = &torque_connection_accept_shim;
	shim_table[torque_connection_reject_rpc] = &torque_connection_reject_shim;
	shim_table[torque_connection_disconnect_rpc] = &torque_connection_disconnect_shim;
	shim_table[torque_connection_send_to_rpc] = &torque_connection_send_to_shim;

	runner.start();
}
