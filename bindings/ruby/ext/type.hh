namespace cxx2rb {
    static VALUE class_of(Type const& type)
    {
        switch(type.getCategory()) {
            case Type::Compound:    return cCompound;
            case Type::Pointer:     return cPointer;
            case Type::Array:       return cArray;
            case Type::Enum:        return cEnum;
            default:                return cType;
        }
    }

    typedef std::map<Type const*, VALUE> WrapperMap;
    static VALUE type_wrap(Type const& type, VALUE registry)
    {
        VALUE known_types = rb_iv_get(registry, "@wrappers");
        if (NIL_P(known_types))
            rb_raise(rb_eArgError, "@wrappers is uninitialized");

	// Type objects are unique, we can register Ruby wrappers based
	// on the type pointer (instead of using names)
	WrapperMap& wrappers = rb2cxx::get_wrapped<WrapperMap>(known_types);

	WrapperMap::const_iterator it = wrappers.find(&type);
	if (it != wrappers.end())
	    return it->second;

        VALUE base  = class_of(type);
        VALUE klass = rb_class_new_instance(1, &base, rb_cClass);
        VALUE rb_type = Data_Wrap_Struct(rb_cObject, 0, do_not_delete, const_cast<Type*>(&type));
        rb_iv_set(klass, "@registry", registry);
        rb_iv_set(klass, "@type", rb_type);
        rb_iv_set(klass, "@name", rb_str_new2(type.getName().c_str()));
	rb_iv_set(klass, "@null", (type.getCategory() == Type::NullType) ? Qtrue : Qfalse);

        if (rb_respond_to(klass, rb_intern("subclass_initialize")))
            rb_funcall(klass, rb_intern("subclass_initialize"), 0);

	wrappers.insert(std::make_pair(&type, klass));
        return klass;
    }
}

/**********************************************
 * Typelib::Type
 */
static VALUE type_equality(VALUE rbself, VALUE rbwith)
{ return are_wrapped_equal<Type>(rbself, rbwith); }

/* PODs are assignable, pointers are dereferenced */
static VALUE type_is_assignable(Type const& type)
{
    switch(type.getCategory())
    {
    case Type::Numeric:
        return INT2FIX(1);
    case Type::Pointer:
        return type_is_assignable( dynamic_cast<Pointer const&>(type).getIndirection());
    case Type::Enum:
        return INT2FIX(1);
    default:
        return INT2FIX(0);
    }
    // never reached
}

