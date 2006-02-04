
/**********************************************
 * Typelib::Compound
 */

namespace rb2cxx {
    static Field const* get_field(VALUE self, VALUE field_name)
    {
        Type const& type(rb2cxx::object<Type>(self));
        Compound const& compound = static_cast<Compound const&>(type);
        return compound.getField(StringValuePtr(field_name));
    }
}

static VALUE compound_has_field(VALUE self, VALUE field_name)
{ return rb2cxx::get_field(self, field_name) ? Qtrue : Qfalse; }
static VALUE compound_get_field(VALUE self, VALUE field_name)
{
    Field const* field = rb2cxx::get_field(self, field_name);
    if (! field)
    {
        rb_raise(rb_eNoMethodError, "no such field '%s' in '%s'", 
                StringValuePtr(field_name), 
                rb2cxx::object<Type>(self).getName().c_str());
    }

    VALUE registry = rb_iv_get(self, "@registry");
    return cxx2rb::type_wrap(field->getType(), registry);
}

static VALUE compound_get_fields(VALUE self)
{
    Type const& type(rb2cxx::object<Type>(self));
    Compound const& compound(dynamic_cast<Compound const&>(type));
    Compound::FieldList const& fields(compound.getFields());
    Compound::FieldList::const_iterator it, end = fields.end();

    VALUE registry = rb_iv_get(self, "@registry");
    VALUE fieldlist = rb_ary_new();
    for (it = fields.begin(); it != end; ++it)
    {
        VALUE field_name = rb_str_new2(it->getName().c_str());
        VALUE field_type = cxx2rb::type_wrap(it->getType(), registry);

        VALUE field_def = rb_ary_new2(3);
        rb_ary_store(field_def, 0, field_name);
        rb_ary_store(field_def, 1, INT2FIX(it->getOffset()));
        rb_ary_store(field_def, 2, field_type);
        rb_ary_push(fieldlist, field_def);
    }

    return fieldlist;
}

