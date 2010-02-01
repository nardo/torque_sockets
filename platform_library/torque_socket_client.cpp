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

struct
{
	nacl::Handle g_front;
	nacl::Handle pair[2];
} client_sockets;

static const core::uint32 MAX_RPC_SIZE = 2048;

void init_client(const char* client_addr, const char* server_addr)
{
	nacl::SocketAddress client;
	strncpy(client.path, client_addr, nacl::kPathMax);
	client_sockets.g_front = nacl::BoundSocket(&client);
	nacl::SocketPair(client_sockets.pair);

	nacl::SocketAddress server;
	strncpy(server.path, server_addr, nacl::kPathMax);

	nacl::MessageHeader header;
	header.iov = NULL;
	header.iov_length = 0;
	header.handles = &client_sockets.pair[1];
	header.handle_count = 1;
	nacl::SendDatagramTo(client_sockets.g_front, &header, 0, &server);
	
	nacl::Close(client_sockets.pair[1]);
}

void send_rpc(core::bit_stream& buf)
{
	nacl::MessageHeader header;
	nacl::IOVec vec[1];
	vec[0].base = buf.get_buffer();
	vec[0].length = buf.get_next_byte_position();
	header.iov = vec;
	header.iov_length = 1;
	header.handles = NULL;
	header.handle_count = 0;
	nacl::SendDatagram(client_sockets.pair[0], &header, 0);
}

void rpc_reply(core::bit_stream& buf)
{
	char buffer[4096];
	nacl::IOVec vec[1];
	vec[0].base = buffer;
	vec[0].length = sizeof buffer;
	nacl::MessageHeader header;
	header.iov = vec;
	header.iov_length = 1;
	header.handles = NULL;
	header.handle_count = 0;
	int bytes_recv = nacl::ReceiveDatagram(client_sockets.pair[0], &header, 0);
	buf.write_bytes((core::byte*)buffer, bytes_recv);
	buf.set_byte_position(0);
}

torque_socket torque_socket_create(const char* address)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_create_rpc);
	core::byte_buffer_ptr addr = new core::byte_buffer((core::uint8*)address, strlen(address) + 1);
	core::write(buf, addr);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	torque_socket ret;
	core::read(rpl, ret); 
	return ret;
}

void torque_socket_destroy(torque_socket sock)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_destroy_rpc);
	core::write(buf, sock);
	send_rpc(buf);
}

void torque_socket_seed_entropy(torque_socket sock, unsigned char entropy[32])
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_seed_entropy_rpc);
	core::write(buf, sock);
	core::byte_buffer_ptr e = new core::byte_buffer(entropy, 32);
	core::write(buf, e);
	send_rpc(buf);
}

void torque_socket_set_private_key(torque_socket sock, unsigned key_data_size, unsigned char *the_key)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_set_private_key_rpc);
	core::write(buf, sock);
	core::byte_buffer_ptr k = new core::byte_buffer(the_key, key_data_size);
	core::write(buf, k);
	send_rpc(buf);
}

void torque_socket_set_challenge_response_data(torque_socket sock, unsigned challenge_response_data_size, status_response challenge_response_data)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_set_challenge_response_data_rpc);
	core::write(buf, sock);
	core::byte_buffer_ptr c = new core::byte_buffer(challenge_response_data, challenge_response_data_size);
	core::write(buf, c);
	send_rpc(buf);
}

void torque_socket_allow_incoming_connections(torque_socket sock, int allowed)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_allow_incoming_connections_rpc);
	core::write(buf, sock);
	core::write(buf, allowed);
	send_rpc(buf);
}

int torque_socket_does_allow_incoming_connections(torque_socket sock)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_does_allow_incoming_connections_rpc);
	core::write(buf, sock);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	int ret;
	core::read(rpl, ret);
	return ret;
}

void torque_socket_can_be_introducer(torque_socket sock, int allowed)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_can_be_introducer_rpc);
	core::write(buf, sock);
	core::write(buf, allowed);
	send_rpc(buf);
}

int torque_socket_is_introducer(torque_socket sock)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_is_introducer_rpc);
	core::write(buf, sock);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	int ret;
	core::read(rpl, ret);
	return ret;
}

struct torque_socket_event *torque_socket_get_next_event(torque_socket sock)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_get_next_event_rpc);
	core::write(buf, sock);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	
	static torque_socket_event event;
	core::read(rpl, event.event_type);
	if(event.event_type == 0)
		return NULL;

	core::read(rpl, event.connection);
	core::read(rpl, event.arranger_connection);
	core::read(rpl, event.client_identity);
	core::byte_buffer_ptr b;
	core::read(rpl, b);
	event.public_key_size = b->get_buffer_size();
	memcpy(event.public_key, b->get_buffer(), b->get_buffer_size());
	core::read(rpl, b);
	event.data_size = b->get_buffer_size();
	memcpy(event.data, b->get_buffer(), b->get_buffer_size());
	core::read(rpl, event.packet_sequence);
	core::read(rpl, event.delivered);
	core::read(rpl, b);
	strncpy(event.source_address, (char*)b->get_buffer(), sizeof(event.source_address));
	
	return &event;
}

torque_connection torque_socket_connect(torque_socket sock, const char* remote_host, unsigned connect_data_size, status_response connect_data)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_socket_connect_rpc);
	core::write(buf, sock);
	core::byte_buffer_ptr b = new core::byte_buffer((core::byte*)remote_host, strlen(remote_host) + 1);
	core::write(buf, b);
	b = new core::byte_buffer(connect_data, connect_data_size);
	core::write(buf, b);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	torque_connection ret;
	core::read(rpl, ret);
	return ret;
}

torque_connection torque_connection_introduce_me(torque_connection introducer, unsigned remote_client_identity, unsigned connect_data_size, status_response connect_data)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_connection_introduce_me_rpc);
	core::write(buf, introducer);
	core::write(buf, remote_client_identity);
	core::byte_buffer_ptr b = new core::byte_buffer(connect_data, connect_data_size);
	core::write(buf, b);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	torque_connection ret;
	core::read(rpl, ret);
	return ret;
}

void torque_connection_accept(torque_connection conn, unsigned connect_accept_data_size, status_response connect_accept_data)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_connection_accept_rpc);
	core::write(buf, conn);
	core::byte_buffer_ptr b = new core::byte_buffer(connect_accept_data, connect_accept_data_size);
	core::write(buf, b);
	send_rpc(buf);
}

void torque_connection_reject(torque_connection conn, unsigned reject_data_size, status_response reject_data)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_connection_reject_rpc);
	core::write(buf, conn);
	core::byte_buffer_ptr b = new core::byte_buffer(reject_data, reject_data_size);
	core::write(buf, b);
	send_rpc(buf);
}

void torque_connection_disconnect(torque_connection conn, unsigned disconnect_data_size, status_response disconnect_data)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_connection_disconnect_rpc);
	core::write(buf, conn);
	core::byte_buffer_ptr b = new core::byte_buffer(disconnect_data, disconnect_data_size);
	core::write(buf, b);
	send_rpc(buf);
}

int torque_connection_send_to(torque_connection conn, unsigned datagram_size, unsigned char data[torque_max_datagram_size], unsigned *sequence_number)
{
	core::byte buffer[MAX_RPC_SIZE];
	core::bit_stream buf(buffer, sizeof buffer);
	core::write(buf, (core::uint8)torque_connection_send_to_rpc);
	core::write(buf, conn);
	core::byte_buffer_ptr b = new core::byte_buffer(data, datagram_size);
	core::write(buf, b);
	send_rpc(buf);
	
	core::byte reply_buf[4096];
	core::bit_stream rpl(reply_buf, sizeof reply_buf);
	rpc_reply(rpl);
	int ret;
	core::read(rpl, ret);
	if(sequence_number)
		core::read(rpl, *sequence_number);
	return ret;
}
