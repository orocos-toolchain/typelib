#ifndef UTILMM_CONFIG_SET_HH
#define UTILMM_CONFIG_SET_HH

#include <map>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility/enable_if.hpp>

#include <list>

namespace utilmm {
/* To discriminate between list and non-list get<> */
namespace details {
template <typename T> struct is_list { static const bool value = false; };
template <typename T> struct is_list<std::list<T> > {
    static const bool value = true;
};
}

/** A scope in configuration files */
class config_set : private boost::noncopyable {
    friend class ConfigFile;

  private:
    typedef std::list<std::string> stringlist;
    template <typename T> static T convert(std::string const &value);

  protected:
    config_set *m_parent;
    typedef std::multimap<std::string, std::string> ValueMap;
    ValueMap m_values;
    typedef std::multimap<std::string, const config_set *> ChildMap;
    ChildMap m_children;

  protected:
    /** Clears this set */
    void clear();

  public:
    explicit config_set(config_set *parent = 0);
    ~config_set();

    typedef std::list<const config_set *> subsets;

    /** Checks if this set is empty (no child, no attributes) */
    bool empty() const;

    /** Tests for the existence of an attribute */
    bool exists(const std::string &attribute) const;

    /** The parent config_set object
     * @return the config_set object for our parent scope, or 0 if
     * this scope is top-level */
    const config_set *parent() const;
    config_set *parent();

    /** Get the list of children named \c name */
    std::list<const config_set *> children(const std::string &name) const;
    /** Get the first child named \c name or an empty set */
    config_set const &child(std::string const &name) const;

    /** In a config_set object, all values are stored as lists of strings. This
     * method converts the stored value into the required type using the
     * associated config_set::convert template specialization (if it exists).
     *
     * If no value is stored under the given key, returns \c defval
     */
    template <typename T>
    T get(std::string const &name, T const &defval = T(),
          typename boost::enable_if<details::is_list<T> >::type *
              enabler = 0) const;
    /** In a config_set object, all values are stored as lists of strings. This
     * method is a convenience method when only one value is stored for a given
     * key.  It converts the stored value into the required type using the
     * associated config_set::convert template specialization (if it exists).
     *
     * If no value is associated with the key, returns \c defval
     */
    template <typename T>
    T get(std::string const &name, T const &defval = T(),
          typename boost::disable_if<details::is_list<T> >::type *
              enabler = 0) const;

    /** Replaces any value associated with \c name with the value provided.
     * See \c insert to append new values to already existing ones
     */
    void set(std::string const &name, std::string const &value);
    /** Sets multiple values for this key. If some value was already
     * associated with \c name, the new value replaces it. See \c insert to
     * append new values to already existing ones.
     */
    void set(std::string const &name, std::list<std::string> const &value);
    /** Appends the given value to the values already associated with \c name */
    void insert(std::string const &name, std::string const &value);
    /** Appends the given values to the valus already associated with \c name */
    void insert(std::string const &name, std::list<std::string> const &value);
    /** Add a child to this config_set */
    void insert(std::string const &name, config_set const *value);
    /** Remove the given option */
    void erase(std::string const &name);
};

namespace details {
typedef std::list<std::string> stringlist;
}

template <> bool config_set::convert(std::string const &value);
template <typename T> T config_set::convert(const std::string &value) {
    return boost::lexical_cast<T>(value);
}

template <>
config_set::stringlist config_set::get(
    std::string const &name, config_set::stringlist const &defval,
    boost::enable_if<details::is_list<config_set::stringlist> >::type *enabler)
    const;

template <typename T>
T config_set::get(
    std::string const &name, T const &defval,
    typename boost::enable_if<details::is_list<T> >::type *enabler) const {
    stringlist values = get<stringlist>(name);
    if (values.empty())
        return defval;

    T result;
    for (stringlist::const_iterator it = values.begin(); it != values.end();
         ++it)
        result.push_back(convert<typename T::value_type>(*it));

    return result;
}

template <typename T>
T config_set::get(
    std::string const &name, T const &defval,
    typename boost::disable_if<details::is_list<T> >::type *enabler) const {
    std::list<T> deflist;
    deflist.push_back(defval);
    return get<std::list<T> >(name, deflist).front();
}
}

#endif
