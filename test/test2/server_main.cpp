// Copyright GarageGames.  See /license/info.txt in this distribution for licensing terms.

#include "torque_sockets_cpp.h"

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

class harness
{
public:
	harness(const std::string& address) : m_socket(address)
	{
		m_socket.allow_incoming_connections(true);
		m_socket.can_be_introducer(true);
		m_socket.set_challenge_response_data("Welcome to the test server.");
	}
	
	void connect(const std::string& address)
	{
		m_connections.push_back(m_socket.connect(address, "Connect"));
	}
	
	void introduce(const std::string& nonce)
	{
		m_connections[0].introduce_me(nonce, "Arranged");
	}
	
	void do_intro(const std::string& nonce)
	{
		m_intro_nonce = nonce;
	}
	
	void run_one_event()
	{
		torque::event evt = m_socket.get_next_event();
		bool end = false;
		if(evt.event_type())
		{
			handle_event(evt);
		}
	}

	bool handle_event(const torque::event& event)
	{
		if (event.event_type() == torque::event::connection_requested_event_type)
		{
	//		printf("Connection requested from %s\n", evt.source_address);
			printf("Connection message: %s\n", event.data().c_str());
			m_connections.push_back(event.source_connection());
			std::string msg = "Hello Mr. Client";
			m_connections[m_connections.size() - 1].accept(msg);
			return false;
		}
		
		if(event.event_type() == torque::event::connection_packet_event_type)
		{
			printf("Connection data: %s\n", event.data().c_str());
			return false;
		}
		
		if (event.event_type() == torque::event::connection_disconnected_event_type)
		{
			printf("Disconnection message: %s\n", event.data().c_str());
			return true;
		}
		
		if(event.event_type() == torque::event::connection_timed_out_event_type)
		{
			printf("Connection timed out\n");
			return true;
		}
		
		if(event.event_type() == torque::event::connection_accepted_event_type)
		{
			printf("Nonce is: %s\n", event.client_identity().c_str());
			if(m_connections.size() && m_intro_nonce.length())
			{
				introduce(m_intro_nonce);
				m_intro_nonce = "";
				return false;
			}
		}

		printf("Unknown event type %i\n", (int)event.event_type());
		return false;
	}
	
private:
	torque::socket m_socket;
	std::vector<torque::connection> m_connections;
	std::string m_intro_nonce;
};

int main(int argc, const char **argv)
{
	std::cout << "Enter address: ";
	std::string address;
	std::cin >> address;
	
	std::string connect = "";
	std::cout << "Connect? ";
	std::cin >> connect;
	bool do_connect = false;
	if(connect[0] == 'y')
	{
		std::cout << "Connect to: ";
		std::cin >> connect;
		do_connect = true;
	}
	
	std::string nonce = "";
	std::cout << "Introduce me? ";
	std::cin >> nonce;
	bool do_intro = false;
	if(nonce[0] == 'y')
	{
		std::cout << "Enter nonce for intro: ";
		std::cin >> nonce;
		do_intro = true;
	}
	
	harness h(address);
	if(do_connect)
		h.connect(connect);
	if(do_intro)
		h.do_intro(nonce);

	while(true)
	{
		h.run_one_event();
		sleep(1);
	}

	return 0;
}
