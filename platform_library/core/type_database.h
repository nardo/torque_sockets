class type_database
{
	void _call_function(function_record *function, function_type_signature *calling_signature, void *return_value, void **arguments)
	{
		// for now, compare signatures and make sure they're exactly the same:
		function_type_signature *func_signature = function->get_signature();
		if(func_signature->return_type != calling_signature->return_type)
		{
			printf("Signature return types don't match!\n");
			return;
		}
		if(func_signature->argument_count != calling_signature->argument_count)
		{
			printf("Argument count mismatch!\n");
			return;
		}
		for(uint32 i = 0; i < func_signature->argument_count; i++)
		{
			if(func_signature->argument_types[i] != calling_signature->argument_types[i])
			{
				printf("arg type %d mismatch!!\n");
				return;
			}
		}
		function->dispatch(arguments, return_value);
	}
public:
	hash_table_flat<indexed_string, function_record *> _function_table;
	template<typename signature> void add_function(static_string static_name, signature func_address)
	{
		indexed_string name = static_to_indexed_string(static_name);
		function_record *rec = new function_record_decl<signature>(func_address);
		_function_table.insert(name, rec);
	}
	function_record *find_function(static_string static_name)
	{
		indexed_string name = static_to_indexed_string(static_name);
		function_record **func = _function_table.find(name).value();
		return func ? *func : 0;
	}
	template<class return_type> void call_function(function_record *func, return_type *return_value)
	{
		static function_call_record_decl<empty_type, return_type> call_decl;
		_call_function(func, call_decl.get_signature(), return_value, 0);
	}
	template<class return_type, class arg1_type> void call_function(function_record *func, return_type *return_value, arg1_type arg1 )
	{
		static function_call_record_decl<empty_type, return_type,arg1_type> call_decl;
		void *args[1];
		args[0] = &arg1;
		_call_function(func, call_decl.get_signature(), return_value, args);
	}
	
	// type_database contains the class tree
	struct field_rep
	{
		indexed_string name;
		type_record *type;
		uint32 offset;
	};
	
	enum type_kind
	{
		type_basic,
		type_compound,	
	};
	
	struct type_rep
	{
		type_kind kind;
		indexed_string name;
		type_rep *parent_class;
		dictionary<field_rep> fields;
		type_record *type;
	};
	static_to_indexed_string_map _string_remapper;
	
	indexed_string static_to_indexed_string(static_string s)
	{
		return _string_remapper.get(s);
	}
	
	context *_context;
	
	hash_table_flat<const type_record *, type_rep *> _type_record_lookup_rep_table;
	
	dictionary<type_rep *> _class_table;
	type_rep *_current_class;
	void _add_type(type_rep *the_type)
	{
		// make sure another type with this name hasn't been registered.
		assert(!_class_table.find(the_type->name));
		_class_table.insert(the_type->name, the_type);
		
		// add the reverse type mapping
		_type_record_lookup_rep_table.insert(the_type->type, the_type);
	}
public:
	type_database(context *the_context) : _string_remapper(the_context->get_string_table())
	{
		_context = the_context;
		_current_class = 0;
	}
	
	type_rep *find_type(indexed_string &type_name)
	{
		dictionary<type_rep *>::pointer p = _class_table.find(type_name);
		return p.value() ? *p.value() : 0;	
	}
	
	type_rep *find_type(const char *type_name)
	{
		indexed_string indexed_name(_context->get_string_table(), type_name);
		// printf("** looking for %s (%d) **\n", indexed_name.c_str(), indexed_name.hash());
		return find_type(indexed_name);
	}
	field_rep *find_field(type_rep *the_type, const char *field_name)
	{
		indexed_string indexed_field_name(_context->get_string_table(), field_name);
		
		dictionary<field_rep>::pointer p;
		
		for(type_rep *walk = the_type; walk; walk = walk->parent_class)
		{
			p = walk->fields.find(indexed_field_name);
			if(p)
				return p.value();
		}
		return 0;
	}
	
	type_rep *find_type(type_record *the_type)
	{
		hash_table_flat<const type_record *, type_rep *>::pointer p = _type_record_lookup_rep_table.find(the_type);
		type_rep **rep = p.value();
		return rep ? *rep : 0;
	}
	
	void add_basic_type(static_string type_name, type_record *the_type_record)
	{
		type_rep *the_type = new type_rep;
		the_type->name = static_to_indexed_string(type_name);
		the_type->kind = type_basic;
		the_type->type = the_type_record;
		_add_type(the_type);
	}
	
	void begin_class(static_string class_name, static_string super_class_name, type_record *class_type)
	{
		printf("beginning class %s - parent = %s\n", class_name, super_class_name);
		assert(_current_class == 0);
		indexed_string super_class_string = static_to_indexed_string(super_class_name);
		
		
		type_rep *the_class = new type_rep;
		the_class->name = static_to_indexed_string(class_name);
		the_class->type = class_type;
		the_class->kind = type_compound;
		dictionary<type_rep *>::pointer parent_p = _class_table.find(super_class_string);
		
		if(parent_p)
			the_class->parent_class = *parent_p.value();
		else
			the_class->parent_class = 0;
		
		_current_class = the_class;
		_add_type(the_class);
	}
	
	void add_public_slot(static_string slot_name, uint32 slot_offset, type_record *type)
	{
		assert(_current_class != 0);
		printf("  -adding field %s - offset = %d, type = %08x\n", slot_name, slot_offset, type);
		field_rep the_field;
		indexed_string name = static_to_indexed_string(slot_name);
		the_field.name = name;
		the_field.offset = slot_offset;
		the_field.type = type;
		_current_class->fields.insert(name, the_field);
	}
	
	void end_class()
	{
		assert(_current_class != 0);
		_current_class = 0;
	}
	
	void dump()
	{
		printf("----- Class Table -----\n");
		printf("%d classes registered.\n", _class_table.size());
		for(dictionary<type_rep *>::pointer p = _class_table.first(); p; ++p)
		{
			indexed_string parent_class_name;
			if((*p.value())->parent_class)
				parent_class_name = (*p.value())->parent_class->name;
			printf("class %s : %s (%x)\n", (*p.value())->name.c_str(), parent_class_name.c_str(), (*p.value())->type);
			for(hash_table_array<indexed_string, field_rep>::pointer fp = (*p.value())->fields.first(); fp; ++fp)
				dump_field(fp.value());
			dictionary<type_rep *>::pointer lookup_p = _class_table.find((*p.value())->name);
			if(!lookup_p)
				printf("  ERROR - LOOKUP FAIL.\n");
			else
				printf("  Class found on lookup.\n");
		}
	}
	
	void dump_field(field_rep *the_field)
	{
		type_record *field_type = the_field->type;
		
		printf("  %s - offset = %d - type = %08x  ", the_field->name.c_str(), the_field->offset, field_type);
		dump_type(field_type);
		printf("\n");
	}
	
	void dump_type(type_record *type)
	{
		if(!type)
			printf("<null>");
		
		if(type->is_container)
		{
			dump_type(type->container_info->value_type);
			printf("[");
			dump_type(type->container_info->key_type);
			printf("]");
			return;
		}
		if(type->is_numeric)
		{
			numeric_record *numeric_info = type->numeric_info;
			if(numeric_info->is_integral)
			{
				printf("%sint{%d..%u}", numeric_info->is_signed ? "" : "u", numeric_info->int_range_min, numeric_info->int_range_min + (numeric_info->int_range_size));
			}
			else if(numeric_info->is_float)
			{
				printf("float%d", type->size * 8);
			}
			return;
		}
		indexed_string field_type_name;
		bool field_is_pointer = false;

		type_rep **field_type_rep = _type_record_lookup_rep_table.find(type).value();
		if(field_type_rep)
			field_type_name = (*field_type_rep)->name;
		else if(type->is_pointer)
		{
			field_is_pointer = true;
			type_record *pointee_type = type->pointer_info->pointee_type;
			field_type_rep = _type_record_lookup_rep_table.find(pointee_type).value();
			if(field_type_rep)
				field_type_name = (*field_type_rep)->name;
		}
		if(field_is_pointer)
			printf("*");
		printf("%s", field_type_name.c_str());
	}

	struct type_conversion_info
	{
		type_record *source_type;
		type_record *dest_type;
		typedef bool (*conversion_fn_type)(void *dest_ptr, void *source_ptr, context *the_context);
		conversion_fn_type conversion_function;
	};
	hash_table_array<uint32, type_conversion_info> _conversion_table;

	static uint32 get_type_conversion_key(type_record *source_type, type_record *dest_type)
	{
		return (* ((uint32 *) &dest_type) << 8) ^ *((uint32 *) &source_type);
	}
	
	template<class a, class b> void add_type_conversion(bool (*conversion_func)(a *dest, b *source, context *), bool loses_information)
	{
		type_record *dest_type = get_global_type_record<a>();
		type_record *source_type = get_global_type_record<b>();
		
		uint32 key = get_type_conversion_key(source_type, dest_type);
		type_conversion_info info;
		info.source_type = source_type;
		info.dest_type = dest_type;
		info.conversion_function = (type_conversion_info::conversion_fn_type) conversion_func;
		_conversion_table.insert(key, info);
	}

	//static void convert_float
	bool type_convert(void *dest, type_record *dest_type, void *source, type_record *source_type)
	{
		if(dest_type == source_type)
		{
			// if it's a straight copy, use the type's construction functions to copy the data:
			dest_type->destroy_object(dest);
			dest_type->construct_copy_object(dest, source);
			return true;
		}
		
		if(source_type->is_numeric && dest_type->is_numeric)
		{
			// in the numeric case, for now, first check if it's float:
			numeric_record *source_num = source_type->numeric_info;
			numeric_record *dest_num = dest_type->numeric_info;
			
			if(dest_num->is_float)
				dest_num->from_float64(dest, source_num->to_float64(source));
			else
				dest_num->from_uint32(dest, source_num->to_uint32(source));
				
			return true;
		}
		
		// look up the conversion in the conversion table:
		uint32 key = get_type_conversion_key(source_type, dest_type);
		hash_table_array<uint32, type_conversion_info>::pointer p = _conversion_table.find(key);
		type_conversion_info *i = p.value();

		while(i && (i->source_type != source_type || i->dest_type != dest_type))
		{
			p.next_match();
			i = p.value();
		}
		if(!i)
			return false;

		bool lossless = i->conversion_function(dest, source, _context);
		return true;
	}
};

