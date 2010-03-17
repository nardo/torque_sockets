template<class key_type, class value_type> class hash_table_flat
{
	public:
	class pointer;
	
	hash_table_flat(uint32 initial_size = 0) { _init(initial_size); }
	
	~hash_table_flat() { _clear(); }
	
	uint32 size() { return _entry_count; }
	
	void clear() { _clear(); }
	
	pointer insert(const key_type &key, const value_type &value) { return _insert(key, value); }
	
	pointer insert(const key_type &key) { return _insert(key); }
	
	pointer find(const key_type &key) { return _find(key); }
	
	pointer first() { pointer p; _get_first(p); return p; }
	
	bool remove(const key_type &key)
	{
		pointer p = find(key);
		if(p)
		{
			p.remove();
			return true;
		}
		else
			return false;
	}
	
	class pointer
	{
		public:
		pointer(hash_table_flat *table = 0, uint32 index = 0) { _table = table; _index = index; }

		pointer(const pointer &the_pointer) { _table = the_pointer._table; _index = the_pointer._index; }

		pointer &operator++() { _index = _table->_increment(_index); return *this; }
		
		const key_type *key() { return _table->_key(_index); }
		
		value_type *value() { return _table->_value(_index); }
		
		bool remove() { return _table->_remove(_index); }
		
		operator bool() { return hash_table_flat::_is_valid(_table, _index); }
		
		uint32 index() { return _index; }

	// ------------------------------------------------------------------
	// implementation
		
		private:
		hash_table_flat *_table;
		uint32 _index;
	};
	private:
	friend class pointer;
	struct entry
	{
		bool is_valid;
		key_type key;
		value_type value;
	};
	entry *_table;
	uint32 _entry_count;
	uint32 _table_size;
	uint32 _resize_threshold;
	
	// implementation
	static void _alloc_table(entry **table_ptr, uint32 size)
	{
		entry *tb = (entry *) memory_allocate(size * sizeof(entry));
		for(uint32 i = 0; i < size; i++)
			tb[i].is_valid = 0;
		*table_ptr = tb;
	}
	
	static bool _is_valid(hash_table_flat *tb, uint32 index)
	{
		if(!tb)
			return false;
		return index < tb->_table_size && tb->_table[index].is_valid;
	}

	void _init(uint32 initial_size)
	{
		_table_size = initial_size;
		_entry_count = 0;
		_resize_threshold = 0;
		if(initial_size)
			_alloc_table(&_table, initial_size);
		else
			_table = 0;
	}
	
	void _clear()
	{
		if(!has_trivial_destructor<key_type>::is_true ||
		   !has_trivial_destructor<value_type>::is_true)
		{
			for(uint32 i = 0; i < _table_size; i++)
			{
				if(_table[i].is_valid)
				{
					destroy(&_table[i].key);
					destroy(&_table[i].value);
				}
			}
		}
		memory_deallocate(_table);
		_table = 0;
		_entry_count = 0;
		_table_size = 0;
		_resize_threshold = 0;
	}
	
	pointer _insert(const key_type &key, const value_type &value)
	{
		pointer p = _insert_without_value_construction(key);
		construct(p.value(), value);
		return p;
	}
	
	pointer _insert(const key_type &key)
	{
		pointer p = _insert_without_value_construction(key);
		construct(p.value());
		return p;
	}
	
	static uint32 _insert_in_table(const key_type &key, entry *table, uint32 table_size)
	{
		looping_counter bucket(hash(key), table_size);

		while(table[bucket].is_valid)
 			++bucket;

		construct(&table[bucket].key, key);
		table[bucket].is_valid = true;
		return bucket;
	}
	
	pointer _insert_without_value_construction(const key_type &key)
	{
		if(_entry_count >= _resize_threshold)
		{
			uint32 new_size = next_larger_hash_prime(_table_size);
			uint32 new_threshold = new_size >> 1;
			assert(_entry_count < new_threshold);
			
			entry *new_table;
			_alloc_table(&new_table, new_size);
			for(uint32 i = 0; i < _table_size; i++)
			{
				if(_table[i].is_valid)
				{
					uint32 index = _insert_in_table(_table[i].key, new_table, new_size);
					construct(&new_table[index].value, _table[i].value);
				}
			}
			uint32 saved_count = _entry_count;
			_clear();
			_entry_count = saved_count;
			_table_size = new_size;
			_resize_threshold = new_threshold;
			_table = new_table;
		}
		uint32 insert_index = _insert_in_table(key, _table, _table_size);
		_entry_count++;
		return pointer(this, insert_index);
	}
	
	bool _remove(uint32 remove_index)
	{
		if(remove_index >= _table_size || !_table[remove_index].is_valid)
			return false;
		
		// first kill the one to remove
		destroy(&_table[remove_index].key);
		destroy(&_table[remove_index].value);
		_table[remove_index].is_valid = false;
		_entry_count--;
		
		// now check that subsequent entries in the hash table are not overflow -- if they are, reinsert them in the table.  As soon as an empty bucket is found, the search for overflow terminates.
		
		looping_counter bucket(remove_index, _table_size);
		for(;;)
		{
			++bucket;

			if(!_table[bucket].is_valid)
				break;
			looping_counter correct_bucket(hash(_table[bucket].key), _table_size);
			if(correct_bucket == bucket) // it's supposed to be here!
				continue;
			
			// otherwise, figure out the reinsert index
			while(_table[correct_bucket].is_valid && correct_bucket != bucket)
				++correct_bucket;
			
			// if the search netted the same bucket, skip out
			if(correct_bucket == bucket)
				continue;
			
			// now move the element to its correct bucket.
			construct(&_table[correct_bucket].key, _table[bucket].key);
			construct(&_table[correct_bucket].value, _table[bucket].value);
			_table[correct_bucket].is_valid = true;
			destroy(&_table[bucket].key);
			destroy(&_table[bucket].value);
			_table[bucket].is_valid = false;
		}
		
		return true;
	}
	
	pointer _find(const key_type &key)
	{
		if(!_table_size)
			return pointer(this, 0);
		looping_counter bucket(hash(key), _table_size);
		
		while(_table[bucket].is_valid)
		{
			if(_table[bucket].key == key)
				return pointer(this, bucket);
			++bucket;
		}
		return pointer(this, _table_size);
	}
	
	uint32 _increment(uint32 index)
	{
		while(++index < _table_size)
			if(_table[index].is_valid)
			break;
		
		return index;
	}
	
	const key_type *_key(uint32 index)
	{
		return (index < _table_size && _table[index].is_valid) ? &_table[index].key : 0;
	}
	
	value_type *_value(uint32 index)
	{
		return (index < _table_size && _table[index].is_valid) ? &_table[index].value : 0;
	}
		
	void _get_first(pointer &p)
		{
			uint32 index = 0;
			while(index < _table_size && !_table[index].is_valid)
				index++;
			p = pointer(this,index);
		}
};

static void hash_table_flat_test()
{
	printf("---- hash_table_flat unit test: ----\n");
	hash_table_tester<hash_table_flat<int, const char *> >::run(true);
}
