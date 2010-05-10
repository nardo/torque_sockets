// socket_event_queue.h - A queue for torque_socket_event records.
// Copyright GarageGames.  torque sockets API and prototype implementation are released under the MIT license.  See /license/info.txt in this distribution for specific details.

class socket_event_queue
{
public:
	struct queue_entry {
		torque_socket_event *event;
		queue_entry *next_event;
	};
	
	queue_entry *_event_queue_tail;
	queue_entry *_event_queue_head;
	page_allocator<16> _allocator;
	
	socket_event_queue(zone_allocator *allocator) : _allocator(allocator)
	{
		_event_queue_head = 0;
		_event_queue_tail = 0;
	}
	
	bool has_event()
	{
		return _event_queue_head != 0;
	}
	void clear()
	{
		_allocator.clear();
	}
	
	torque_socket_event *dequeue()
	{
		assert(_event_queue_head);
		torque_socket_event *ret = _event_queue_head->event;
		_event_queue_head = _event_queue_head->next_event;
		if(!_event_queue_head)
			_event_queue_tail = 0;
		return ret;
	}
	
	uint8 *allocate_queue_data(uint32 data_size)
	{
		return (uint8 *) _allocator.allocate(data_size);
	}
	
	torque_socket_event *post_event(uint32 event_type, torque_connection_id connection_id = 0)
	{
		torque_socket_event *ret = (torque_socket_event *) _allocator.allocate(sizeof(torque_socket_event));
		queue_entry *entry = (queue_entry *) _allocator.allocate(sizeof(queue_entry));
		
		entry->event = ret;
		
		ret->event_type = event_type;
		ret->data = 0;
		ret->key = 0;
		ret->data_size = 0;
		ret->key_size = 0;
		ret->connection = connection_id;

		entry->next_event = 0;
		if(_event_queue_tail)
		{
			_event_queue_tail->next_event = entry;
			_event_queue_tail = entry;
		}
		else
			_event_queue_head = _event_queue_tail = entry;
		return entry->event;
	}
	
	void set_event_data(torque_socket_event *event, uint8 *data, uint32 data_size)
	{
		event->data_size = data_size;
		event->data = allocate_queue_data(data_size);
		memcpy(event->data, data, data_size);
	}
	
	void set_event_key(torque_socket_event *event, uint8 *key, uint32 key_size)
	{
		event->key_size = key_size;
		event->key = allocate_queue_data(key_size);
		memcpy(event->key, key, key_size);
	}
};

