#include <math.h>
#include <stdio.h>
#include <iostream>
#include "tomcrypt.h"

#include "core/platform.h"
#include "torque_sockets/torque_sockets_c_api.h"

namespace core
{
	#include "core/core.h"
	struct net {
		#include "torque_sockets/torque_sockets.h"
	};
};

#include "torque_sockets/torque_sockets_c_implementation.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "plugin_framework/npapi.h"
#include "plugin_framework/npupp.h"
#include "plugin_framework/npapi.h"
#include "plugin_framework/npupp.h"

//#include <map>
//#include <string>
//#include <string.h>
//#include <stdio.h>

#ifndef WIN32
#define OSCALL
#endif

#include "plugin_framework/plugin_framework.h"

class torque_socket_instance : public scriptable_object
{
	torque_socket_handle _socket;
	NPObjectRef on_challenge_response;
	NPObjectRef on_connect_request;
	NPObjectRef on_established;
	NPObjectRef on_close;
	NPObjectRef on_packet;
	NPObjectRef on_packet_delivery_notify;
public:
	torque_socket_instance()
	{
		logprintf("torque_socket_instance constructor");
		_socket = torque_socket_create(true, background_socket_notify, this);
	}
	
	~torque_socket_instance()
	{
		torque_socket_destroy(_socket);
	}
	
	static void background_socket_notify(void *the_socket_instance)
	{
		logprintf("back-call %d", net::time::get_current().get_milliseconds() % 1000);
		torque_socket_instance *inst = (torque_socket_instance *) the_socket_instance;
		browser->pluginthreadasynccall(inst->get_plugin_instance(), main_thread_socket_notify, the_socket_instance);
	}
									
	static void main_thread_socket_notify(void *the_socket_instance)
	{
		torque_socket_instance *inst = (torque_socket_instance *) the_socket_instance;		
		inst->pump();
	}
	
	void pump()
	{
		logprintf("pump %d", net::time::get_current().get_milliseconds() % 1000);

		// pump the socket's event queue and generate events to post back from the plugin.
		torque_socket_event *event;
		empty_type void_return_value;
		string key, message;
		
		while((event = torque_socket_get_next_event(_socket)) != NULL)
		{
			int connection = event->connection;
			int sequence = event->packet_sequence;
			switch(event->event_type)
			{
				case torque_connection_challenge_response_event_type:
					key.set((const char *) event->public_key, event->public_key_size);
					message.set((const char *) event->data, event->data_size);
					call_function(on_challenge_response, &void_return_value, connection, key, message);
					break;
				case torque_connection_requested_event_type:
					key.set((const char *) event->public_key, event->public_key_size);
					message.set((const char *) event->data, event->data_size);
					call_function(on_connect_request, &void_return_value, connection, key, message);
					break;
				case torque_connection_arranged_connection_request_event_type:
					break;
				case torque_connection_timed_out_event_type:
					message.set("timeout");
					call_function(on_close, &void_return_value, connection, message);
					break;
				case torque_connection_disconnected_event_type:
					message.set((const char *) event->data, event->data_size);
					call_function(on_close, &void_return_value, connection, message);
					break;
				case torque_connection_established_event_type:
					call_function(on_established, &void_return_value, connection);
					break;
				case torque_connection_packet_event_type:
					message.set((const char *) event->data, event->data_size);
					call_function(on_packet, &void_return_value, connection, sequence, message);
					break;
				case torque_connection_packet_notify_event_type:
					call_function(on_packet_delivery_notify, &void_return_value, connection, sequence, event->delivered);
					break;
				case torque_socket_packet_event_type:
					break;
			}
		}
	}
	
	bool bind(core::string bind_address)
	{
		core::net::address the_addr(bind_address.c_str(), false, 0);
		sockaddr addr;
		the_addr.to_sockaddr(&addr);
		bind_result r = torque_socket_bind(_socket, &addr);
		logprintf("Bind? %d", r == bind_success);
		return r == bind_success;
	}

	void set_key_pair(core::string the_key)
	{
		const char *str = the_key.c_str();
		//byte_buffer_ptr key_buffer = buffer_decode(str, strlen(str));
		
		torque_socket_set_key_pair(_socket, the_key.len(), (core::uint8*)the_key.c_str());
	}
	
	void set_challenge_response(core::string the_response)
	{
		torque_socket_set_challenge_response(_socket, the_response.len(), (core::uint8*)the_response.c_str());		
	}
	
	int connect(core::string url, core::string connect_data, core::string protocol_settings)
	{
		core::net::address connect_address(url.c_str(), false, 0);
		sockaddr addr;
		connect_address.to_sockaddr(&addr);
		return torque_socket_connect(_socket, &addr, connect_data.len(), (core::uint8*) connect_data.c_str());
	}
	
	int connect_introduced (int introducer, int remote_client_connection_id, bool is_host, core::string connect_data_or_challenge_response, core::string protocol_settings)
	{
		return torque_socket_connect_introduced(_socket, introducer, remote_client_connection_id, is_host, connect_data_or_challenge_response.len(), (core::uint8 *) connect_data_or_challenge_response.c_str());
	}
	
	void introduce(int initiator_connection, int host_connection)
	{
		torque_socket_introduce(_socket, initiator_connection, host_connection);
	}
	
	void accept_challenge(int pending_connection)
	{
		logprintf("accept_challenge %d", pending_connection);
		torque_socket_accept_challenge(_socket, pending_connection);
	}
	
	void accept_connection(int pending_connection)
	{
		logprintf("accept_connection %d", pending_connection);
		torque_socket_accept_connection(_socket, pending_connection);
		/*
		string return_value;
		char buffer[100];
		sprintf(buffer, "OMG%d!", pending_connection);
		string arg(buffer);
		logprintf("Calling on_established");
		call_function(on_established, &return_value, arg);
		logprintf("Returned: \"%s\"", return_value.c_str());*/
	}
	
	void close_connection(int connection_id, core::string reason)
	{
		logprintf("close connection %d", connection_id);
		torque_socket_close_connection(_socket, connection_id, reason.len(), (core::uint8*) reason.c_str());
	}
	
	int send_to(int connection_id, core::string packet_data)
	{
		return torque_socket_send_to_connection(_socket, connection_id, packet_data.len(), (core::uint8*) packet_data.c_str());
	}

	static void register_class(core::type_database &db)
	{
		tnl_begin_class(db, torque_socket_instance, scriptable_object, true);
		tnl_slot(db, torque_socket_instance, on_challenge_response, 0);
		tnl_slot(db, torque_socket_instance, on_connect_request, 0);
		tnl_slot(db, torque_socket_instance, on_established, 0);
		tnl_slot(db, torque_socket_instance, on_close, 0);
		tnl_slot(db, torque_socket_instance, on_packet, 0);
		tnl_slot(db, torque_socket_instance, on_packet_delivery_notify, 0);
		tnl_method(db, torque_socket_instance, bind);
		tnl_method(db, torque_socket_instance, set_key_pair);
		tnl_method(db, torque_socket_instance, set_challenge_response);
		tnl_method(db, torque_socket_instance, connect);
		tnl_method(db, torque_socket_instance, connect_introduced);
		tnl_method(db, torque_socket_instance, introduce);
		tnl_method(db, torque_socket_instance, accept_challenge);
		tnl_method(db, torque_socket_instance, accept_connection);
		tnl_method(db, torque_socket_instance, close_connection);
		tnl_method(db, torque_socket_instance, send_to);
		tnl_end_class(db);
	}
};

class torque_socket_plugin : public scriptable_object
{
public:
	torque_socket_plugin()
	{
		logprintf("torque_socket_plugin constructor");
	}
	~torque_socket_plugin()
	{
		logprintf("torque_socket_plugin destructor");
	}
	NPObject *create_torque_socket()
	{
		logprintf("create_torque_socket called. %08x, %08x\n", this, get_plugin_instance());
		NPObject* object = global_plugin.create_object(get_plugin_instance(), core::get_global_type_record<torque_socket_instance>());
		return object;
	}
	static void register_class(core::type_database &the_database)
	{
		tnl_begin_class(the_database, torque_socket_plugin, scriptable_object, true);
		tnl_method(the_database, torque_socket_plugin, create_torque_socket);
		tnl_end_class(the_database);
	}
};

void plugin_initialize()
{
	ltc_mp = ltm_desc;
	torque_socket_instance::register_class(global_type_database());
	torque_socket_plugin::register_class(global_type_database());
	global_plugin.add_class(get_global_type_record<torque_socket_instance>());
	global_plugin.add_class(get_global_type_record<torque_socket_plugin>());
	global_plugin.set_plugin_class(get_global_type_record<torque_socket_plugin>());
}

void plugin_shutdown()
{

}