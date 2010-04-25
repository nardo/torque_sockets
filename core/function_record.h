struct function_type_signature
{
	uint32 argument_count;
	type_record **argument_types;
	type_record *return_type;
	type_record *class_type;
	
	function_type_signature()
	{
		class_type = 0;
	}
};

struct function_call_storage
{
	void *_storage;
	void **_args;
	void *_return_value;
	function_type_signature *_signature;

	inline uint32 align(uint32 size)
	{
		return (size + 7) & ~7;
	}

	function_call_storage(function_type_signature *sig)
	{
		_signature = sig;
		uint32 total_size = 0;
		total_size += align(sizeof(void *) * sig->argument_count);
		uint32 arg_start = total_size;
		for(uint32 i = 0; i < sig->argument_count; i++)
			total_size += align(sig->argument_types[i]->size);
		total_size += align(sig->return_type->size);
		_storage = memory_allocate(total_size);
		_args = (void **) _storage;
		
		uint8 *ptr = (uint8 *) _storage;
		ptr += arg_start;
		
		for(uint32 i = 0; i < sig->argument_count;i++)
		{
			_args[i] = ptr;
			sig->argument_types[i]->construct_object(_args[i]);
			ptr += align(sig->argument_types[i]->size);
		}
		_return_value = ptr;
		sig->return_type->construct_object(_return_value);
	}
	~function_call_storage()
	{
		for(uint32 i = 0; i < _signature->argument_count; i++)
			_signature->argument_types[i]->destroy_object(_args[i]);
		_signature->return_type->destroy_object(_return_value);
		memory_deallocate(_storage);
	}
};

struct function_record
{
	virtual ~function_record() {}
	function_type_signature _signature;
	function_type_signature *get_signature() { return &_signature; }
	
	// dispatch method is called to call the function.  Note that the arguments and return_data_ptr MUST match the signature of the function or this will cause unpredictable results.
	virtual void dispatch(void *object, void **arguments, void *return_data_ptr) = 0;
};

template<class empty> struct function_record_decl : function_record
{
	function_record_decl() {}
};

template<class T, class return_type> struct function_record_decl<return_type (T::*)()> : function_record
{
	typedef return_type (T::*func_ptr)();
	func_ptr _func;
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 0;
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		T *ptr = (T*) object;
		return_type *dest = (return_type *) return_data_ptr;
		func_ptr f = _func;
		
		*dest = (ptr->*_func)();
	}	
};

template<class T, class arg0> struct function_record_decl<void (T::*)(arg0)> : function_record
{
	typedef void (T::*func_ptr)(arg0);
	func_ptr _func;
	type_record *_arg_types[1];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 1;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		( ((T*)object)->*_func) (*( (arg0 *)arguments[0]) );
	}	
};

template<class T, class arg0, class arg1> struct function_record_decl<void (T::*)(arg0,arg1)> : function_record
{
	typedef void (T::*func_ptr)(arg0,arg1);
	func_ptr _func;
	type_record *_arg_types[2];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 2;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		(((T*)object)->*_func) (*( (arg0 *)arguments[0]), *( (arg1 *)arguments[1]) );
	}	
};

template<class T, class arg0, class arg1, class arg2> struct function_record_decl<void (T::*)(arg0,arg1,arg2)> : function_record
{
	typedef void (T::*func_ptr)(arg0,arg1,arg2);
	func_ptr _func;
	type_record *_arg_types[3];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 3;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		((T*)object)->*_func (*( (arg0 *)arguments[0]), *( (arg1 *)arguments[1]), *( (arg2 *)arguments[2]));
	}	
};

template<class T, class arg0, class arg1, class arg2, class arg3> struct function_record_decl<void (T::*)(arg0,arg1,arg2,arg3)> : function_record
{
	typedef void (T::*func_ptr)(arg0,arg1,arg2,arg3);
	func_ptr _func;
	type_record *_arg_types[4];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 4;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.argument_types[3] = get_global_type_record<arg3>();
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		((T*)object)->*_func (*( (arg0 *)arguments[0]), *( (arg1 *)arguments[1]), *( (arg2 *)arguments[2]), *( (arg3 *)arguments[3]));
	}	
};

template<class T, class arg0, class arg1, class arg2, class arg3,class arg4> struct function_record_decl<void (T::*)(arg0,arg1,arg2,arg3,arg4)> : function_record
{
	typedef void (T::*func_ptr)(arg0,arg1,arg2,arg3,arg4);
	func_ptr _func;
	type_record *_arg_types[5];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 5;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.argument_types[3] = get_global_type_record<arg3>();
		_signature.argument_types[4] = get_global_type_record<arg4>();
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		((T*)object)->*_func (*( (arg0 *)arguments[0]), *( (arg1 *)arguments[1]), *( (arg2 *)arguments[2]), *( (arg3 *)arguments[3]), *( (arg4 *)arguments[4]));
	}	
};

template<class T, class return_type, class arg0> struct function_record_decl<return_type (T::*)(arg0)> : function_record
{
	typedef return_type (T::*func_ptr)(arg0);
	func_ptr _func;
	type_record *_arg_types[1];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 1;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = (((T*)object)->*_func)(*((arg0 *)arguments[0]));
	}	
};

template<class T, class return_type, class arg0, class arg1> struct function_record_decl<return_type (T::*)(arg0,arg1)> : function_record
{
	typedef return_type (T::*func_ptr)(arg0,arg1);
	func_ptr _func;
	type_record *_arg_types[2];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 2;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = ( ((T*)object)->*_func) (*((arg0 *)arguments[0]), *((arg1 *)arguments[1]));
	}
};

template<class T, class return_type, class arg0, class arg1, class arg2> struct function_record_decl<return_type (T::*)(arg0,arg1,arg2)> : function_record
{
	typedef return_type (T::*func_ptr)(arg0,arg1,arg2);
	func_ptr _func;
	type_record *_arg_types[3];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 3;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = ( ((T*)object)->*_func) (*((arg0 *)arguments[0]), *((arg1 *)arguments[1]), *((arg2 *)arguments[2]));
	}
};

template<class T, class return_type, class arg0, class arg1, class arg2, class arg3> struct function_record_decl<return_type (T::*)(arg0,arg1,arg2,arg3)> : function_record
{
	typedef return_type (T::*func_ptr)(arg0,arg1,arg2,arg3);
	func_ptr _func;
	type_record *_arg_types[4];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 4;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.argument_types[3] = get_global_type_record<arg3>();
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = ((T*)object)->*_func(*((arg0 *)arguments[0]), *((arg1 *)arguments[1]), *((arg2 *)arguments[2]), *((arg3 *)arguments[3]));
	}
};

template<class T, class return_type, class arg0, class arg1, class arg2, class arg3, class arg4> struct function_record_decl<return_type (T::*)(arg0,arg1,arg2,arg3,arg4)> : function_record
{
	typedef return_type (T::*func_ptr)(arg0,arg1,arg2,arg3,arg4);
	func_ptr _func;
	type_record *_arg_types[5];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.class_type = get_global_type_record<T>();
		_signature.argument_count = 5;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.argument_types[3] = get_global_type_record<arg3>();
		_signature.argument_types[4] = get_global_type_record<arg4>();
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = ( ((T*)object)->*_func) (*((arg0 *)arguments[0]), *((arg1 *)arguments[1]), *((arg2 *)arguments[2]), *((arg3 *)arguments[3]), *((arg4 *)arguments[4]));
	}
};

template<class return_type> struct function_record_decl<return_type (*)()> : function_record
{
	typedef return_type (*func_ptr)();
	func_ptr _func;
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.argument_count = 0;
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = _func();
	}
};

template<> struct function_record_decl<void (*)()> : function_record
{
	typedef void (*func_ptr)();
	func_ptr _func;
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.argument_count = 0;
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		_func();
	}
};

template<class return_type, class arg1> struct function_record_decl<return_type (*)(arg1)> : function_record
{
	typedef return_type (*func_ptr)(arg1);
	func_ptr _func;
	type_record *_arg_types[1];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.argument_count = 1;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg1>();
		_signature.return_type = get_global_type_record<return_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		*((return_type *) return_data_ptr) = _func(*((arg1 *)arguments[0]));
	}
};

template<class arg1> struct function_record_decl<void (*)(arg1)> : function_record
{
	typedef void (*func_ptr)(arg1);
	func_ptr _func;
	type_record *_arg_types[1];
	
	function_record_decl(func_ptr f)
	{
		_func = f;
		_signature.argument_count = 1;
		_signature.argument_types = _arg_types;
		_signature.argument_types[0] = get_global_type_record<arg1>();
		_signature.return_type = get_global_type_record<empty_type>();
	}
	void dispatch(void *object, void **arguments, void *return_data_ptr)
	{
		_func(*((arg1 *)arguments[0]));
	}
};


