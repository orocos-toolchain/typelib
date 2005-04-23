#ifndef __GENOM_MODULE_H
#define __GENOM_MODULE_H

#include "registry.h"
#include <list>
#include <map>
#include <ostream>

class GenomModule : public Genom::Module
{
    friend class GenomModulePrivate;

    // Contains type definitions
    Typelib::Registry* m_registry;

public:
    typedef std::pair<std::string, std::string> StringPair;
    typedef std::list<std::string>              StringList;
    typedef std::map<std::string, std::string>  StringMap;

    typedef StringPair                          SDIRef;
    typedef std::list<SDIRef>                   SDIRefList;

    typedef Genom::Module   Module;
    typedef Genom::Request  Request;
    typedef Genom::Poster   Poster;
    typedef Genom::ExecTask ExecTask;

protected:
    void genomModule(const Module& module);
    void genomRequest(const Request& request);
    void genomPoster(const Poster& poster);
    void genomExecTask(const ExecTask& task);

private:
    typedef std::map<std::string, Request>      RequestMap;
    RequestMap m_requests;
    typedef std::map<std::string, Poster>       PosterMap;
    PosterMap m_posters;
    typedef std::map<std::string, ExecTask>     ExecTaskMap;
    ExecTaskMap m_tasks;

public:
    GenomModule(Typelib::Registry* registry);
    bool read(const std::string& path);

    const Typelib::Type*     getSDI() const;
    const Typelib::Registry* getRegistry() const;

    enum DumpMode
    {
        Default     = 0,
        DumpSDIType = 1
    };
    void dump(std::ostream& to, int mode = 0);
};

#endif

