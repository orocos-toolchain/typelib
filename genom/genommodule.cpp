#include "genommodule.h"

#include "CPPLexer.hpp"
#include "GenomLexer.hpp"
#include <antlr/TokenStreamSelector.hpp>

#include "GenomParser.hpp"

#include <fstream>
#include <iostream>

using namespace std;
using namespace antlr;

namespace
{
    class GenomModulePrivate : public GenomParser
    {
        ::GenomModule* m_reader;

    public:
        GenomModulePrivate(::GenomModule* reader, antlr::TokenStream& lexer)
            : GenomParser(lexer), m_reader(reader) {}

        void genomModule(const GenomModule& module)
        { m_reader -> genomModule(module); }
        void genomRequest(const GenomRequest& request)
        { m_reader -> genomRequest(request); }
        void genomPoster(const GenomPoster& poster)
        { m_reader -> genomPoster(poster); }
        void genomExecTask(const GenomExecTask& task)
        { m_reader -> genomExecTask(task); }
    };
}

void GenomModule::read(const std::string& file)
{
    try {
        ifstream s(file.c_str());
        if (!s)
        {
            cerr << "Input stream could not be opened for " << file << "\n";
            exit(1);
        }

        CPPLexer cpp_lexer(s);
        cpp_lexer.setFilename(file);

        GenomLexer gnm_lexer(cpp_lexer.getInputState());
        gnm_lexer.setFilename(file);

        TokenStreamSelector selector;
        selector.addInputStream(&gnm_lexer, "genom");
        selector.addInputStream(&cpp_lexer, "cpp");
        selector.select("genom");
        
        GenomModulePrivate reader(this, selector);
        reader.setRegistry(&m_registry);
        reader.setSelector(&selector);
        reader.translation_unit();
    }
    catch (ANTLRException& e) 
    { cerr << "parser exception: " << e.toString() << endl; }
    catch (TypeBuilder::NotFound& e)
    { cerr << "Type solver exception: " << e.toString() << endl; }
}

void GenomModule::genomModule(const Module& module)
{
    this->name  = module.name;
    this->id    = module.id;
    this->data  = module.data;
}
void GenomModule::genomRequest(const Request& request)
{ m_requests[request.name] = request; }
void GenomModule::genomPoster(const Poster& poster)
{ m_posters[poster.name] = poster; }
void GenomModule::genomExecTask(const ExecTask& task)
{ m_tasks[task.name] = task; }

const Type* GenomModule::getSDI() const
{ return m_registry.get(data); }

void GenomModule::dump(std::ostream& to)
{
    to << "Module " << name << " (" << id << "), sdi: " << data << endl;
    to << "SDI";
    to << getSDI() -> toString("\t") << endl;
        

    to << "Requests" << endl;
    for (RequestMap::const_iterator it = m_requests.begin(); it != m_requests.end(); ++it)
        to << "\t" << it -> first << endl;

    to << "Posters" << endl;
    for (PosterMap::const_iterator it = m_posters.begin(); it != m_posters.end(); ++it)
        to << "\t" << it -> first << endl;

    to << "Exec tasks" << endl;
    for (ExecTaskMap::const_iterator it = m_tasks.begin(); it != m_tasks.end(); ++it)
        to << "\t" << it -> first << endl;
}
    
