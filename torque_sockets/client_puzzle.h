class puzzle_solver : public thread_queue
{
public:
	puzzle_solver() : thread_queue(1) { }
	void process_request(const byte_buffer_ptr &the_request, byte_buffer_ptr &the_response, bool *cancelled, float *progress)
	{
		nonce the_nonce;
		nonce remote_nonce;
		uint32 puzzle_difficulty;
		uint32 client_identity;
		bit_stream s(the_request->get_buffer(), the_request->get_buffer_size());
		core::read(s, the_nonce);
		core::read(s, remote_nonce);
		core::read(s, puzzle_difficulty);
		core::read(s, client_identity);
		uint32 solution = 0;
		time start = time::get_current();
		for(;;)
		{
			if(*cancelled)
				break;
			if(client_puzzle_manager::check_one_solution(solution, the_nonce, remote_nonce, puzzle_difficulty, client_identity))
				break;
			
			++solution;
		}
		if(!*cancelled)
		{
			byte_buffer_ptr response = new byte_buffer(sizeof(solution));
			bit_stream s(response->get_buffer(), response->get_buffer_size());
			core::write(s, solution);
			the_response = response;
			TorqueLogMessageFormatted(LogNettorque_socket, ("Client puzzle solved in %lli ms.", (time::get_current() - start).get_milliseconds()));
		}
	}
};

/// The client_puzzle_manager class issues, solves and validates client
/// puzzles for connection authentication.
class client_puzzle_manager
{
public:
private:
	/// nonce_table manages the list of client nonces for which clients
	/// have constructed valid puzzle solutions for the current server
	/// nonce.  There are 2 nonce tables in the client_puzzle_manager -
	/// one for the current nonce and one for the previous nonce.

	class nonce_table : ref_object {
		private:
		struct entry {
			nonce _nonce;
			entry *_hash_next;
		};
		enum {
			hash_table_size = 387,
		};

		entry *_hash_table[hash_table_size];
		page_allocator<8> _allocator;

		public:
		/// nonce_table constructor
		nonce_table(zone_allocator *the_zone_allocator) : _allocator(the_zone_allocator) { reset(); }

		/// Resets and clears the nonce table
		void reset()
		{
			_allocator.clear();
			for(uint32 i = 0; i < hash_table_size; i++)
			_hash_table[i] = NULL;
		}

		/// checks if the given nonce is already in the table and adds it
		/// if it is not.  Returns true if the nonce was not in the table
		/// when the function was called.
		bool check_add(nonce &the_nonce)
		{
			uint32 hash_index = uint32(the_nonce % hash_table_size);
			for(entry *walk = _hash_table[hash_index]; walk; walk = walk->_hash_next)
				if(walk->_nonce == the_nonce)
					return false;
			entry *new_entry = (entry *) _allocator.allocate(sizeof(entry));
			new_entry->_nonce = the_nonce;
			new_entry->_hash_next = _hash_table[hash_index];
			_hash_table[hash_index] = new_entry;
			return true;
		}
	};

	uint32 _current_difficulty;
	time _last_update_time;
	time _last_tick_time;

	nonce _current_nonce;
	nonce _last_nonce;

	nonce_table *_current_nonce_table;
	nonce_table *_last_nonce_table;
	public:
	client_puzzle_manager(random_generator &random_gen, zone_allocator *zone)
	{
		_current_difficulty = initial_puzzle_difficulty;
		random_gen.random_buffer((uint8 *) &_current_nonce, sizeof(nonce));
		random_gen.random_buffer((uint8 *) &_last_nonce, sizeof(nonce));

		_current_nonce_table = new nonce_table(zone);
		_last_nonce_table = new nonce_table(zone);
		_last_tick_time = time::get_current();
	}

	~client_puzzle_manager()
	{
		delete _current_nonce_table;
		delete _last_nonce_table;
	}

	/// Checks to see if a new nonce needs to be created, and if so
	/// generates one and tosses out the current list of accepted nonces
	void tick(time currentTime, random_generator &random_gen)
	{
		// use delta of last tick time and current time to manage puzzle
		// difficulty.

		// not yet implemented.

		// see if it's time to refresh the current puzzle:
		time timeDelta = currentTime - _last_update_time;
		if(timeDelta > time(puzzle_refresh_time))
		{
			_last_update_time = currentTime;
			_last_nonce = _current_nonce;
			nonce_table *tempTable = _last_nonce_table;
			_last_nonce_table = _current_nonce_table;
			_current_nonce_table = tempTable;

			_last_nonce = _current_nonce;
			_current_nonce_table->reset();
			random_gen.random_buffer((uint8 *) &_current_nonce, sizeof(nonce));
		}
	}


	/// Error codes that can be returned by check_solution
	enum result_code {
		success,
		invalid_solution,
		invalid_server_nonce,
		invalid_client_nonce,
		invalid_puzzle_difficulty,
		error_code_count,
	};

	/// Difficulty levels of puzzles
	enum {
		puzzle_refresh_time          = 30000, ///< Refresh the server puzzle every 30 seconds
		initial_puzzle_difficulty    = 17, ///< Initial puzzle difficulty is set so clients do approx 2-3x the shared secret
		///  generation of the server
		max_puzzle_difficulty        = 26, ///< Maximum puzzle difficulty is approx 1 minute to solve on ~2004 hardware.
		max_solution_compute_fragment = 30, ///< Number of milliseconds spent computing solution per call to solve_puzzle.
		solution_fragment_iterations = 50000, ///< Number of attempts to spend on the client puzzle per call to solve_puzzle.
	};

	/// Checks a puzzle solution submitted by a client to see if it is a valid solution for the current or previous puzzle nonces
	result_code check_solution(uint32 solution, nonce &client_nonce, nonce &server_nonce, uint32 puzzle_difficulty, uint32 client_identity)
	{
		if(puzzle_difficulty != _current_difficulty)
			return invalid_puzzle_difficulty;
		nonce_table *the_table = NULL;
		if(server_nonce == _current_nonce)
			the_table = _current_nonce_table;
		else if(server_nonce == _last_nonce)
			the_table = _last_nonce_table;
		if(!the_table)
			return invalid_server_nonce;
		if(!check_one_solution(solution, client_nonce, server_nonce, puzzle_difficulty, client_identity))
			return invalid_solution;
		if(!the_table->check_add(client_nonce))
			return invalid_client_nonce;
		return success;
	}

	static bool check_one_solution(uint32 solution, nonce &client_nonce, nonce &server_nonce, uint32 puzzle_difficulty, uint32 client_identity)
	{
		uint8 buffer[24];
		write_uint32_to_buffer(solution, buffer);
		write_uint32_to_buffer(client_identity, buffer + 4);
		write_uint64_to_buffer(client_nonce, buffer + 8);
		write_uint64_to_buffer(server_nonce, buffer + 16);
		
		hash_state hash_state;
		uint8 hash[32];
		
		sha256_init(&hash_state);
		sha256_process(&hash_state, buffer, sizeof(buffer));
		sha256_done(&hash_state, hash);
		
		uint32 index = 0;
		while(puzzle_difficulty > 8)
		{
			if(hash[index])
				return false;
			index++;
			puzzle_difficulty -= 8;
		}
		uint8 mask = 0xFF << (8 - puzzle_difficulty);
		return (mask & hash[index]) == 0;
	}

	/// Returns the current server nonce
	nonce get_current_nonce() { return _current_nonce; }

	/// Returns the current client puzzle difficulty
	uint32 get_current_difficulty() { return _current_difficulty; }
};
