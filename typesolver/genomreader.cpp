#include "genomreader.h"

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
    class GenomReaderPrivate : public GenomParser
    {
        GenomReader* m_reader;

    public:
        GenomReaderPrivate(GenomReader* reader, antlr::TokenStream& lexer)
            : GenomParser(lexer), m_reader(reader) {}

        void genomModule(const GenomReader::GenomModule& module)
        { m_reader -> genomModule(module); }
        void genomRequest(const GenomReader::GenomRequest& request)
        { m_reader -> genomRequest(request); }
        void genomPoster(const GenomPoster& poster)
        { m_reader -> genomPoster(poster); }
        void genomExecTask(const GenomExecTask& task)
        { m_reader -> genomExecTask(task); }
    };
}

void GenomReader::read(const std::string& file)
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
        
        GenomReaderPrivate reader(this, selector);
        reader.setRegistry(&m_registry);
        reader.setSelector(&selector);
        reader.translation_unit();
    }
    catch (ANTLRException& e) 
    { cerr << "parser exception: " << e.toString() << endl; }
    catch (TypeBuilder::NotFound& e)
    { cerr << "Type solver exception: " << e.toString() << endl; }
}

void GenomReader::genomModule(const GenomModule& module)
{
    cout << "New module " << module.name << endl
        << "\tid: " << module.id << endl
        << "\tdata: " << module.data << endl;
}
void GenomReader::genomRequest(const GenomRequest& request)
{
    cout << "New request " << request.name << endl
        << "\tdoc: " << request.doc << endl
        << "\ttype: " << request.type << endl;
}

void GenomReader::genomPoster(const GenomPoster& poster)
{
    cout << "New poster " << poster.name << endl
        << "\tupdate: " << (poster.update == GenomPoster::Auto ? "auto" : "user") << endl
        << "\texec_task: " << poster.exec_task << endl;
}

void GenomReader::genomExecTask(const GenomExecTask& task)
{
    cout << "New task " << task.name << endl
        << "\tperiod: " << task.period << endl;
}

