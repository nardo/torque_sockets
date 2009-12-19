struct function_type_signature
{
	uint32 argument_count;
	type_record **argument_types;
	type_record *return_type;
};

struct function_call_record
{
	function_type_signature _signature;
	function_type_signature *get_signature() { return &_signature; }
};

template<class return_type, class arg1 = empty_type, class arg2 = empty_type, class arg3 = empty_type, class arg4 = empty_type> struct function_call_record_decl : function_call_record
{
	function_call_record_decl() {}
};

template<class return_type> struct function_call_record_decl<return_type, empty_type, empty_type, empty_type, empty_type> : function_call_record
{
	function_call_record_decl()
	{
		_signature.argument_count = 0;
		_signature.return_type = get_global_type_record<return_type>();
	}
};

template<> struct function_call_record_decl<void, empty_type, empty_type, empty_type, empty_type> : function_call_record
{
	function_call_record_decl()
	{
		_signature.argument_count = 0;
		_signature.return_type = get_global_type_record<empty_type>();
	}
};

template<class return_type, class arg1> struct function_call_record_decl<return_type, arg1, empty_type, empty_type, empty_type> : function_call_record
{
	type_record *argument_types[1];
	function_call_record_decl()
	{
		_signature.argument_count = 1;
		_signature.return_type = get_global_type_record<return_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg1>();
	}
};

template<class arg1> struct function_call_record_decl<void, arg1, empty_type, empty_type, empty_type> : function_call_record
{
	type_record *argument_types[1];
	function_call_record_decl()
	{
		_signature.argument_count = 1;
		_signature.return_type = get_global_type_record<empty_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg1>();
	}
};

struct function_record
{
	function_type_signature _signature;
	function_type_signature *get_signature() { return &_signature; }
	
	// dispatch method is called to call the function.  Note that the arguments and return_data_ptr MUST match the signature of the function or this will cause unpredictable results.
	virtual void dispatch(void **arguments, void *return_data_ptr) = 0;
};

template<class empty> struct function_record_decl : function_record
{
	function_record_decl() {}
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
	void dispatch(void **arguments, void *return_data_ptr)
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
	void dispatch(void **arguments, void *return_data_ptr)
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
	void dispatch(void **arguments, void *return_data_ptr)
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
	void dispatch(void **arguments, void *return_data_ptr)
	{
		_func(*((arg1 *)arguments[0]));
	}
};


