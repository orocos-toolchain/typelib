
// NOP deleter, for Type objects and some Ptr objects
static void do_not_delete(void*) {}

namespace cxx2rb {
    template<typename T> static VALUE class_of();
    template<> static VALUE class_of<Registry>() { return cRegistry; }
    template<> static VALUE class_of<Type>()     { return rb_cClass; }
    template<> static VALUE class_of<Value>()    { return cType; }
}

namespace rb2cxx {
    static void check_is_kind_of(VALUE self, VALUE expected)
    {
        if (! rb_obj_is_kind_of(self, expected))
            rb_raise(rb_eTypeError, "expected %s, got %s", rb_class2name(expected), rb_obj_classname(self));
    }

    template<typename T>
    static T& get_wrapped(VALUE self)
    {
        void* object = 0;
        Data_Get_Struct(self, void, object);
        return *reinterpret_cast<T*>(object);
    }

    template<typename T> 
    static T& object(VALUE self)
    {
        check_is_kind_of(self, cxx2rb::class_of<T>());
        return get_wrapped<T>(self);
    }

    template<>
    static Type& object(VALUE self) 
    {
        check_is_kind_of(self, rb_cClass);
        VALUE type = rb_iv_get(self, "@type");
        return get_wrapped<Type>(type);
    }

}

template<typename T>
static VALUE are_wrapped_equal(VALUE rbself, VALUE rbwith)
{
    if (rb_obj_class(rbself) != rb_obj_class(rbwith))
        return Qfalse;

    T const& self(rb2cxx::object<T>(rbself));
    T const& with(rb2cxx::object<T>(rbwith));
    return (self == with) ? Qtrue : Qfalse;
}

/* Those are defined in visitors.hh */
static VALUE typelib_to_ruby(Value v, VALUE registry);
static VALUE typelib_to_ruby(Value value, VALUE name, VALUE registry);
static VALUE typelib_from_ruby(Value value, VALUE new_value);


