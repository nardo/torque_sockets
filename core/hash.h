static uint32 next_larger_hash_prime(uint32 number)
{
	assert(number < 4294967291ul);
	
	static uint32 primes[] = { 5, 11, 23, 53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741, 3221225473ul, 4294967291ul, };
	
	uint32 *walk = primes;

	while(number >= *walk)
		walk++;
	
	return *walk;
};

class looping_counter
{
	public:
	looping_counter(uint32 initial_value, uint32 count) { assert(count > 0); _index = ((initial_value < count) ? 0 : (initial_value % count)); _count = count; }
	
	looping_counter &operator=(uint32 value) { _index = value % _count; return *this; }
	looping_counter &operator++() { _index++; if(_index >= _count) _index = 0; return *this; }

	operator uint32() { return _index; }
	
	private:
	uint32 _index;
	uint32 _count;
};

template <typename type> inline uint32 hash(const type &x)
{
	return x.hash();
}

template <typename type> inline uint32 hash(const type *x)
{
	return x->hash();
}

uint32 hash_buffer(const void *buffer, uint32 len)
{
	uint8 *buf = (uint8 *) buffer;
	uint32 result = 0;
	while(len--)
		result = ((result << 8) | (result >> 24)) ^ uint32(*buf++);
	return result;
}
template <typename signature> uint32 hash_method(signature the_method)
{
	return hash_buffer((void *) &the_method, sizeof(the_method));
}

#define numeric_hash_overload(type) template<> inline uint32 hash<type>(const type &x) { return uint32(x); }

numeric_hash_overload(int)
numeric_hash_overload(uint32)

template<> inline uint32 hash(const char *x) { return uint32(x); }
//template<class type> uint32 hash(const type *x) { return uint32(x); }
template<class table> class hash_table_tester
{
	public:
	static void dump_table(table &the_table)
	{
		typename table::pointer p;
		printf("table has %d elements:\n", the_table.size());
		for(p = the_table.first(); p; ++p)
			printf("table element[%d]: %d -> %s\n", p.index(), *(p.key()), *(p.value()));
	}
	
	static void run(bool verbose = false)
	{
		static const char *values[] = { "one: foo", "two: bar", "three: foobar", "four: cool companies", "five: apple", "six: garagegames", "seven: google", "eight: tesla", "nine: peace", "ten: in the world", "eleven: is worth working towards", "twelve: if it really involves work", "" };
		static int keys[] = { 300201, 12002, 58003, 9203, 278904, 87305, 880006, 123007, 3267008, 50409, 230010, 30659011, 2301012 };
		
		table the_table;
		for(int i = 0; values[i][0]; i++)
		{
			printf(".inserting \"%d\", \"%s\"\n", keys[i], values[i]);
			the_table.insert(keys[i], values[i]);
			if(verbose)
				dump_table(the_table);
		}
		
		dump_table(the_table);
		printf(".removing elements two...four\n");
		for(int i = 1; i < 4; i++)
		{
			typename table::pointer p = the_table.find(keys[i]);
			printf("key %d %s\n", keys[i], bool(p) ? "found." : "NOT FOUND - ERROR!");
			if(p)
				p.remove();
			if(verbose)
				dump_table(the_table);
		}
		dump_table(the_table);
	}
};
