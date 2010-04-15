struct function_call_record
{
	enum
	{
		max_arguments = 4,
	};
	function_type_signature _signature;
	function_type_signature *get_signature() { return &_signature; }
};

template<class return_type, class arg0 = empty_type, class arg1 = empty_type, class arg2 = empty_type, class arg3 = empty_type, class arg4 = empty_type> struct function_call_record_decl : function_call_record
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

template<class return_type, class arg0> struct function_call_record_decl<return_type, arg0, empty_type, empty_type, empty_type> : function_call_record
{
	type_record *argument_types[1];
	function_call_record_decl()
	{
		_signature.argument_count = 1;
		_signature.return_type = get_global_type_record<return_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
	}
};

template<class arg0> struct function_call_record_decl<void, arg0, empty_type, empty_type, empty_type> : function_call_record
{
	type_record *argument_types[1];
	function_call_record_decl()
	{
		_signature.argument_count = 1;
		_signature.return_type = get_global_type_record<empty_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
	}
};

template<class return_type, class arg0, class arg1> struct function_call_record_decl<return_type, arg0, arg1, empty_type, empty_type> : function_call_record
{
	type_record *argument_types[2];
	function_call_record_decl()
	{
		_signature.argument_count = 2;
		_signature.return_type = get_global_type_record<return_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
	}
};

template<class arg0, class arg1> struct function_call_record_decl<void, arg0, arg1, empty_type, empty_type> : function_call_record
{
	type_record *argument_types[2];
	function_call_record_decl()
	{
		_signature.argument_count = 2;
		_signature.return_type = get_global_type_record<empty_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
	}
};

template<class return_type, class arg0, class arg1, class arg2> struct function_call_record_decl<return_type, arg0, arg1, arg2, empty_type> : function_call_record
{
	type_record *argument_types[3];
	function_call_record_decl()
	{
		_signature.argument_count = 3;
		_signature.return_type = get_global_type_record<return_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
	}
};

template<class arg0, class arg1, class arg2> struct function_call_record_decl<void, arg0, arg1, arg2, empty_type> : function_call_record
{
	type_record *argument_types[3];
	function_call_record_decl()
	{
		_signature.argument_count = 3;
		_signature.return_type = get_global_type_record<empty_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
	}
};

template<class return_type, class arg0, class arg1, class arg2, class arg3> struct function_call_record_decl<return_type, arg0, arg1, arg2, arg3> : function_call_record
{
	type_record *argument_types[4];
	function_call_record_decl()
	{
		_signature.argument_count = 4;
		_signature.return_type = get_global_type_record<return_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.argument_types[3] = get_global_type_record<arg3>();
	}
};

template<class arg0, class arg1, class arg2, class arg3> struct function_call_record_decl<void, arg0, arg1, arg2, arg3> : function_call_record
{
	type_record *argument_types[4];
	function_call_record_decl()
	{
		_signature.argument_count = 4;
		_signature.return_type = get_global_type_record<empty_type>();
		_signature.argument_types = argument_types;
		_signature.argument_types[0] = get_global_type_record<arg0>();
		_signature.argument_types[1] = get_global_type_record<arg1>();
		_signature.argument_types[2] = get_global_type_record<arg2>();
		_signature.argument_types[3] = get_global_type_record<arg3>();
	}
};
