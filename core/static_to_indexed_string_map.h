class static_to_indexed_string_map
{
	typedef hash_table_flat<static_string, indexed_string> table_type;
	table_type _remap_table;
	indexed_string::table &_string_table;
	public:
	
	static_to_indexed_string_map(indexed_string::table &the_string_table) : _string_table(the_string_table) {}
	
	indexed_string get(static_string the_static_string)
	{
		table_type::pointer p;
		p = _remap_table.find(the_static_string);
		if(p)
		{
			//printf("%s .. found : %s (%x)\n", the_static_string, p.value()->c_str(), p.value()->c_str());
			return *p.value();
		}
		indexed_string the_indexed_string(_string_table, the_static_string);
		p = _remap_table.insert(the_static_string, the_indexed_string);
		//printf("%s .. inserting : %s (%x)\n", the_static_string, p.value()->c_str(), p.value()->c_str());
		//_string_table.dump();
		return the_indexed_string;
	}
};
