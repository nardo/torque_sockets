
/// Managing object for a queue of worker threads that pass
/// messages back and forth to the main thread.  A request to a thread_queue will return a token that can be used to track the status of the call as well as cancel operations in flight.

class thread_queue : public ref_object
{
public:
	/// thread_queue constructor.  threadCount specifies the number of worker threads that will be created.
	thread_queue(uint32 threadCount)
	{
		_current_index = 0;
		_storage.set((void *) 1);
		for(uint32 i = 0; i < threadCount; i++)
		{
			thread *theThread = new thread_queue_thread(this);
			_threads.push_back(theThread);
			theThread->start();
		}
	}

	~thread_queue()
	{
	}
	
	/// Dispatches all thread_queue calls queued by worker threads.  This should
	/// be called periodically from a main loop.
	bool get_next_result(byte_buffer_ptr &result_buffer, uint32 &request_index)
	{
		bool found = false;
		lock();
		for(uint32 i = 0; i < _process_list.size();)
		{
			if(_process_list[i]->state == process_record::process_complete)
			{
				process_record *rec = _process_list[i];
				
				if(rec->cancelled)
				{
					delete rec;
					_process_list.erase(i);
					continue;
				}
				result_buffer = rec->response_buffer;
				request_index = rec->request_index;
				found = true;
				delete rec;
				_process_list.erase(i);
				break;
			}
			i++;
		}
		unlock();
		return found;
	}

	/// Posts a request to be handled.
	uint32 post_request(const byte_buffer_ptr &the_request)
	{
		lock();
		uint32 index = _current_index++;
		process_record *record = new process_record;
		record->request_buffer = the_request;
		record->state = process_record::not_yet_processed;
		record->request_index = index;
		record->cancelled = false;
		record->progress = 0;
		_process_list.push_back(record);
		_semaphore.increment();
		unlock();
		return index;
	}
	
	/// Cancels a request in flight -- worker process must periodically check cancellation flag to actually stop the process.
	bool cancel_request(uint32 request_index)
	{
		bool found = false;
		lock();
		for(uint32 i = 0; i < _process_list.size(); i++)
		{
			process_record *rec = _process_list[i];
			if(rec->request_index == request_index)
			{
				if(rec->state == process_record::not_yet_processed || rec->state == process_record::process_complete)
				{
					delete rec;
					_process_list.erase(i);
				}
				else
					rec->cancelled = true;
				found = true;
				break;
			}
		}
		unlock();
		return found;
	}
	/// Process one request on this queue -- should periodically check the value of *cancelled and early out if it's set to true.
	virtual void process_request(const byte_buffer_ptr &the_request, byte_buffer_ptr &the_response, bool *cancelled, float *progress) = 0;
protected:
	struct process_record
	{
		enum {
			not_yet_processed,
			in_process,
			process_complete,			
		} state;
		uint32 request_index;
		byte_buffer_ptr request_buffer;
		byte_buffer_ptr response_buffer;
		float progress;
		bool cancelled;
	};
	class thread_queue_thread : public thread
	{
		thread_queue *_thread_queue;
	public:
		thread_queue_thread(thread_queue *q)
		{
			_thread_queue = q;			
		}
		uint32 run()
		{
			_thread_queue->thread_start();
			
			_thread_queue->lock();
			thread_storage &sto = _thread_queue->get_storage();
			sto.set((void *) 0);
			_thread_queue->unlock();
			
			for(;;)
				_thread_queue->dispatch_next_call();
			return 0;			
		}
	};
	
	uint32 _current_index;
	friend class thread_queue_thread;
	/// list of worker threads on this thread_queue
	array<thread *> _threads;
	
	/// list of elements in process
	array<process_record *> _process_list;

	/// Synchronization variable that manages worker threads
	semaphore _semaphore;
	
	/// Internal mutex for synchronizing access to thread call vectors.
	mutex _lock;
	
	/// Storage variable that tracks whether this is the main thread or a worker thread.
	thread_storage _storage;
protected:
	/// Locks the thread_queue for access to member variables.
	void lock() { _lock.lock(); }
	/// Unlocks the thread_queue.
	void unlock() { _lock.unlock(); }
	/// Dispatches the next available worker thread call.  Called internally by the worker threads when they awaken from the semaphore.
	void dispatch_next_call()
	{
		_semaphore.wait();
		lock();
		process_record *the_record = 0;
		for(uint32 i = 0; i < _process_list.size(); i++)
		{
			if(_process_list[i]->state == process_record::not_yet_processed)
			{
				the_record = _process_list[i];
				break;
			}
		}
		if(!the_record)
		{
			unlock();
			return;
		}
		the_record->state = process_record::in_process;
		unlock();
		process_request(the_record->request_buffer, the_record->response_buffer, &(the_record->cancelled), &(the_record->progress));
		the_record->state = process_record::process_complete;
	}
	
	/// helper function to determine if the currently executing thread is a worker thread or the main thread.
	bool is_main_thread() { return (bool) _storage.get(); }
	thread_storage &get_storage() { return _storage; }
	/// called by each worker thread when it starts for subclass initialization of worker threads.
	virtual void thread_start() { }
};
