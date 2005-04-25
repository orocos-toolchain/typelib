#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <list>

namespace utilmm
{
    class config_set;
}
namespace Typelib
{
    class Registry;
}

class Plugin
{
public:
    typedef std::list<std::string> OptionList;

public:
    Plugin(const std::string& name, const std::string& type);
    virtual ~Plugin(); 

    std::string getName() const;
    std::string getType() const;

    // Command-line options
    virtual OptionList getOptions() const;
    virtual bool apply(
              const OptionList& remaining
            , const utilmm::config_set& options
            , Typelib::Registry& registry) = 0;

private:
    std::string m_name;
    std::string m_type;
};

#endif

