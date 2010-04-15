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

#include "plugin_framework.h"

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
	}
	
	void set_key_pair(NPObject *public_private_key)
	{
		empty_type return_value;
		call_function(public_private_key, &return_value);
	}
	
	int connect(core::string url, core::string connect_data, core::string protocol_settings)
	{
		return 0;
	}
	
	int connect_introduced (int introducer, int remote_client_connection_id, bool is_host, core::string connect_data_or_challenge_response, core::string protocol_settings)
	{
		return 0;
	}
	
	int introduce(int initiator_connection, int host_connection)
	{
		return 0;
	}
	
	void accept_challenge(int pending_connection)
	{
		logprintf("Got accept_challenge %d", pending_connection);
	}
	
	void accept_connection(int pending_connection)
	{
		string return_value;
		char buffer[100];
		sprintf(buffer, "OMG%d!", pending_connection);
		string arg(buffer);
		logprintf("Calling on_established");
		call_function(on_established, &return_value, arg);
		logprintf("Returned: \"%s\"", return_value.c_str());
	}
	
	void close(int connection_id, core::string reason)
	{
		
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
		tnl_method(db, torque_socket_instance, set_key_pair);
		tnl_method(db, torque_socket_instance, connect);
		tnl_method(db, torque_socket_instance, connect_introduced);
		tnl_method(db, torque_socket_instance, introduce);
		tnl_method(db, torque_socket_instance, accept_challenge);
		tnl_method(db, torque_socket_instance, accept_connection);
		tnl_method(db, torque_socket_instance, close);
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
	NPObject *create_torque_socket()
	{
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

void plugin_main()
{
	torque_socket_instance::register_class(global_type_database());
	torque_socket_plugin::register_class(global_type_database());
	global_plugin.add_class(get_global_type_record<torque_socket_instance>());
	global_plugin.add_class(get_global_type_record<torque_socket_plugin>());
	global_plugin.set_plugin_class(get_global_type_record<torque_socket_plugin>());
}

