#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <list>

namespace Utils
{
    class ConfigSet;
}
namespace Typelib
{
    class Registry;
}

class Plugin
{
public:
    enum Type
    {
        Register
    };
    typedef std::list<std::string> OptionList;

public:
    Plugin(const std::string& name, Type type);
    virtual ~Plugin(); 

    std::string getName() const;
    Type        getType() const;

    // Command-line options
    virtual OptionList getOptions() const;
    virtual bool       apply(
              const OptionList& remaining
            , const Utils::ConfigSet& options
            , Typelib::Registry* registry) = 0;

private:
    std::string m_name;
    Type        m_type;
};

#endif


