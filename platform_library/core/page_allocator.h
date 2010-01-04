template <uint32 alignment = 4> class page_allocator
{
public:
	page_allocator(zone_allocator *zone)
	{
		_allocator = zone;

		_current_offset = zone_allocator::page_size;
		_current_page_size = zone_allocator::page_size;
		
		_current_page = 0;
	}

	~page_allocator()
	{
		clear();
	}
	
	virtual void clear()
	{
		while(_current_page)
		{
			page *prev = _current_page->prev;
			_allocator->deallocate_pages(_current_page,_current_page->page_count);
			_current_page = prev;
		}	
	}
	
	void *allocate(uint32 size)
	{
		if(!size)
			return 0;

		// align size:
		size = (size + (alignment - 1)) & ~(alignment - 1);
		
		if(_current_offset + size > _current_page_size)
		{
			// need a new page to fit this allocation:
			uint32 new_page_count = (max(uint32(zone_allocator::page_size), size + page_header_size) + zone_allocator::page_size - 1) / zone_allocator::page_size;
			
			page *new_page = (page *) _allocator->allocate_pages(new_page_count);
			new_page->allocator = this;
			new_page->page_count = new_page_count;
			
			uint32 new_size = new_page_count * zone_allocator::page_size;
			uint32 new_offset = page_header_size + size;
			
			if(new_size - new_offset >= _current_page_size - _current_offset)
			{
				// if the new page has equal or more free space than the current page, make the new page the current page.
				new_page->prev = _current_page;
				_current_page = new_page;
				_current_page_size = new_size;
				_current_offset = new_offset;
			}
			else
			{
				// otherwise, link the new page in _after_ the current page
				new_page->prev = _current_page->prev;
				_current_page->prev = new_page;
			}
			return ((uint8 *) new_page) + page_header_size;
		}
		else
		{
			void *ret = ((uint8 *) _current_page) + _current_offset;
			_current_offset += size;
			return ret;
		}
	}
private:
	struct page
	{
		page_allocator *allocator;
		page *prev;
		uint32 page_count;
	};
	uint32 _current_offset;
	uint32 _current_page_size;
	zone_allocator *_allocator;
	page *_current_page;
	uint32 _alignment;
	
public:
	enum {
		page_header_size = (sizeof(page) + (alignment - 1)) & ~(alignment - 1),
		maximum_single_page_allocation_size = zone_allocator::page_size - page_header_size,
	};
};
