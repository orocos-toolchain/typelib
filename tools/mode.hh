#ifndef MODE_H
#define MODE_H

#include <iosfwd>
#include <string>
#include <list>
#include <map>

class Plugin;
class Mode
{
protected:
    virtual bool apply(int argc, char* const argv[]) = 0;
    virtual void help(std::ostream& stream) const = 0;

    std::list<std::string> getPluginNames() const;
    Plugin* getPlugin(const std::string& name) const;

public:
    Mode(const std::string& name);
    virtual ~Mode();

    std::string getName() const;

    bool main(int argc, char* const argv[]);

    void addPlugin(Plugin* plugin);

private:
    std::string m_name;
    typedef std::map<std::string, Plugin *> PluginMap;
    PluginMap   m_plugins;
    
};

#endif

