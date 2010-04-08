/// Base class for functor_decl template classes.  The functor objects store the parameters and member function pointer for the invocationof some class member function.  functor is used in TNL by the RPC mechanism to store a function for later transmission and dispatch, either to a remote host, a journal file, or another thread in the process.

struct functor : public ref_object {
	/// Construct the functor.
	functor() {}
	/// Destruct the functor.
	virtual ~functor() {}
	/// Reads this functor from a bit_stream.
	virtual void read(bit_stream &stream) = 0;
	/// Writes this functor to a bit_stream.
	virtual void write(bit_stream &stream) = 0;
	/// Dispatch the function represented by the functor.
	virtual void dispatch(void *t) = 0;
};

class functor_creator
{
public:
	virtual functor *create() = 0;
	virtual ~functor_creator() {}
};

template<typename signature> class functor_creator_decl : public functor_creator
{
	signature f;
public:
	functor_creator_decl(signature sig)
	{
		f = sig;
	}
	functor *create()
	{
		return new functor_decl<signature>(f);
	}
};


/// functor_decl template class.  This class is specialized based on the member function call signature of the method it represents.  Other specializations hold specific member function pointers and slots for each of the function arguments.
template <class T> 
struct functor_decl : public functor {
	functor_decl() {}
	void set() {}
	void read(bit_stream &stream) {}
	void write(bit_stream &stream) {}
	void dispatch(void *t) { }
};

template <class T> 
struct functor_decl<void (T::*)()> : public functor {
	typedef void (T::*func_ptr)();
	func_ptr ptr;
	functor_decl(func_ptr p) : ptr(p) {}
	void set() {}
	void read(bit_stream &stream) {}
	void write(bit_stream &stream) {}
	void dispatch(void *t) { ((T *)t->*ptr)(); }
};
template <class T, class A> 
struct functor_decl<void (T::*)(A)> : public functor {
	typedef void (T::*func_ptr)(A);
	func_ptr ptr; A a;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a) { a = _a; }
	void read(bit_stream &stream) { core::read(stream, a); }
	void write(bit_stream &stream) { core::write(stream, a); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a); }
};
template <class T, class A, class B>
struct functor_decl<void (T::*)(A,B)>: public functor {
	typedef void (T::*func_ptr)(A,B);
	func_ptr ptr; A a; B b;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b) { a = _a; b = _b;}
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b); }
};

template <class T, class A, class B, class C>
struct functor_decl<void (T::*)(A,B,C)>: public functor {
	typedef void (T::*func_ptr)(A,B,C);
	func_ptr ptr; A a; B b; C c;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c) { a = _a; b = _b; c = _c;}
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c); }
};

template <class T, class A, class B, class C, class D>
struct functor_decl<void (T::*)(A,B,C,D)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D);
	func_ptr ptr; A a; B b; C c; D d;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d) { a = _a; b = _b; c = _c; d = _d; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d); }
};

template <class T, class A, class B, class C, class D, class E>
struct functor_decl<void (T::*)(A,B,C,D,E)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D,E);
	func_ptr ptr; A a; B b; C c; D d; E e;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d, E &_e) { a = _a; b = _b; c = _c; d = _d; e = _e; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); core::read(stream, e); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); core::write(stream, e); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e); }
};

template <class T, class A, class B, class C, class D, class E, class F>
struct functor_decl<void (T::*)(A,B,C,D,E,F)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D,E,F);
	func_ptr ptr; A a; B b; C c; D d; E e; F f;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); core::read(stream, e); core::read(stream, f); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); core::write(stream, e); core::write(stream, f); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G>
struct functor_decl<void (T::*)(A,B,C,D,E,F,G)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D,E,F,G);
	func_ptr ptr; A a; B b; C c; D d; E e; F f; G g;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); core::read(stream, e); core::read(stream, f); core::read(stream, g); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); core::write(stream, e); core::write(stream, f); core::write(stream, g); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H>
struct functor_decl<void (T::*)(A,B,C,D,E,F,G,H)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D,E,F,G,H);
	func_ptr ptr; A a; B b; C c; D d; E e; F f; G g; H h;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); core::read(stream, e); core::read(stream, f); core::read(stream, g); core::read(stream, h); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); core::write(stream, e); core::write(stream, f); core::write(stream, g); core::write(stream, h); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
struct functor_decl<void (T::*)(A,B,C,D,E,F,G,H,I)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D,E,F,G,H,I);
	func_ptr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); core::read(stream, e); core::read(stream, f); core::read(stream, g); core::read(stream, h); core::read(stream, i); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); core::write(stream, e); core::write(stream, f); core::write(stream, g); core::write(stream, h); core::write(stream, i); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
struct functor_decl<void (T::*)(A,B,C,D,E,F,G,H,I,J)>: public functor {
	typedef void (T::*func_ptr)(A,B,C,D,E,F,G,H,I,J);
	func_ptr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
	functor_decl(func_ptr p) : ptr(p) {}
	void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; }
	void read(bit_stream &stream) { core::read(stream, a); core::read(stream, b); core::read(stream, c); core::read(stream, d); core::read(stream, e); core::read(stream, f); core::read(stream, g); core::read(stream, h); core::read(stream, i); core::read(stream, j); }
	void write(bit_stream &stream) { core::write(stream, a); core::write(stream, b); core::write(stream, c); core::write(stream, d); core::write(stream, e); core::write(stream, f); core::write(stream, g); core::write(stream, h); core::write(stream, i); core::write(stream, j); }
	void dispatch(void *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j); }
};

