#include "typelib.hh"

#include <dyncall.h>
#include <dynload.h>
#include <boost/static_assert.hpp>

static VALUE cCallVM;
static VALUE cLibrary;
static VALUE cFunction;

using namespace Typelib;
using namespace typelib_ruby;

static VALUE
library_wrap(VALUE self, VALUE name, VALUE auto_unload)
{
    void* libhandle = dlLoadLibrary(StringValuePtr(name));
    if (!libhandle)
	rb_raise(rb_eArgError, "cannot load library %s", StringValuePtr(name));

    if (RTEST(auto_unload))
        return Data_Wrap_Struct(cLibrary, 0, dlFreeLibrary, libhandle);
    else
        return Data_Wrap_Struct(cLibrary, 0, 0, libhandle);
}

/* call-seq:
 *   lib.find(function_name) => function object
 *
 * Searches for the function named +function_name+ in +lib+ and returns
 * the corresponding Function object. If the symbol is not available in
 * the library, raises ArgumentError.
 */
static VALUE
library_find(VALUE self, VALUE name)
{
    //void* libhandle;
    DLLib* libhandle; 
    Data_Get_Struct(self, DLLib, libhandle);

    void* symhandle = dlFindSymbol(libhandle, StringValuePtr(name));
    if (!symhandle)
    {
	VALUE libname = rb_iv_get(self, "@name");
	rb_raise(rb_eArgError, "cannot find symbol '%s' in library '%s'",
		StringValuePtr(name), StringValuePtr(libname));
    }

    VALUE function = Data_Wrap_Struct(cFunction, 0, 0, symhandle);
    rb_funcall(function, rb_intern("initialize"), 2, self, name);
    return function;
}

class PrepareVM : public TypeVisitor
{
    DCCallVM* m_vm;
    VALUE m_data;

    virtual bool visit_ (Numeric const& type)
    {
        if (type.getNumericCategory() == Numeric::Float)
	{
	    double value = NUM2DBL(m_data);
            switch(type.getSize())
            {
                case 4: dcArgFloat(m_vm,  static_cast<float>(value)); break;
                case 8: dcArgDouble(m_vm, static_cast<double>(value)); break;
            }
	}
	else
        {
            switch(type.getSize())
            {
                case 1: dcArgChar    (m_vm, static_cast<char>(NUM2INT(m_data))); break;
                case 2: dcArgShort   (m_vm, static_cast<short>(NUM2INT(m_data))); break;
                case 4: dcArgInt	 (m_vm, NUM2INT(m_data)); break;
                case 8: dcArgLongLong(m_vm, NUM2LL(m_data)); break;
            }
        }
        return false;
    }
    virtual bool visit_ (Enum const& type)      
    { 
        BOOST_STATIC_ASSERT(( sizeof(Enum::integral_type) == sizeof(int) ));
        dcArgInt(m_vm, NUM2INT(m_data));
        return false;
    }

    virtual bool visit_ (Pointer const& type)
    {
        dcArgPointer(m_vm, memory_cptr(m_data));
        return false;
    }
    virtual bool visit_ (Array const& type)
    { 
        dcArgPointer(m_vm, memory_cptr(m_data));
	return false;
    }

    virtual bool visit_ (Compound const& type)
    { throw UnsupportedType(type); }

public:
    PrepareVM(DCCallVM* vm, VALUE data)
	: m_vm(vm), m_data(data) {}

    static void append(DCCallVM* vm, Type const& type, VALUE data)
    {
	PrepareVM visitor(vm, data);
	visitor.apply(type);
    }
};

static VALUE
function_compile(VALUE self, VALUE filtered_args)
{
    DCCallVM* vm = dcNewCallVM(4096);
    VALUE rb_vm  = Data_Wrap_Struct(cCallVM, 0, dcFree, vm);
    rb_iv_set(rb_vm, "@arguments", filtered_args);

    VALUE argument_types = rb_iv_get(self, "@arguments");
    size_t argc = RARRAY_LEN(argument_types);
    VALUE* argv = RARRAY_PTR(argument_types);

    for (size_t i = 0; i < argc; ++i)
	PrepareVM::append(vm, rb2cxx::object<Type>(argv[i]), RARRAY_PTR(filtered_args)[i]);

    return rb_vm;
}

class VMCall : public TypeVisitor
{
    DCCallVM* m_vm;
    void*     m_handle;
    VALUE     m_return_type;
    VALUE     m_return;

    virtual bool visit_ (NullType const& type)
    {
	dcCallVoid(m_vm, m_handle);
	return true;
    }

    virtual bool visit_ (Numeric const& type)
    {
        if (type.getNumericCategory() == Numeric::Float)
	{
            switch(type.getSize())
            {
                case 4: m_return = rb_float_new(dcCallFloat(m_vm,  m_handle)); break;
                case 8: m_return = rb_float_new(dcCallDouble(m_vm, m_handle)); break;
            }
	}
	else
        {
            switch(type.getSize())
            {
                case 1: m_return = INT2NUM (dcCallChar    (m_vm, m_handle)); break;
                case 2: m_return = INT2NUM (dcCallShort   (m_vm, m_handle)); break;
                case 4: m_return = INT2NUM (dcCallInt	  (m_vm, m_handle)); break;
                case 8: m_return = LL2NUM  (dcCallLongLong(m_vm, m_handle)); break;
            }
        }
        return false;
    }
    virtual bool visit_ (Enum const& type)      
    { 
        BOOST_STATIC_ASSERT(( sizeof(Enum::integral_type) == sizeof(int) ));
        m_return = INT2NUM(dcCallInt(m_vm, m_handle));
        return false;
    }

    virtual bool visit_ (Pointer const& type)
    {
#ifdef  VERBOSE
        fprintf(stderr, "wrapping dcCallPointer with type=%s\n", type.getName().c_str());
#endif
	m_return = memory_wrap(new void*(dcCallPointer(m_vm, m_handle)), true);
        return false;
    }
    virtual bool visit_ (Array const& type)
    { 
#ifdef  VERBOSE
        fprintf(stderr, "wrapping dcCallPointer with type=%s\n", type.getName().c_str());
#endif
	m_return = memory_wrap(new void*(dcCallPointer(m_vm, m_handle)), true);
	return false;
    }

    virtual bool visit_ (Compound const& type)
    { throw UnsupportedType(type); }

public:
    VMCall(DCCallVM* vm, void* handle, VALUE return_type)
	: m_vm(vm), m_handle(handle), m_return_type(return_type), 
	m_return(Qnil) {}

    VALUE getReturnedValue() const { return m_return; }

    static VALUE call(DCCallVM* vm, void* handle, VALUE return_type)
    {
	if (NIL_P(return_type))
	{
	    dcCallVoid(vm, handle);
	    return Qnil;
	}

	VMCall visitor(vm, handle, return_type);
	visitor.apply(rb2cxx::object<Type>(return_type));
	return visitor.getReturnedValue();
    }
};

static VALUE
vm_call(VALUE self, VALUE function)
{
    DCCallVM* vm;
    Data_Get_Struct(self, DCCallVM, vm);

    void* symhandle;
    Data_Get_Struct(function, void, symhandle);

    return VMCall::call(vm, symhandle, rb_iv_get(function, "@return_type"));
}

static
VALUE filter_numeric_arg(VALUE self, VALUE arg, VALUE rb_expected_type)
{
    Type const& expected_type = rb2cxx::object<Type>(rb_expected_type);

    if (expected_type.getCategory() == Type::Enum)
        return INT2FIX( rb2cxx::enum_value(arg, static_cast<Enum const&>(expected_type)) );
    else if (expected_type.getCategory() == Type::Pointer)
    {
        // Build directly a DL::Ptr object, no need to build a Ruby Value wrapper
        Pointer const& ptr_type  = static_cast<Pointer const&>(expected_type);
        Type const& pointed_type = ptr_type.getIndirection();
        VALUE ptr = memory_allocate(pointed_type.getSize());

        Value typelib_value(memory_cptr(ptr), pointed_type);
        typelib_from_ruby(typelib_value, arg);
        return ptr;
    }
    return arg;
}

static 
VALUE filter_value_arg(VALUE self, VALUE arg, VALUE rb_expected_type)
{
    Type const& expected_type   = rb2cxx::object<Type>(rb_expected_type);
    Value const& arg_value      = rb2cxx::object<Value>(arg);
    Type const& arg_type        = arg_value.getType();     

    if (arg_type == expected_type)
    {
        if (arg_type.getCategory() == Type::Pointer)
        {
#ifdef      VERBOSE
            fprintf(stderr, "wrapping filtered argument with arg_type=%s\n", arg_type.getName().c_str());
#endif
            return memory_wrap(*reinterpret_cast<void**>(arg_value.getData()));
        }
	else if (arg_type.getCategory() == Type::Array)
	    return rb_funcall(arg, rb_intern("to_memory_ptr"), 0);
        else if (arg_type.getCategory() == Type::Numeric)
            return rb_funcall(arg, rb_intern("to_ruby"), 0);
        else
            return Qnil;
    }

    if (expected_type.getCategory() != Type::Pointer && expected_type.getCategory() != Type::Array)
        return Qnil;

    Indirect const& expected_indirect_type   = static_cast<Indirect const&>(expected_type);
    Type const& expected_pointed_type  = expected_indirect_type.getIndirection();

    // /void == /nil, so that if expected_type is null, then 
    // it is because the argument can hold any kind of pointers
    if (expected_pointed_type.isNull() || expected_pointed_type == arg_type)
        return rb_funcall(arg, rb_intern("to_memory_ptr"), 0);
    
    // There is only array <-> pointer conversion left to handle
    Indirect const& arg_indirect = static_cast<Indirect const&>(arg_type);
    if (arg_indirect.getIndirection() != expected_pointed_type)
	return Qnil;

    if (expected_type.getCategory() == Type::Pointer)
    {
	if (arg_type.getCategory() == Type::Pointer)
	{
	    // if it was OK, then arg_type == expected_type which is already handled
	    return Qnil;
	}
	return rb_funcall(arg, rb_intern("to_memory_ptr"), 0);
    }
    else
    {
	if (arg_type.getCategory() == Type::Pointer)
            return memory_wrap(*reinterpret_cast<void**>(arg_value.getData()));
	// check sizes
	Array const& expected_array = static_cast<Array const&>(expected_type);
	Array const& arg_array = static_cast<Array const&>(arg_type);
	if (expected_array.getDimension() > arg_array.getDimension())
	    return Qnil;
	return rb_funcall(arg, rb_intern("to_memory_ptr"), 0);
    }
}

void typelib_ruby::Typelib_init_functions()
{
    VALUE mTypelib  = rb_define_module("Typelib");
    rb_define_singleton_method(mTypelib, "filter_numeric_arg", RUBY_METHOD_FUNC(filter_numeric_arg), 2);
    rb_define_singleton_method(mTypelib, "filter_value_arg", RUBY_METHOD_FUNC(filter_value_arg), 2);

    cLibrary = rb_define_class_under(mTypelib, "Library", rb_cObject);
    rb_define_singleton_method(cLibrary, "wrap", RUBY_METHOD_FUNC(library_wrap), 2);
    rb_define_method(cLibrary, "find", RUBY_METHOD_FUNC(library_find), 1);

    cFunction = rb_define_class_under(mTypelib, "Function", rb_cObject);
    rb_define_private_method(cFunction, "prepare_vm", RUBY_METHOD_FUNC(function_compile), 1);

    cCallVM  = rb_define_class_under(mTypelib, "CallVM", rb_cObject);
    rb_define_method(cCallVM, "call", RUBY_METHOD_FUNC(vm_call), 1);
}

