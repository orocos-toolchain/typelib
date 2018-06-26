#include "importer.hh"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <boost/lexical_cast.hpp>

using namespace Typelib;
using namespace std;

void Importer::load
    ( std::string const& path
    , utilmm::config_set const& config
    , Registry& registry)
{
    // Check that the input file can be opened
    ifstream s(path.c_str());
    if (!s)
        throw ImportError(path, "cannot open for reading");
    load(s, config, registry);
}


ImportError::ImportError(const std::string& file, const std::string& what_, int line, int column)
    : std::runtime_error(file + ":" + boost::lexical_cast<string>(line) + ":" + what_)
    , m_file(file), m_line(line), m_column(column), m_what(what_), m_buffer(0) {}
ImportError::~ImportError() throw() {}

void ImportError::setFile(const std::string& path) { m_file = path; }
std::string ImportError::getFile() const { return m_file; }
int ImportError::getLine() const { return m_line; }
int ImportError::getColumn() const { return m_column; }


