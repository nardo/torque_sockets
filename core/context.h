struct context
{
	public:
	context() : _small_block_allocator(&_zone_allocator, this), _string_table(&_zone_allocator) {
	} //}, _pile(&_zone_allocator) {}
	
	zone_allocator &get_zone_allocator() { return _zone_allocator; }
	small_block_allocator<context> &get_small_block_allocator() { return _small_block_allocator; }
	indexed_string::table &get_string_table() { return _string_table; }
	//pile *get_pile() { return &_pile; }
	
	private:
	//pile _pile;
	zone_allocator _zone_allocator;
	small_block_allocator<context> _small_block_allocator;
	indexed_string::table _string_table;
};