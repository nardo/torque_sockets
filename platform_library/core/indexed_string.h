/// the indexed_string class manages string data that is indexed for fast comparison and identification.
/// a note about indexed_string and threads, and which operations are safe: basically each indexed_string object is tied to the thread it was created on.  The c_str() data will work across threads, but any assignment or string/string comparisons will not function correctly, and could cause strange memory issues.  The debug code should help in catching these offenders, but beware -- indexed_strings should be kept local to each thread.

class indexed_string
{
	public:
	class table;

	indexed_string() { _node = table::_get_null_node(); _node->ref_count++; }
	indexed_string(table &the_table, const char8 *string) { _node = the_table.insert(string, strlen(string)); _node->ref_count++; }
	indexed_string(table &the_table, const char8 *buffer, uint32 buffer_len) { _node = the_table.insert(buffer, buffer_len); _node->ref_count++; }
	indexed_string(const indexed_string &s) { _node = s._node; _node->ref_count++; }
	
	~indexed_string() { if(!--_node->ref_count) table::node_reference_to_zero(_node); }
	
	const char8 *c_str() const { return _node->string_data; }
	
	indexed_string &operator=(const indexed_string &the_string) { if(!--_node->ref_count) table::node_reference_to_zero(_node); _node = the_string._node; _node->ref_count++; return *this; }
	bool operator== (const indexed_string &s) const { return compare(s); }
	bool operator!= (const indexed_string &s) const { return !compare(s); }
	
	operator bool() { return _node->case_insensitive_string_id != 0; }
	
	bool compare(const indexed_string & the_string, bool case_sensitive = false) const
	{
		if(the_string._node == _node)
			return true;
		return case_sensitive ? false : the_string._node->case_insensitive_string_id == _node->case_insensitive_string_id;
	}
	
	uint32 hash() const { return _node->case_insensitive_string_id; }
	class table
	{
		friend class indexed_string;
		public:
		table(zone_allocator *allocator) : _block_allocator(allocator, this) { _init(); }
		~table() { _destroy(); }

		void dump()
		{
			printf("\nString table: %d entries, table_size =  %d\n", _entry_count, _table_size);
			for(uint32 i = 0; i < _table_size; i++)
			{
				uint32 count = 0;
				for(node *walk = _hash_table[i]; walk; walk = walk->next_hash)
					count++;
				if(count)
					printf("bucket: %d = %d elements\n", i, count);
				for(node *walk = _hash_table[i]; walk; walk = walk->next_hash)
				{
					printf("%c rfc: %d  id: %d  \"%s\"\n", walk == _hash_table[i] ? '-' : 'x', walk->ref_count, walk->case_insensitive_string_id, walk->string_data);
					uint32 len = strlen(walk->string_data);
					uint32 hash = hash_string_no_case(walk->string_data, len);
					uint32 bucket = hash % _table_size;
					assert(bucket == i); // assert that the string is in the right bucket.
				}
			}
		}
		
		private:
		// the node structure is designed so that all manipulation of the string table and entry references hits only the first 16 bytes of the structure (one cache line), and all string data/len stuff will hit only subsequent cache lines.  64 bit pointers will dork this up.
		
		struct node
		{
			uint32 ref_count;
			uint32 case_insensitive_string_id;
			node *next_hash;
			uint32 hash;
			uint16 string_len;
			char8 string_data[1];
		};
		enum
		{
			max_string_length = small_block_allocator<table>::max_size - sizeof(node),
		};
		
		static node *_get_null_node()
		{
			static node null_node = { max_value_uint32 >> 1, 0, 0, 0, 0, { 0 } };
			return &null_node;
		}
		node **_hash_table;
		
		small_block_allocator<table> _block_allocator;
		uint32 _table_size;
		uint32 _entry_count;
		uint32 _resize_threshold;
		uint32 _current_string_id;
		
		void _init()
		{
			// store this in the block allocator's handy extra pointer.  This way we can get the string table associated with a given node pointer.
			_block_allocator.set_reference_pointer(this);
			_entry_count = 0;
			_current_string_id = 1;
			_table_size = next_larger_hash_prime(3);
			_resize_threshold = _table_size >> 1;
			_hash_table = new node*[_table_size];
			for(uint32 i = 0;i < _table_size; i++)
				_hash_table[i] = 0;
		}
		
		void _destroy()
		{
			delete[] _hash_table;
		}
		
		enum string_compare_result
		{
			not_at_all_equal,
			equal_in_the_case_insensitive_way,
			totally_equal,
		};
		
		static char8 _tolower(char8 c)
		{
			return (c < 'A' || c > 'Z') ? c : c - 'A' + 'a';
		}
		
		static string_compare_result _compare_strings(const char8 *buffer, uint32 buffer_len, const char8 *stored_string)
		{
			string_compare_result result = totally_equal;
			while(buffer_len--)
			{
				char8 c1 = *buffer++;
				char8 c2 = *stored_string++;
				if(!c2)
					return not_at_all_equal;
				if(c1 == c2)
					continue;
				// they don't match, see if they match in the case insenitive way
				if(_tolower(c1) != _tolower(c2))
					return not_at_all_equal;
				result = equal_in_the_case_insensitive_way;
			}
			if(*stored_string)
				return not_at_all_equal;
			else
				return result;
		}
		
		static uint32 hash_string_no_case(const char8 *buffer, uint32 buffer_len)
		{
			uint32 result = 0;
			while(buffer_len--)
			{
				char8 c = _tolower(*buffer++);
				result = ((result << 4) | (result >> 28)) ^ c;
			}
			return result;
		}
		
		node *find(const char8 *buffer, uint32 buffer_len, bool full_match)
		{
			if(buffer_len > max_string_length)
				buffer_len = max_string_length;			
			uint32 hash = hash_string_no_case(buffer, buffer_len);
			uint32 bucket = hash % _table_size;

			for(node *walk = _hash_table[bucket]; walk; walk = walk->next_hash)
			{
				if(walk->hash == hash)
				{
					// the hash values are equal, compare the strings
					string_compare_result val = _compare_strings(buffer, buffer_len, walk->string_data);
					if(val == not_at_all_equal)
						continue;
					else if(full_match && val == totally_equal || !full_match)
						return walk;
				}
			}
			return 0;
		}
		
		static void node_reference_to_zero(node *the_node)
		{
			assert(the_node->ref_count == 0);
			small_block_allocator<table>::get_reference_pointer(the_node)->remove(the_node);
		}
		
		void remove(node *the_node)
		{
			// first remove from the hash table:
			uint32 bucket = the_node->hash % _table_size;
			for(node **walk = &_hash_table[bucket]; *walk; walk = &((*walk)->next_hash))
			{
				if(*walk == the_node)
				{
					*walk = the_node->next_hash;
					break;
				}
			}
			_block_allocator.deallocate(the_node);
			_entry_count--;
		}
		
		node *insert(const char8 *buffer, uint32 buffer_len)
		{
			if(!buffer_len)
				return _get_null_node();
			if(buffer_len > max_string_length)
				buffer_len = max_string_length;
			uint32 hash = hash_string_no_case(buffer, buffer_len);
			uint32 bucket = hash % _table_size;
			uint32 string_id = 0;
			for(node *walk = _hash_table[bucket]; walk; walk = walk->next_hash)
			{
				if(walk->hash == hash)
				{
					// the hash values are equal, compare the strings
					string_compare_result val = _compare_strings(buffer, buffer_len, walk->string_data);
					if(val == not_at_all_equal)
						continue;
					else if(val == equal_in_the_case_insensitive_way)
					{
						string_id = walk->case_insensitive_string_id;
						continue;
					}
					else
						return walk;
				}
			}
			// it wasn't in the table, so add it.  First check if the table needs to be resized:
			
			if(++_entry_count > _resize_threshold)
			{
				uint32 new_table_size = next_larger_hash_prime(_table_size);
				
				// make a new table and reinsert the elements:
				node **new_table = new node*[new_table_size];
				for(uint32 i  =0; i < new_table_size;i ++)
					new_table[i] = 0;
				
				// now loop thru the old hash table, and insert row chains into the new hash table.
				for(uint32 i = 0;i < _table_size; i++)
				{
					for(node *chain = _hash_table[i]; chain;)
					{
						node *next = chain->next_hash;
						uint32 new_index = chain->hash % new_table_size;
						chain->next_hash = new_table[new_index];
						new_table[new_index] = chain;
						chain = next;
					}
				}
				_table_size = new_table_size;
				_resize_threshold = _table_size >> 1;
				delete[] _hash_table;
				_hash_table = new_table;
				bucket = hash % _table_size;
			}
			
			if(string_id == 0)
				string_id = _current_string_id++;
			
			node *new_node = (node *) _block_allocator.allocate(buffer_len + sizeof(node));
			new_node->ref_count = 0;
			new_node->case_insensitive_string_id = string_id;
			new_node->hash = hash;
			new_node->string_len = buffer_len;
			new_node->next_hash = _hash_table[bucket];
			_hash_table[bucket] = new_node;
			memcpy(new_node->string_data, buffer, buffer_len);
			new_node->string_data[buffer_len] = 0;

			return new_node;
		}
	};
	table::node *_node;
};

static void indexed_string_test()
{
	zone_allocator zone;
	indexed_string::table string_table(&zone);
	
	const char *strings[6] = { "Hello, world!", "heLLo, WORld!", "Hello, world!", "A long string, longer than the first.", "a SHorT!", "Foobar!" };
	
	indexed_string *test_strings[6];
	for(uint32 i = 0; i < 6; i++)
	{
		printf("inserting \"%s\":\n", strings[i]);
		test_strings[i] = new indexed_string(string_table, strings[i]);
		string_table.dump();
	}
	printf("Test: %d %d %d %d (expect 1 1 1 0)\n", *(test_strings[0]) == *(test_strings[1]), *(test_strings[0]) == *(test_strings[2]), test_strings[0]->compare(*(test_strings[2]), true), test_strings[0]->compare(*(test_strings[1]), true));
	
	for(uint32 i = 0; i < 6; i++)
	{
		printf("removing \"%s\":\n", strings[i]);
		delete test_strings[i];
		string_table.dump();
	}
};
