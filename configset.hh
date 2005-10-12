#ifndef UTILMM_CONFIG_SET_HH
#define UTILMM_CONFIG_SET_HH

#include <boost/noncopyable.hpp>
#include <map>
#include <string>

#include <list>

namespace utilmm
{
    /** A scope in configuration files */
    class config_set 
        : private boost::noncopyable
    {
        friend class ConfigFile;

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
        const config_set* get_parent() const;
        config_set* get_parent();

        /** Get the list of our children scopes */
        std::list<const config_set*> get_children(const std::string& name) const;

        std::list<std::string> get_list_string(const std::string& name) const;
        std::list<int>         get_list_int(const std::string& name) const;
        std::list<bool>        get_list_bool(const std::string& name) const;
        int         get_int(const std::string& name, int default_value = 0) const;
        std::string get_string(const std::string& name, const std::string& default_value = "") const;
        bool        get_bool(const std::string& name, bool default_value = false) const;

        void insert(std::string const& name, std::string const& value);
        void insert(std::string const& name, config_set const* value);
    };
}

#endif



