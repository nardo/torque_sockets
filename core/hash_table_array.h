/// the hash_table_array class is an array/hash table hybrid.  The array portion is used for relative ordering and indexing, and the hash table part is used for fast lookups by key.  If multiple items in the table share the same key, the find()/next_match() methods will return them in insert order.

template <typename key_type, typename value_type> class hash_table_array
{
	struct table;
	
	public:
	class pointer;
	friend class pointer;
	hash_table_array() { _table = table::get_null(); }
	~hash_table_array() { table::deallocate(_table); }
	
	pointer first() { return pointer(_table, 0); }
	pointer last() { return pointer(_table, _table->count != 0 ? _table->count-1 : 0); }
	pointer find(const key_type &key) { return pointer(_table, _table->find(key)); }
	uint32 size() { return _table->count; }
	void reserve(uint32 count) { _check_size(count); }
	pointer operator[] (uint32 index) { return pointer(_table, index); }
	pointer insert(const key_type &the_key, const value_type &the_value, uint32 index = max_value_uint32) { return _insert(the_key, the_value, index); }
	pointer insert_unique(const key_type &key, const value_type &value) { pointer p = find(key); if(p) return pointer(); else return insert(key, value); }
	
	class pointer
	{
		public:
		pointer() { _init(table::get_null(), 0); }
		pointer(table *table, uint32 index) { _init(table, index); }
		pointer(const pointer &the_pointer) { _init(the_pointer._table, the_pointer._index); }
		pointer &operator=(const pointer &p) { _init(p._table, p._index); return *this; }
		const key_type *key() { return _index < _table->count ? &_table->array[_index]->key : 0; }
		value_type *value() { return _index < _table->count ? &_table->array[_index]->value : 0; }
		const value_type *value() const { return _index < _table->count ? &_table->array[_index]->value : 0; }
		pointer &next() { _index++; return *this; }
		pointer &previous() { _index--; return *this; }
		pointer &operator++() { return next(); }
		bool is_valid() { return _index < _table->count; }
		operator bool() { return is_valid(); }
		uint32 index() { return _index; }
		void move_back(uint32 count = 1) { _table->move_item_back(_index, count); }
		void move_forward(uint32 count = 1) { _table->move_item_forward(_index, count); }
		void set_index(uint32 index) { _table->set_item_index(_index, index); }
		void set_key(const key_type &new_key) { _table->set_item_key(_index, new_key); }
		pointer &next_match() { _index = _table->next_match(_index); return *this; }
		void remove() { _table->remove(_index); }
		
		// ---- IMPLEMENTATION -------------
		private:
#ifdef CORE_DEBUG
		key_type *_key;
		value_type *_value;
#endif
		table *_table;
		uint32 _index;
		
		void _init(table *the_table, uint32 the_index)
		{
			_table = the_table;
			_index = the_index;
#ifdef CORE_DEBUG
			_key = 0;
			_value = 0;
#endif
		}
	};
	private:

	struct node
	{
		node(const key_type &the_key, const value_type &the_value) : key(the_key), value(the_value) {}
		
		node *next_hash;
		uint32 index;
		uint32 hash;
		uint32 hash_index;
		key_type key;
		value_type value;
	};

	struct table
	{
		uint32 table_size;
		uint32 count;
		node **hash_table;
		node *array[1];
		
		static table *get_null()
		{
			static table the_table = { 0, 0, 0, 0 };
			return &the_table;			
		}
		
		static table *allocate(uint32 num_elements)
		{
			uint32 size = next_larger_hash_prime(num_elements);
			table *ret = (table *) memory_allocate(sizeof(table) + sizeof(node *) * (size * 2 - 1));
			ret->table_size = size;
			ret->count = 0;
			ret->hash_table = ret->array + size;
			for(uint32 i = 0; i < size * 2; i++)
				ret->array[i] = 0;
			return ret;
		}
		
		static void deallocate(table *the_table)
		{
			if(the_table->table_size != 0)
				memory_deallocate(the_table);
		}
		
		void remove_from_hash(node *the_node)
		{
			node **walk = &hash_table[the_node->hash_index];
			while(*walk != the_node)
				walk = &((*walk)->next_hash);
			*walk = the_node->next_hash;
		}
		
		void insert_in_hash(node *the_node)
		{
			the_node->hash = hash(the_node->key);
			the_node->hash_index = the_node->hash % table_size;
			
			node **walk = &hash_table[the_node->hash_index];
			while(*walk && (*walk)->index < the_node->index)
				walk = &((*walk)->next_hash);
			
			the_node->next_hash = *walk;
			*walk = the_node;
		}
		
		void move_item_back(uint32 index, uint32 back_count)
		{
			assert(index < count);
			if(index >= count)
				return;
			
			uint32 next_index = index + back_count;
			
			if(count <= next_index)
				return;
			
			// first move the items in the array portion
			node *the_node = array[index];
			for(uint32 i = index; i < next_index; i++)
			{
				node *renum = array[i+1];
				assert(renum->index == i + 1);
				array[i] = renum;
				renum->index = i;
			}
			array[next_index] = the_node;
			the_node->index = next_index;
			index = next_index;
			
			// second, move it in the hash table so that hash iteration will be in index order.
			if(the_node->next_hash && the_node->next_hash->index < next_index)
			{
				node **walk = &hash_table[the_node->hash_index];
				while(*walk != the_node)
					walk = &((*walk)->next_hash);
				
				*walk = the_node->next_hash;
				walk = &((*walk)->next_hash);
				
				while(*walk && (*walk)->index < next_index)
					walk = &((*walk)->next_hash);
				
				the_node->next_hash = *walk;
				*walk = the_node;
			}
		}
		
		void move_item_forward(uint32 index, uint32 forward_count)
		{
			assert(index < count);
			if(index >= count)
				return;
			
			uint32 next_index = index - count;
			node *the_node = array[index];
			
			// first, shift the items in the array.
			for(uint32 i = index; i > next_index; i--)
			{
				node *renum = array[i-1];
				assert(renum->index == i - 1);
				array[i] = renum;
				renum->index = i;
			}
			
			array[next_index] = the_node;
			the_node->index = next_index;
			index = next_index;
			
			// second, shift the items in the hash table portion so that hash iteration will be in order.
			
			node *remainder = the_node->next_hash;
			node **walk = &hash_table[the_node->hash_index];
			
			while((*walk)->index < next_index)
				walk = &((*walk)->next_hash);
			
			if(*walk == the_node)
				return;
            
			// splice the node in at walk.
			the_node->next_hash = *walk;
			*walk = the_node;
			node *search = the_node->next_hash;
			while(search->next_hash != the_node)
				search = search->next_hash;
			
			search->next_hash = remainder;
		}
		
		void set_item_index(uint32 item_index, uint32 new_index)
		{
			assert(item_index < count);
			if(new_index > item_index)
				move_item_back(item_index, new_index - item_index);
			else if(new_index < item_index)
				move_item_forward(item_index, item_index - new_index);
		}
		
		void set_item_key(uint32 index, const key_type &new_key)
		{
			assert(index < count);
			node *the_node = array[index];
			remove_from_hash(the_node);
			the_node->key = new_key;
			insert_in_hash(the_node);
		}
		
		uint32 next_match(uint32 index)
		{
			assert(index < count);
			node *the_node = array[index];
			node *walk = the_node->next_hash;
			while(walk && walk->key != the_node->key)
				walk = walk->next_hash;
			if(walk)
				return walk->index;
			else
				return count;
		}
		
		void remove(uint32 index)
		{
			assert(index < count);
			node *the_node = array[index];
			remove_from_hash(the_node);

			for(uint32 i = index; i < count - 1; i++)
			{
				array[i] = array[i+1];
				array[i]->index = i;
			}
			count--;
			delete the_node;
		}
		uint32 find(const key_type &key)
		{
			if(!table_size)
				return 0;
			
			uint32 hash_key = hash(key);
			uint32 index = hash_key % table_size;
			for(node *walk = hash_table[index]; walk; walk = walk->next_hash)
				if(walk->key == key)
					return walk->index;
			return table_size;
		}
		uint32 insert(node *the_node)
		{
			assert(count < table_size);
			the_node->index = count;
			array[count] = the_node;
			count++;
			insert_in_hash(the_node);
			return the_node->index;
		}
	};
	table *_table;

	void _check_size(uint32 reserve_size)
	{
		if(_table->table_size <= reserve_size)
		{
			// it's time to reallocate the table
			table *old_table = _table;
			table *new_table = table::allocate(reserve_size);
			uint32 new_table_size = new_table->table_size;
			for(int32 i = int32(old_table->count) - 1; i >= 0; i--)
			{
				// traverse the array in reverse index order, inserting into the front of the new hash table.  This preserves forward index ordering in the hash chains.
				node *n = old_table->array[i];
				n->hash_index = n->hash % new_table_size;
				n->next_hash = new_table->hash_table[n->hash_index];
				new_table->hash_table[n->hash_index] = n;
				new_table->array[i] = n;
			}
			new_table->count = old_table->count;
			_table = new_table;
			table::deallocate(old_table);
		}
	}
	pointer _insert(const key_type &the_key, const value_type &the_value, uint32 index)
	{
		_check_size(_table->count + 1);
		node *new_node = new node(the_key, the_value);
		pointer ret(_table, _table->insert(new_node));
		
		if(index < _table->count)
			ret.set_index(index);
		return ret;
	}
};

static void hash_table_array_test()
{
	printf("---- hash_table_array unit test: ----\n");
	hash_table_tester<hash_table_array<int, const char *> >::run();
}
