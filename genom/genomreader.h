#ifndef __GENOM_READER_H
#define __GENOM_READER_H

#include "registry.h"
#include <list>
#include <map>
#include <ostream>

namespace { class GenomModulePrivate; };

namespace Genom
{
    typedef std::list<std::string>              StringList;
    typedef std::map<std::string, std::string>  StringMap;

    typedef std::pair<std::string, std::string> SDIRef;
    typedef std::list<SDIRef>                   SDIRefList;

    struct Module
    {
        std::string name;
        int id;
        std::string data;
    };

    struct Request
    {
        std::string name, doc, type, task;
        SDIRef input, output;
        StringList posters;
        StringMap  functions;
        StringList fail_msg, incompat;
    };

    struct Poster
    {
        std::string name;

        enum UpdateType { Auto, User }; 
        UpdateType update;
        SDIRefList data;
        std::string exec_task;
        
        StringList typelist;
        std::string create_func;
    };

    struct ExecTask
    {
        std::string name;

        int period, delay, priority, stack_size;

        std::string c_init_func, c_func, c_end_func;
        StringList cs_client_from;
        SDIRefList poster_client_from;
        StringList fail_msg;
    };
};


class GenomModule : public Genom::Module
{
    friend class GenomModulePrivate;

    // Contains type definitions
    Registry m_registry;

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
    void read(const std::string& path);

    const Type* getSDI() const;

    void dump(std::ostream& to);
};

#endif

