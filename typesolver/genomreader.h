#ifndef __GENOM_READER_H
#define __GENOM_READER_H

#include "GenomParser.hpp"
#include "registry.h"

namespace { class GenomReaderPrivate; }

class GenomReader
{
    friend class GenomReaderPrivate;
    Registry m_registry;

public:
    typedef GenomParser::GenomModule  GenomModule;
    typedef GenomParser::GenomRequest GenomRequest;
    typedef GenomParser::GenomPoster GenomPoster;
    typedef GenomParser::GenomExecTask GenomExecTask;

protected:
    void genomModule(const GenomModule& module);
    void genomRequest(const GenomRequest& request);
    void genomPoster(const GenomPoster& poster);
    void genomExecTask(const GenomExecTask& task);

public:
    void read(const std::string& path);
};

#endif

