#ifndef TNP_EVENT_H_
#define TNP_EVENT_H_

class connection;

struct torque_socket_event_ex
{
	torque_socket_event_ex() : conn(NULL), arragner_conn(NULL)
	{
		memset(&data, 0, sizeof(data));
	}

	torque_socket_event data;
	ref_ptr<connection> conn;
	ref_ptr<connection> arragner_conn;
};

#endif
