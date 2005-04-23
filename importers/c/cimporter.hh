#ifndef __GENOM_MODULE_H
#define __GENOM_MODULE_H

#include "registry.h"
#include <list>
#include <map>
#include <ostream>

class CImporter : public Genom::Module
{
    // Contains type definitions
    Typelib::Registry* m_registry;

public:
    typedef std::pair<std::string, std::string> StringPair;
    typedef std::list<std::string>              StringList;
    typedef std::map<std::string, std::string>  StringMap;

    typedef StringPair                          SDIRef;
    typedef std::list<SDIRef>                   SDIRefList;

public:
    CImporter(Typelib::Registry* registry);
    bool read(const std::string& path);

    const Typelib::Registry* getRegistry() const;

    enum DumpMode
    {
        Default     = 0,
        DumpSDIType = 1
    };
    void dump(std::ostream& to, int mode = 0);
};

#endif

