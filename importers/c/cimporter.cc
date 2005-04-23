#include "CPPLexer.hh"
#include "CPPParser.hh"
#include <antlr/TokenStreamSelector.hpp>

#include <fstream>
#include <iostream>

using namespace std;
using namespace antlr;
using namespace Typelib;

namespace
{
}

CImporter::CImporter(Registry* registry)
    : m_registry(registry) {}

bool CImporter::read(const std::string& file)
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

        TypeSolver reader(&cpp_lexer, m_registry);
      // reader.setRegistry(m_registry);
      // reader.setSelector(&selector);
        reader.translation_unit();
    }
    catch (ANTLRException& e) 
    { 
        cerr << "parser exception: " << e.toString() << endl; 
        return false;
    }
    catch (Typelib::Undefined& e)
    { 
        cerr << "Undefined type found " << e.getName() << endl; 
        return false;
    }

    return true;
}

const Type* CImporter::getSDI() const { return m_registry->get(data); }
const Registry* CImporter::getRegistry() const { return m_registry; }

void CImporter::dump(std::ostream& to, int mode)
{
}
    
