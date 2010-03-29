// A zone_allocator is a memory allocation pool object.  The zone object manages an allocation quota and will return null for allocation requests if the quota is met.  zone allocations can be either by fixed "page" sized chunks, or of variable size.  inter-zone page sharing may be added later, or it may be factored into its own seperate lower level page allocator.

class zone_allocator
{
	public:
	enum {
		default_quota = 0,
		page_size = 4096,
	};

	zone_allocator(uint32 quota = default_quota)
	{
		_quota = quota;
		_memory_used = 0;
	}
	
	~zone_allocator()
	{
		assert(_memory_used == 0);
	}

	// currently we allocate pages one at a time from the OS VM.  A future optimization could allocate these in chunks... I'm not sure there would be any real net benefit, since the first access to a page is going to cause a page fault anyway.
	
	void *allocate_pages(uint32 page_count)
	{
		uint32 allocation_size = page_count * page_size;
		if(_quota && (_memory_used + allocation_size > _quota))
			return 0;
		
#ifdef PLATFORM_MAC_OSX
		void* data;
		kern_return_t err;
		
		// Allocate directly from mach VM
		err = vm_allocate((vm_map_t) mach_task_self(), (vm_address_t*) &data, allocation_size, VM_FLAGS_ANYWHERE);
		
		assert(err == KERN_SUCCESS);
		if(err != KERN_SUCCESS)
			data = NULL;
		else
			_memory_used += allocation_size;
		return data;		
#else
		return malloc(allocation_size);
#endif
	}
	
	void deallocate_pages(void *the_page, uint32 page_count)
	{
#ifdef PLATFORM_MAC_OSX
		kern_return_t err;
		err = vm_deallocate((vm_map_t) mach_task_self(), (vm_address_t) the_page, page_size * page_count);

		assert(err == KERN_SUCCESS);
		if(err == KERN_SUCCESS)
			_memory_used -= page_size * page_count;
#else
		free(the_page);
#endif
	}
	private:
	uint32 _quota;
	uint32 _memory_used;
};
