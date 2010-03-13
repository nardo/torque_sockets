enum memory_storage_kind
{
	memory_storage_unknown,
	memory_storage_heap,
	memory_storage_small_block,
};

TYPE_INFO_TEMPLATE(pointer_memory_storage_kind, memory_storage_kind, kind, memory_storage_unknown);
BOOLEAN_TYPE_INFO_TEMPLATE(is_container, false);

#define declare_dynamic_class() virtual type_record *get_type_record() { return get_global_type_record(this); }


class type_record;
template<> inline uint32 hash(const type_record *record) { return uint32(record); }

template<typename type> type_record *get_global_type_record();

class context;

struct numeric_record
{
	bool is_integral;
	bool is_float;
	int32 bit_size;
	bool is_signed;
	int32 int_range_min;
	uint32 int_range_size;
	
	typedef float64 (*to_float64_fn)(void *source);
	typedef uint32 (*to_uint32_fn)(void *source);
	typedef bool (*from_float64_fn)(void *dest, float64 source);
	typedef bool (*from_uint32_fn)(void *dest, uint32 source);
	
	to_float64_fn to_float64;
	to_uint32_fn to_uint32;
	from_float64_fn from_float64;
	from_uint32_fn from_uint32;
};

template<typename type> struct numeric_manipulator
{
	static float64 to_float64(type *source) { return (float64) *source; }
	static uint32 to_uint32(type *source) { return (uint32) *source; }
	static void from_float64(type *dest, float64 source) { *dest = (type) source; }
	static void from_uint32(type *dest, uint32 source) { *dest = (type) source; }
};

template<typename type> struct numeric_record_instance : numeric_record
{
	numeric_record_instance()
	{
		is_integral = core::is_integral<type>::is_true;
		int_range_min = core::integer_range<type>::range_min;
		int_range_size = core::integer_range<type>::range_size;
		is_signed = core::is_signed<type>::is_true;
		is_float = core::is_float<type>::is_true;
		bit_size = 0;
		
		to_float64 = (to_float64_fn) &numeric_manipulator<type>::to_float64;
		to_uint32 = (to_uint32_fn) &numeric_manipulator<type>::to_uint32;
		from_float64 = (from_float64_fn) &numeric_manipulator<type>::from_float64;
		from_uint32 = (from_uint32_fn) &numeric_manipulator<type>::from_uint32;
	}
};

template<class type> numeric_record *get_numeric_record_instance(true_type)
{
	static numeric_record_instance<type> inst;
	return &inst;
};

template<class type> numeric_record *get_numeric_record_instance(false_type)
{
	return 0;
}

enum allocation_space {
	allocation_space_heap,
	allocation_space_pile,
	allocation_space_small_block,
};

template<typename type> struct pointer_manipulator
{
	typedef empty_type pointee_type;
	static const allocation_space pointee_allocation_space = allocation_space_heap;

	// by default this returns the pointer's pointee type.
	static type_record *get_real_pointee_type(type *pointer) { return get_global_type_record<pointee_type>(); }
	static pointee_type *dereference(type *type_instance) { return 0; }
	static void assign_instance(type *pointer, pointee_type *object) {}
};

struct pointer_record
{
	type_record *pointee_type;
	
	typedef void (*assign_instance_fn)(void *pointer, void *object);
	typedef void *(*dereference_fn)(void *pointer);
	typedef type_record *(*get_real_pointee_type_fn)(void *pointer);

	allocation_space pointee_allocation_space;
	assign_instance_fn assign_instance;
	dereference_fn dereference;
	get_real_pointee_type_fn get_real_pointee_type;
};

template<typename type> struct pointer_record_instance : pointer_record
{
	pointer_record_instance()
	{
		pointee_type = get_global_type_record<typename pointer_manipulator<type>::pointee_type>();
		pointee_allocation_space = pointer_manipulator<type>::pointee_allocation_space;
		assign_instance = (assign_instance_fn) &pointer_manipulator<type>::assign_instance;
		dereference = (dereference_fn) &pointer_manipulator<type>::dereference;
		get_real_pointee_type = (get_real_pointee_type_fn) &pointer_manipulator<type>::get_real_pointee_type;
	}
};

template<typename type> pointer_record *get_pointer_record_instance()
{
	static pointer_record_instance<type> inst;
	return &inst;
};

template<typename type> struct container_manipulator
{
	typedef empty_type key_type;
	typedef empty_type value_type;

	static bool reserve(type *container, uint32 size, context *the_context) { return false; }
	static void insert_key_value(type *container, key_type *key, value_type *value, context *the_context) { }
	static value_type *insert_key(type *container, key_type *key, context *the_context) { return 0; }
	static uint32 size(type *container) { return 0; }
	
	static value_type *find_element(type *container, key_type *key) { return 0; }
};

struct container_record
{
	typedef bool (*reserve_fn)(void *container, uint32 size, context *the_context);
	typedef void (*insert_key_value_fn)(void *container, void *key, void *value, context *the_context);
	typedef void *(*insert_key_fn)(void *container, void *key, context *the_context);
	typedef uint32 (*size_fn)(void *container);
	typedef void *(*find_element_fn)(void *container, void *key);

	type_record *key_type;
	type_record *value_type;
	
	reserve_fn reserve;
	insert_key_value_fn insert_key_value;
	insert_key_fn insert_key;
	size_fn size;
	find_element_fn find_element;
};

template<typename type> struct container_record_instance : container_record
{
	container_record_instance()
	{
		key_type = get_global_type_record<typename container_manipulator<type>::key_type>();
		value_type = get_global_type_record<typename container_manipulator<type>::value_type>();
		
		reserve = (reserve_fn) &container_manipulator<type>::reserve;
		insert_key_value = (insert_key_value_fn) &container_manipulator<type>::insert_key_value;
		insert_key = (insert_key_fn) &container_manipulator<type>::insert_key;
		size = (size_fn) &container_manipulator<type>::size;
		find_element = (find_element_fn) &container_manipulator<type>::find_element;
	}
};

template<typename type> container_record *get_container_record_instance()
{
	static container_record_instance<type> inst;
	return &inst;
};

struct type_record
{
	size_t size;
	bool is_numeric;
	numeric_record *numeric_info;
	
	bool is_pointer;
	pointer_record *pointer_info;
	
	bool is_container;
	container_record *container_info;
	
	typedef type_record *(get_type_from_instance_fn)(void *object);
	typedef void (*construct_object_fn)(void *object);
	typedef void (*construct_copy_object_fn)(void *object, void *copy);
	typedef void (*destroy_object_fn)(void *object);
	
	construct_object_fn construct_object;
	construct_copy_object_fn construct_copy_object;
	destroy_object_fn destroy_object;
	get_type_from_instance_fn get_type_from_instance;
};

template<class type_name> struct type_record_instance : public type_record
{
	type_record_instance()
	{
		size = sizeof(type_name);
		is_numeric = core::is_integral<type_name>::is_true || core::is_float<type_name>::is_true;
		
		typename bool_type<core::is_integral<type_name>::is_true || core::is_float<type_name>::is_true>::value_type has_numeric_info;
		
		numeric_info = get_numeric_record_instance<type_name>(has_numeric_info);
		
		is_pointer = core::is_pointer<type_name>::is_true;
		if(is_pointer)
			pointer_info = get_pointer_record_instance<type_name>();
		else
			pointer_info = 0;
		
		is_container = core::is_container<type_name>::is_true;
		if(is_container)
			container_info = get_container_record_instance<type_name>();
		else
			container_info = 0;
		
		type_name * (*construct_inst)(type_name *object) = &construct;
		type_name * (*construct_copy_inst)(type_name *object, const type_name &copy_of) = &construct;
		void (*destroy_inst)(type_name *object) = &destroy;
		
		construct_object = (construct_object_fn) construct_inst;
		construct_copy_object = (construct_copy_object_fn) construct_copy_inst;
		destroy_object = (destroy_object_fn) destroy_inst;
	}
};

template<class type_name> type_record *get_global_type_record()
{
	static type_record_instance<type_name> inst;
	return &inst;
}

template<class type_name> type_record *get_global_type_record( type_name const * )
{
	return get_global_type_record<type_name>();
}

#define TYPE_TEST(type, name) type name; printf("%08x - %s: size = %d, is_numeric = %d\n", \
	get_global_type_record(&name), #type, get_global_type_record(&name)->size, get_global_type_record(&name)->is_numeric)
	
static void test_type_record()
{
	TYPE_TEST(int, i1);
	TYPE_TEST(int *, i2);
}
