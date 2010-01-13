#include "torque_sockets.h"

#include "tomcrypt.h"
#include "platform/platform.h"
namespace core
{
	
#include "core/core.h"

};

#include <queue>

using namespace core;
struct net {
	#include "net/net.h"
};

#include <vector>

#include "torque_sockets_cpp.h"

static std::vector<ref_ptr<net::interface> > interfaces;
static std::vector<ref_ptr<net::connection> > connections;

static inline int get_idx_for_socket_(torque_socket socket)
{
	if(socket <= 0)
		return -1;

	if(socket > interfaces.size())
		return -1;

	return socket - 1;
}

static inline int get_idx_for_connection_(torque_connection connection)
{
	if(connection <= 0)
		return -1;

	if(connection > connections.size())
		return -1;

	return connection - 1;
}

static inline ref_ptr<net::interface>& get_interface_for_socket_(torque_socket socket)
{
	int idx = get_idx_for_socket_(socket);
	assert(idx != -1);
	return interfaces[idx];
}

static inline ref_ptr<net::connection>& get_connection_for_connection_(torque_connection connection)
{
	int idx = get_idx_for_connection_(connection);
	assert(idx != -1)
	return connections[idx];
}

torque_socket torque_socket_create(struct sockaddr* socket_address)
{
	ltc_mp = ltm_desc;
	assert(socket_address);
	
	if(!socket_address)
		return 0;

	ref_ptr<net::interface> interface(new net::interface(*socket_address));
	for(int i = 0; i < interfaces.size(); ++i)
	{
		if(interfaces[i] == NULL)
		{
			interfaces[i] = interface;
			return i + 1;
		}
	}
	
	interfaces.push_back(interface);
	return interfaces.size();
}

void torque_socket_destroy(torque_socket socket)
{
	assert(socket > 0)
	if(socket <= 0)
		return;
	if(socket > interfaces.size())
		return;
	
	interfaces[socket - 1] = NULL;
}

void torque_socket_seed_entropy(torque_socket socket, unsigned char entropy[32])
{
}

void torque_socket_read_entropy(torque_socket socket, unsigned char entropy[32])
{
}

void torque_socket_set_private_key(torque_socket socket, unsigned key_data_size, unsigned char* the_key)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return;

	interface->set_private_key(new net::asymmetric_key(the_key, key_data_size));
}

void torque_socket_set_challenge_response_data(torque_socket socket, unsigned challenge_response_data_size, status_response challenge_response_data)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return;

	interface->set_challenge_response_data(new byte_buffer(challenge_response_data, challenge_response_data_size));
}

void torque_socket_can_be_introducer(torque_socket socket, int allowed)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return;
}

int torque_socket_is_introducer(torque_socket socket)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return 0;
	return 0;
}

torque_connection torque_socket_connect(torque_socket socket, struct sockaddr* remote_host, unsigned connect_data_size, status_response connect_data)
{
	assert(remote_host);

	if(!remote_host)
		return 0;

	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return 0;

	ref_ptr<net::connection> c(new net::connection(interface->random()));
	byte_buffer_ptr data = new byte_buffer(connect_data, connect_data_size);
	c->connect(interface, *remote_host, data);

	for(int i = 0; i < connections.size(); ++i)
	{
		if(connections[i] == NULL)
		{
			connections[i] = c;
			return i + 1;
		}
	}

	connections.push_back(c);
	return connections.size();
}

torque_connection torque_connection_introduce_me(torque_connection introducer, uint64 remote_client_identity, unsigned connect_data_size, status_response connect_data)
{
	ref_ptr<net::connection> connection = get_connection_for_connection_(introducer);
	if(connection == NULL)
		return 0;

	return 0;
}

void torque_connection_accept(torque_connection conn, unsigned connect_accept_data_size, status_response connect_accept_data)
{
	ref_ptr<net::connection> connection = get_connection_for_connection_(conn);
	if(connection == NULL)
		return;

	byte_buffer_ptr data = new byte_buffer(connect_accept_data, connect_accept_data_size);

	connection->get_interface()->tnp_accept_connection(connection, data);
}

void torque_connection_reject(torque_connection conn, unsigned connect_reject_data_size, status_response connect_reject_data)
{
	ref_ptr<net::connection> connection = get_connection_for_connection_(conn);
	if(connection == NULL)
		return;

	byte_buffer_ptr data = new byte_buffer(connect_reject_data, connect_reject_data_size);

	connection->get_interface()->tnp_reject_connection(connection->get_address(), data);
}

int torque_connection_send_to(torque_connection conn, unsigned datagram_size, unsigned char buffer[torque_max_datagram_size], unsigned *sequence_number)
{
	ref_ptr<net::connection> connection = get_connection_for_connection_(conn);
	if(connection == NULL)
		return 0;

	byte_buffer_ptr data = new byte_buffer(buffer, datagram_size);
	std::pair<net::udp_socket::send_to_result, uint32> r = connection->tnp_send_data_packet(data);

	if(sequence_number)
		*sequence_number = r.second;

	if(r.first == net::udp_socket::send_to_success)
		return 1;

	return 0;
}

void torque_connection_disconnect(torque_connection conn, unsigned disconnect_data_size, status_response disconnect_data)
{
	ref_ptr<net::connection> connection = get_connection_for_connection_(conn);
	if(connection == NULL)
		return;

	byte_buffer_ptr data = new byte_buffer(disconnect_data, disconnect_data_size);
	connection->disconnect(data);
	connection = NULL;
}

static inline torque_connection get_connection_for_event_(const net::torque_socket_event_ex& evt)
{
	if(evt.conn.is_null())
		return 0;

	int32 idx = -1;
	for(int32 i = 0; i < connections.size(); ++i)
	{
		if(connections[i] == evt.conn)
		{
			idx = i;
			break;
		}
	}

	if(idx == -1)
	{
		connections.push_back(evt.conn);
		idx = connections.size() - 1;
	}

	return idx + 1;
}

static inline void fill_connection_(torque_socket_event& event, net::torque_socket_event_ex& evt)
{
	event.connection = get_connection_for_event_(evt);
	if(event.event_type == torque_connection_requested_event_type || event.event_type == torque_socket_packet_event_type)
	{
		evt.conn->get_address().to_sockaddr(&event.source_address);
	}
}

struct torque_socket_event* torque_socket_get_next_event(torque_socket socket)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return 0;

	interface->check_incoming_packets();
	interface->process_connections();

	static torque_socket_event event;
	memset(&event, 0, sizeof(event));

	net::torque_socket_event_ex evt;
	if(!interface->tnp_pop_event(evt))
		return NULL;

	event = evt.data;
	fill_connection_(event, evt);

	return &event;
}

void torque_socket_allow_incoming_connections(torque_socket socket, int allow)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return;
	interface->set_allows_connections(allow);
}

int torque_socket_does_allow_incoming_connections(torque_socket socket)
{
	ref_ptr<net::interface>& interface = get_interface_for_socket_(socket);
	if(interface == NULL)
		return 0;

	return interface->does_allow_connections();
}

namespace torque
{
	struct event::impl
	{
		torque_socket_event e;
	};

	event::event() : e(new impl)
	{
	}

	event::~event()
	{
		delete e;
	}

	event::event(const event& other) : e(new impl)
	{
		e->e = other.e->e;
	}
	
	event& event::operator=(const event& other)
	{
		e->e = other.e->e;
		return *this;
	}

	void event::set(void* v_e)
	{
		if(v_e)
		{
			e->e = *static_cast<torque_socket_event*>(v_e);
		}
		else
		{
			e->e.event_type = 0;
		}

	}

	event::type event::event_type() const
	{
		return (event::type)e->e.event_type;
	}

	connection event::source_connection() const
	{
		return connection(e->e.connection);
	}

	connection event::introducer() const
	{
		return connection(e->e.arranger_connection);
	}

	std::string event::client_identity() const
	{
		static char buffer[64];
		snprintf(buffer, sizeof(buffer), "%llu", e->e.client_identity);
		return buffer;
	}

	std::string event::public_key() const
	{
		return std::string(e->e.public_key, e->e.public_key + e->e.public_key_size);
	}

	std::string event::data() const
	{
		return std::string(e->e.data, e->e.data + e->e.data_size);
	}

	unsigned event::packet_sequence() const
	{
		return e->e.packet_sequence;
	}

	bool event::delivered() const
	{
		return e->e.delivered;
	}

	std::string event::source_address() const
	{
		net::address addr(e->e.source_address);
		return addr.to_string().c_str();
	}

	connection::connection(unsigned source) : c(source)
	{
	}

	connection::~connection()
	{
	}

	connection connection::introduce_me(const std::string& remote_client, const std::string& connect_data)
	{
		uint64 remote_id = 0;
		sscanf(remote_client.c_str(), "%llu", &remote_id);
		torque_connection ret = torque_connection_introduce_me(c, remote_id, connect_data.size() + 1, (unsigned char*)connect_data.c_str());
		return connection(ret);
	}

	void connection::accept(const std::string& accept_data)
	{
		torque_connection_accept(c, accept_data.size() + 1, (unsigned char*)accept_data.c_str());
	}

	void connection::reject(const std::string& reject_data)
	{
		torque_connection_reject(c, reject_data.size() + 1, (unsigned char*)reject_data.c_str());
	}

	send_result connection::send(const std::string& data)
	{
		send_result ret;
		ret.sent_ = torque_connection_send_to(c, data.size() + 1, (unsigned char*)data.c_str(), &ret.packet_sequence_);
		return ret;
	}

	void connection::disconnect(const std::string& disconnect_data)
	{
		status_response data;
		std::copy(disconnect_data.begin(), disconnect_data.end(), data);
		torque_connection_disconnect(c, disconnect_data.size() + 1, data);
	}

	socket::socket(const std::string& address)
	{
		net::address addr(address.c_str());
		sockaddr sock;
		addr.to_sockaddr(&sock);
		s = torque_socket_create(&sock);
	}

	socket::~socket()
	{
		torque_socket_destroy(s);
	}

	void socket::seed_entropy(const std::string& entropy)
	{
		torque_socket_seed_entropy(s, (unsigned char*)entropy.c_str());
	}

	std::string socket::read_entropy()
	{
		unsigned char entropy[32];
		torque_socket_read_entropy(s, entropy);
		return (const char*)entropy;
	}

	void socket::set_private_key(const std::string& key)
	{
		torque_socket_set_private_key(s, key.size() + 1, (unsigned char*)key.c_str());
	}

	void socket::set_challenge_response_data(const std::string& data)
	{
		torque_socket_set_challenge_response_data(s, data.size() + 1, (unsigned char*)data.c_str());
	}

	void socket::allow_incoming_connections(bool allow)
	{
		torque_socket_allow_incoming_connections(s, allow);
	}

	bool socket::does_allow_incoming_connections()
	{
		return torque_socket_does_allow_incoming_connections(s);
	}

	void socket::can_be_introducer(bool allow)
	{
		torque_socket_can_be_introducer(s, allow);
	}

	bool socket::is_introducer()
	{
		return torque_socket_is_introducer(s);
	}

	connection socket::connect(const std::string& address, const std::string& connect_data)
	{
		net::address addr(address.c_str());
		sockaddr sock;
		addr.to_sockaddr(&sock);

		torque_connection c = torque_socket_connect(s, &sock, connect_data.size() + 1, (unsigned char*)connect_data.c_str());

		return connection(c);
	}

	event socket::get_next_event()
	{
		static event e;
		torque_socket_event* evt = torque_socket_get_next_event(s);
		e.set(evt);
		return e;
	}
}
