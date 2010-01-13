#ifndef TORQUE_SOCKETS_CPP_H_
#define TORQUE_SOCKETS_CPP_H_

#include <string>
#include <vector>

namespace torque
{
	class socket;
	class connection;
	class event
	{
	public:
		enum type
		{
			connection_challenge_response_event_type = 1,
			connection_requested_event_type,
			connection_arranged_connection_request_event_type,
			connection_accepted_event_type,
			connection_rejected_event_type,
			connection_timed_out_event_type,
			connection_disconnected_event_type,
			connection_established_event_type,
			connection_packet_event_type,
			connection_packet_notify_event_type,
			socket_packet_event_type,
		};
		
		event();
		event(const event& other);
		event& operator=(const event& other);
		~event();
		
		type event_type() const;
		connection source_connection() const;
		connection introducer() const;
		std::string client_identity() const;
		std::string public_key() const;
		std::string data() const;
		unsigned packet_sequence() const;
		bool delivered() const;
		std::string source_address() const;
	
	private:
		friend class socket;
		void set(void*);
		struct impl;
		impl* e;
	};
	
	class send_result
	{
	public:
		bool sent() const { return sent_; }
		unsigned packet_sequence() const { return packet_sequence_; }
		bool sent_;
		unsigned packet_sequence_;
	};

	class connection
	{
	public:
		connection() : c(0) {}
		connection(const connection& other) : c(other.c) {}
		connection& operator=(const connection& other) { c = other.c; return *this; }
		~connection();
		
		connection introduce_me(const std::string& remote_client, const std::string& connect_data);
		
		void accept(const std::string& accept_data);
		void reject(const std::string& reject_data);
		
		send_result send(const std::string& data);
		
		void disconnect(const std::string& disconnect_data);
		
	private:
		friend class socket;
		friend class event;
		connection(unsigned);
		unsigned c;
	};

	class socket
	{
	public:
		socket(const std::string& address);
		~socket();
		
		void seed_entropy(const std::string& entropy);
		std::string read_entropy();
		
		void set_private_key(const std::string& key);
		
		void set_challenge_response_data(const std::string& data);
		
		void allow_incoming_connections(bool allow);
		bool does_allow_incoming_connections();
		
		void can_be_introducer(bool);
		bool is_introducer();
		
		connection connect(const std::string& address, const std::string& connect_data);
		
		event get_next_event();
		
	private:
		unsigned s;
	};
}

#endif
