
class safe_object_ref;

class ref_object
{
	safe_object_ref *_first_object_ref; ///< The head of the linked list of safe object references.
	uint32 _ref_count;                  ///< Reference counter for ref_ptr objects.
	
	friend class safe_object_ref;
	friend class ref_object_ref;
public:
	ref_object();
	virtual ~ref_object();
	
	/// object destroy self call (from ref_ptr).
	///
	/// @note Override if this class has specially allocated memory.
	virtual void destroy_self() { delete this; }
			
	void inc_ref()
	{
		_ref_count++;
	}
	
	void dec_ref()
	{
		_ref_count--;
		if(!_ref_count)
			destroy_self();
	}
	/// @}
};

/// Base class for object reference counting.
class ref_object_ref
	{
	protected:
		ref_object *_object; ///< The object this ref_object_ref references.
		
		/// Increments the reference count on the referenced object.
		void inc_ref()
		{
			if(_object)
				_object->inc_ref();
		}
		
		/// Decrements the reference count on the referenced object.
		void dec_ref()
		{
			if(_object)
			{
				_object->dec_ref();
			}
		}
	public:
		
		/// Constructor, assigns from the object and increments its reference count if it's not NULL.
		ref_object_ref(ref_object *the_object = NULL)
		{
			_object = the_object;
			inc_ref();
		}
		
		/// Destructor, dereferences the object, if there is one.
		~ref_object_ref()
		{
			dec_ref();
		}
		
		/// Assigns this reference object from an existing object instance.
		void set(ref_object *the_object)
		{
			dec_ref();
			_object = the_object;
			inc_ref();
		}
	};

/// Reference counted object template pointer class.
///
/// Instances of this template class can be used as pointers to
/// instances of object and its subclasses.  The object will not
/// be deleted until all of the ref_ptr instances pointing to it
/// have been destructed.
template <class T> class ref_ptr : public ref_object_ref
{
public:
	ref_ptr() : ref_object_ref() {}
	ref_ptr(T *ptr) : ref_object_ref(ptr) {}
	ref_ptr(const ref_ptr<T>& ref) : ref_object_ref((T *) ref._object) {}
	
	ref_ptr<T>& operator=(const ref_ptr<T>& ref)
	{
		set((T *) ref._object);
		return *this;
	}
	ref_ptr<T>& operator=(T *ptr)
	{
		set(ptr);
		return *this;
	}
	bool is_null() const   { return _object == 0; }
	bool is_valid() const  { return _object != 0; }
	T* operator->() const { return static_cast<T*>(_object); }
	T& operator*() const  { return *static_cast<T*>(_object); }
	operator T*() const   { return static_cast<T*>(_object); }
	operator T*() { return static_cast<T*>(_object); }
	T* get_pointer() { return static_cast<T*>(_object); }
};

/// Base class for object safe pointers.
class safe_object_ref
{
	friend class ref_object;
protected:
	ref_object *_object;               ///< The object this is a safe pointer to, or NULL if the object has been deleted.
	safe_object_ref *_prev_object_ref; ///< The previous safe_object_ref for _object.
	safe_object_ref *_next_object_ref; ///< The next safe_object_ref for _object.
public:
	safe_object_ref(ref_object *object)
	{
		_object = object;
		register_reference();
	}

	safe_object_ref()
	{
		_object = NULL;
	}
	
	void set(ref_object *object)
	{
		unregister_reference();
		_object = object;
		register_reference();
	}
	~safe_object_ref()
	{
		unregister_reference();
	}
	
	
	void register_reference()   ///< Links this safe_object_ref into the doubly linked list of safe_object_ref instances for _object.
	{
		if(_object)
		{
			_next_object_ref = _object->_first_object_ref;
			if(_next_object_ref)
				_next_object_ref->_prev_object_ref = this;
			_prev_object_ref = NULL;
			_object->_first_object_ref = this;
		}
	}
	
	void unregister_reference() ///< Unlinks this safe_object_ref from the doubly linked list of safe_object_ref instance for _object.
	{
		if(_object)
		{
			if(_prev_object_ref)
				_prev_object_ref->_next_object_ref = _next_object_ref;
			else
				_object->_first_object_ref = _next_object_ref;
			if(_next_object_ref)
				_next_object_ref->_prev_object_ref = _prev_object_ref;
		}
	}
	
};

/// Safe object template pointer class.
///
/// Instances of this template class can be used as pointers to
/// instances of object and its subclasses.
///
/// When the object referenced by a safe_ptr instance is deleted,
/// the pointer to the object is set to NULL in the safe_ptr instance.
template <class T> class safe_ptr : public safe_object_ref
{
public:
	safe_ptr() : safe_object_ref() {}
	safe_ptr(T *ptr) : safe_object_ref(ptr) {}
	safe_ptr(const safe_ptr<T>& ref) : safe_object_ref((T *) ref._object) {}
	
	safe_ptr<T>& operator=(const safe_ptr<T>& ref)
	{
		set((T *) ref._object);
		return *this;
	}
	safe_ptr<T>& operator=(T *ptr)
	{
		set(ptr);
		return *this;
	}
	bool is_null() const   { return _object == 0; }
	bool is_valid() const  { return _object != 0; }
	T* operator->() const { return static_cast<T*>(_object); }
	T& operator*() const  { return *static_cast<T*>(_object); }
	operator T*() const   { return static_cast<T*>(_object); }
	operator T*() { return reinterpret_cast<T*>(_object); }
	T* get_pointer() { return static_cast<T*>(_object); }
};

