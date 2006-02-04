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

    static VALUE type_wrap(Type const& type, VALUE registry)
    {
        VALUE base  = class_of(type);
        VALUE klass = rb_class_new_instance(1, &base, rb_cClass);
        VALUE rb_type = Data_Wrap_Struct(rb_cObject, 0, do_not_delete, const_cast<Type*>(&type));
        rb_iv_set(klass, "@registry", registry);
        rb_iv_set(klass, "@type", rb_type);

        if (rb_respond_to(klass, rb_intern("subclass_initialize")))
            rb_funcall(klass, rb_intern("subclass_initialize"), 0);
            
        return klass;
    }
}

/**********************************************
 * Typelib::Type
 */
static VALUE type_is_a(VALUE self, Type::Category category)
{ 
    Type const& type(rb2cxx::object<Type>(self));
    return (type.getCategory() == category) ? Qtrue : Qfalse;
}
static VALUE type_is_null(VALUE self)    { return type_is_a(self, Type::NullType); }
static VALUE type_equality(VALUE rbself, VALUE rbwith)
{ return are_wrapped_equal<Type>(rbself, rbwith); }

static VALUE type_name(VALUE self)
{
    Type const& type = rb2cxx::object<Type>(self);
    return rb_str_new2(type.getName().c_str());
}

/* Only PODs are assignable, moreover pointers are followed */
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

