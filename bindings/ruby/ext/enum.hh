
namespace cxx2rb {
    /* Get the Ruby symbol associated with a C enum, or nil
     * if the value is not valid for this enum
     */
    VALUE enum_symbol(Enum::integral_type value, Enum const& e)
    {
        std::string symbol = e.get(value);
        try { return ID2SYM(rb_intern(symbol.c_str())); }
        catch(Enum::ValueNotFound) { return Qnil; }
    }
}

namespace rb2cxx {
    /* Get the C value associated with a ruby representation of an enum
     * Valid ruby representations are: symbols, strings and integer
     */
    Enum::integral_type enum_value(VALUE rb_value, Enum const& e)
    {
        // m_value can be either an integer, a symbol or a string
        if (TYPE(rb_value) == T_FIXNUM)
        {
            Enum::integral_type value = FIX2INT(rb_value);
            try { e.get(value); }
            catch(Enum::ValueNotFound)
            { rb_raise(rb_eArgError, "%i is not a valid value for %s", value, e.getName().c_str()); }

            return value;
        }

        char const* name;
        if (SYMBOL_P(rb_value))
            name = rb_id2name(SYM2ID(rb_value));
        else
            name = StringValuePtr(rb_value);

        try { return e.get(name); }
        catch(Enum::SymbolNotFound)
        { rb_raise(rb_eArgError, "%s is not a valid symbol for %s", name, e.getName().c_str()); }
    }
}

VALUE enum_keys(VALUE self)
{
    Enum const& type = static_cast<Enum const&>(rb2cxx::object<Type>(self));

    VALUE keys = rb_iv_get(self, "@values");
    if (!NIL_P(keys)) 
        return keys;

    keys = rb_hash_new();
    typedef std::list<std::string> string_list;
    string_list names = type.names();
    for (string_list::const_iterator it = names.begin(); it != names.end(); ++it)
        rb_hash_aset(keys, rb_str_new2(it->c_str()), INT2FIX(type.get(*it)));

    rb_iv_set(self, "@values", keys);
    return keys;
}

