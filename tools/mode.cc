#include "mode.hh"
#include "plugin.hh"
#include <iostream>

using namespace std;

Mode::Mode(const std::string& name) 
    : m_name(name) {}

Mode::~Mode() {}

std::string Mode::getName() const { return m_name; }

bool Mode::main(int argc, char* const argv[])
{
    if (argc > 1 && argv[1] == string("help"))
    {
        help(cout);
        return true;
    }
    return apply(argc, argv);
}

list<string> Mode::getPluginNames() const
{
    list<string> ret;
    for (PluginMap::const_iterator it = m_plugins.begin(); it != m_plugins.end(); ++it)
        ret.push_back(it->first);
    return ret;
}
Plugin* Mode::getPlugin(const std::string& name) const
{ 
    PluginMap::const_iterator it = m_plugins.find(name);
    return (it == m_plugins.end()) ? 0 : it->second;
}
void    Mode::addPlugin(Plugin* plugin)
{ 
    string name(plugin->getName());
    PluginMap::iterator it = m_plugins.find(name);
    if (it != m_plugins.end()) delete it->second;
    m_plugins[name] = plugin;
}


