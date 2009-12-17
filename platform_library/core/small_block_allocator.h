// small_block_allocator is an allocator for small blocks.  All blocks allocated by the small_block_allocator are cache-line aligned to cpu_cache_line_size.  For the initial implementation, cpu_cache_line_size is defined to be 16 bytes.  The maximum size of an allocation is max_size, which is defined to be 1536 bytes.  There are 256 total cache lines in each page -- so each page reserves 256 bytes at the start (1/32 memory management overhead) for a byte sized bucket index for that block.

template <typename block_reference_type = void> class small_block_allocator
{
	public:
	small_block_allocator(zone_allocator *allocator, block_reference_type *reference_pointer = 0) { _reference_pointer = reference_pointer; _allocator = allocator; _init(); }
	
	~small_block_allocator() { _destroy(); }
	
	void clear() { _destroy(); _init(); }
	
	void *allocate(uint32 num_bytes) { return _allocate(num_bytes); }
	
	static void deallocate(void *memory_block) { _deallocate(memory_block); }
	
	void set_reference_pointer(block_reference_type *pointer) { _reference_pointer  = pointer; }
	
	static block_reference_type *get_reference_pointer(void *memory) { return _find_page(memory)->reference_pointer; }
	
	enum {
		cache_line_size = 16,
		cache_line_mask = cache_line_size - 1,
		cache_line_shift = 4,
		page_size = zone_allocator::page_size,
		page_mask = page_size - 1,
		page_shift = 12,
		cache_lines_per_page = page_size / cache_line_size,
		max_cache_line_size = 96,
		max_size = max_cache_line_size * cache_line_size, // 1536
		bucket_index_mask = 0x1f,
		flag_allocated = 32,
		flag_invalid = 64,
		flag_freed = 128,
		num_memory_status_blocks_per_block_page = 16,
		num_buckets = 18,
	};
	private:
	// the small_block_allocator allocates 2 different kinds of pages - data pages and memory status pages.  Each memory status page holds 16 256-byte status blocks, and so will be referenced by 16 different pages.  Whichever data page has a pointer to the block at the start of the page will be responsible for freeing the page.
	
	struct free_memory_block
	{
		free_memory_block *next;
		byte *status_byte_ptr;
	};
	struct page
	{
		byte *memory_status_block;
		page *prev;
		free_memory_block **free_buckets;
		block_reference_type *reference_pointer;
	};
	
	block_reference_type *_reference_pointer;
	
	byte *_current_memory_status_block_page;
	uint32 _num_memory_status_blocks_used;
	page *_top_page;
	uint32 _cache_lines_used_in_top_page;
	free_memory_block *_free_buckets[num_buckets];
	zone_allocator *_allocator;
	
	void *_allocate(uint32 size)
	{
		// assert in debug builds
		assert(size <= max_size);
		if(size > max_size) // and if asserts aren't in, crash and burn in the outer code
			return 0;
		
		// compute the number of cache_lines for size bytes:
		uint32 line_count = (size + cache_line_mask) >> cache_line_shift;
		
		uint32 bucket_index, bucket_size;
		
		_find_bucket(line_count, bucket_index, bucket_size);
		assert(bucket_index < num_buckets);
		
		// simplest case: there's already a free one ready:
		if(_free_buckets[bucket_index])
		{
			free_memory_block *ret = _free_buckets[bucket_index];
			_free_buckets[bucket_index] = ret->next;
			uint8 status = *ret->status_byte_ptr;
			assert(status & flag_freed);
			*ret->status_byte_ptr = (status & bucket_index_mask) | flag_allocated;
			return ret;
		}
		
		// otherwise, allocate it from the top page.  If there's not enough room
		// in the top page, allocate a new one:
		if(_cache_lines_used_in_top_page + bucket_size > cache_lines_per_page)
			if(!_alloc_new_top_page())
			return 0;
		
		// mark the range as allocated in the top page's status block:
		_top_page->memory_status_block[_cache_lines_used_in_top_page] = uint8(bucket_index | flag_allocated);
		void *ptr = ((uint8 *) _top_page) + (_cache_lines_used_in_top_page << cache_line_shift);
		_cache_lines_used_in_top_page += bucket_size;
		
		return ptr;
	}
	
	static bool _is_page_aligned(void *memory)
	{
		return (uint32(memory) & page_mask) == 0;
	}
	
	static page *_find_page(void *memory)
	{
		uint32 offset = uint32(memory) & page_mask;
		page *ret = (page *) ((uint8 *) memory - offset);
		return ret;
	}
	
	static void _deallocate(void *memory)
	{
		page *the_page = _find_page(memory);
		uint32 cache_line = ((uint8 *) memory - (uint8 *) the_page) >> cache_line_shift;
		uint8 status = the_page->memory_status_block[cache_line];
		assert(status & flag_allocated);
		uint8 bucket_index = status & bucket_index_mask;
		free_memory_block *block = (free_memory_block *) memory;
		block->status_byte_ptr = the_page->memory_status_block + cache_line;
		block->next = the_page->free_buckets[bucket_index];
		the_page->free_buckets[bucket_index] = block;
		*block->status_byte_ptr = bucket_index | flag_freed;
	}
	
	void _init()
	{
		_current_memory_status_block_page = 0;
		_top_page = 0;
		_num_memory_status_blocks_used = num_memory_status_blocks_per_block_page;
		_cache_lines_used_in_top_page = cache_lines_per_page;
		
		for(uint32 i = 0; i < num_buckets; i++)
			_free_buckets[i] = 0;
	}
	
	bool _alloc_new_top_page()
	{
		page *new_page = (page *) _allocator->allocate_pages(1);
		if(!new_page)
			return false;
		
		// see if there are any free status blocks:
		if(_num_memory_status_blocks_used == num_memory_status_blocks_per_block_page)
		{
			_num_memory_status_blocks_used = 0;
			_current_memory_status_block_page = (byte *) _allocator->allocate_pages(1);
			if(!_current_memory_status_block_page)
			{
				_allocator->deallocate_pages(new_page, 1);
				return false;
			}
			memset(_current_memory_status_block_page, flag_invalid, page_size);
		}
		new_page->prev = _top_page;
		_top_page = new_page;
		
		new_page->reference_pointer = _reference_pointer;
		new_page->memory_status_block = _current_memory_status_block_page;
		_current_memory_status_block_page += cache_lines_per_page;
		_num_memory_status_blocks_used++;
		new_page->free_buckets = _free_buckets;
		
		// now mark as allocated as many cache lines as cover the page structure:
		_cache_lines_used_in_top_page = (sizeof(page) + cache_line_mask) >> cache_line_shift;
		return true;
	}
	
	void _destroy()
	{
		while(_top_page)
		{
			// check for deallocating the status block:
			if(_is_page_aligned(_top_page->memory_status_block))
				_allocator->deallocate_pages(_top_page->memory_status_block, 1);
			page *temp = _top_page->prev;
			_allocator->deallocate_pages(_top_page, 1);
			_top_page = temp;
		}
	}
	
	static void _find_bucket(uint32 cache_line_count, uint32 &out_bucket_index, uint32 &out_bucket_size)
	{
		static uint32 bucket_sizes[] = { 1, 2, 3, 4, 5, 6, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64, 80, max_cache_line_size, max_value_uint32 };
		
		uint32 i = 0;
		while(bucket_sizes[i] < cache_line_count)
			i++;
		out_bucket_index = i;
		out_bucket_size = bucket_sizes[i];
	}
};

static void small_block_allocator_test()
{
	printf("---- small_block_allocator unit test: ----\n");

	zone_allocator the_allocator;
	small_block_allocator<> *a = new small_block_allocator<>(&the_allocator);
	
	void *block = a->allocate(100);
	a->deallocate(block);
	block = a->allocate(100);
	a->deallocate(block);
	
	delete a;
}