
/*
 * Converts a Typelib::Type into its corresponding Ruby::DL spec
 *
 * Warnings:
 *   - Ruby::DL does not support long long
 *   - Because types in Ruby::DL are not of fixed sizes (think "char"),
 *     I cannot guarantee the behaviour of the Ruby::DL mapping
 */
class DLSpec : public TypeVisitor
{
    std::string m_spec;

    static char const* numeric_type_to_spec(Numeric const& type)
    {
        // Ruby/DL support for float and doubles on non-i386 machines is broken
        // see README
        if (type.getNumericCategory() != Numeric::Float)
        {

            switch(type.getSize())
            {
                case 1: return "C";
                case 2: return "H";
                case 4: return "L";
            }
        }

        throw UnsupportedType(type);
    }

    virtual bool visit_ (Numeric const& type)
    {
        m_spec = numeric_type_to_spec(type);
        return false;
    }
    virtual bool visit_ (Enum const& type)      
    { 
        BOOST_STATIC_ASSERT(( sizeof(Enum::integral_type) == sizeof(int) ));
        m_spec = "I"; 
        return false;
    }

    virtual bool visit_ (Pointer const& type)
    {
        m_spec = "P";
        return false;
    }
    virtual bool visit_ (Array const& type)
    { throw UnsupportedType(type); }

    virtual bool visit_ (Compound const& type)
    { throw UnsupportedType(type); }

public:

    // Returns the Ruby::DL spec for +type+
    // or throws UnsupportedType
    std::string apply(Type const& type)
    {
        m_spec = std::string();
        TypeVisitor::apply(type);
        if (m_spec.empty())
            throw UnsupportedType(type);
        return m_spec;
    }
};


/* Converts an array of wrapped Type objects into the corresponding Ruby::DL spec */
static std::string typelib_get_dl_spec(int argc, VALUE* argv)
{
    std::string spec;
    DLSpec converter;
    for (int i = 0; i < argc; ++i)
    {
        if (NIL_P(argv[i]))
            spec += "0";
        else
        {
            Type const& type(rb_type2cxx(argv[i]));
            spec += converter.apply(type);
        }
    }
    return spec;
}

