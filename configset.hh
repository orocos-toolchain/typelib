#ifndef UTILMM_CONFIG_SET_HH
#define UTILMM_CONFIG_SET_HH

#include <map>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility/enable_if.hpp>

#include <list>

namespace utilmm
{
    /* To discriminate between list and non-list get<> */
    namespace details
    {
        template<typename T> struct is_list
        { static const bool value = false; };
        template<typename T> struct is_list< std::list<T> >
        { static const bool value = true; };
    }

    /** A scope in configuration files */
    class config_set 
        : private boost::noncopyable
    {
        friend class ConfigFile;

    private:
        typedef std::list<std::string> stringlist;
        template<typename T>
        static T convert(std::string const& value);
        
    protected:
        config_set* m_parent;
        typedef std::multimap<std::string, std::string> ValueMap;
        ValueMap m_values;
        typedef std::multimap<std::string, const config_set*> ChildMap;
        ChildMap m_children;

    protected:
        /** Clears this set */
        void clear();
        
    public:
        explicit config_set(config_set* parent = 0);
        ~config_set();
        
        typedef std::list<const config_set*> subsets;

        /** Tests for the existence of an attribute */
        bool exists (const std::string& attribute) const;
        
        /** The parent config_set object
         * @return the config_set object for our parent scope, or 0 if 
         * this scope is top-level */
        const config_set* parent() const;
        config_set* parent();

        /** Get the list of our children */
        std::list<const config_set*> children(const std::string& name) const;

        /** Get a list of values from this config_set */
        template<typename T> 
        T get(std::string const& name, T const& defval = T(),
                typename boost::enable_if< details::is_list<T> >::type *enabler = 0) const;
        /** Get a value from this config_set. If there is more than one value for this key,
         * then the first one is returned */
        template<typename T> 
        T get(std::string const& name, T const& defval = T(),
                typename boost::disable_if< details::is_list<T> >::type *enabler = 0) const;

        /** Set the value for this key */
        void set(std::string const& name, std::string const& value);
        /** Sets multiple values for this key */
        void set(std::string const& name, std::list<std::string> const& value);
        /** Insert a value in this config_set */
        void insert(std::string const& name, std::string const& value);
        /** Inserts multiple values in this config_set */
        void insert(std::string const& name, std::list<std::string> const& value);
        /** Add a child to this config_set */
        void insert(std::string const& name, config_set const* value);
        /** Remove the given option */
        void erase(std::string const& name);
    };

    namespace details {
        typedef std::list<std::string> stringlist;
    }

    template<> bool config_set::convert(std::string const& value);
    template<typename T> 
    T config_set::convert(const std::string& value)
    { return boost::lexical_cast<T>(value); }


    /** Define the basic access function: list of strings. Everything
     * is derived from this one and from the convert<> functions
     */
    template<>
    config_set::stringlist config_set::get(std::string const& name,
            config_set::stringlist const& defval,
            boost::enable_if< details::is_list<config_set::stringlist> >::type *enabler) const;

    /** Define accessors for lists */
    template<typename T> 
    T config_set::get(std::string const& name, T const& defval,
            typename boost::enable_if< details::is_list<T> >::type *enabler) const
    {
        stringlist values = get< stringlist >(name);
        if (values.empty())
            return defval;
        
        T result;
        for (stringlist::const_iterator it = values.begin(); it != values.end(); ++it)
            result.push_back(convert<typename T::value_type>(*it));

        return result;
    }

    /** Define accessor for scalars */
    template<typename T>
    T config_set::get(std::string const& name, T const& defval,
            typename boost::disable_if< details::is_list<T> >::type *enabler) const
    {   
        std::list<T> deflist;
        deflist.push_back(defval);
        return get< std::list<T> >(name, deflist).front();
    }
}

#endif



