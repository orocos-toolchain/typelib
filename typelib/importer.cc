#include "importer.hh"
#include <iostream>
#include <fstream>
#include <sstream>

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
    : m_file(file), m_line(line), m_column(column), m_what(what_), m_buffer(0) {}
ImportError::~ImportError() throw() {} 

void ImportError::setFile(const std::string& path) { m_file = path; }
std::string ImportError::getFile() const { return m_file; }
int ImportError::getLine() const { return m_line; }
int ImportError::getColumn() const { return m_column; }

char const* ImportError::what() throw() {
    if (m_buffer)
        delete m_buffer;

    std::string msg = toString();
    size_t const length = msg.length();
    m_buffer = new char[length + 1];
    memcpy(m_buffer, &msg[0], length);
    m_buffer[length] = 0;
    return m_buffer;
}

std::string ImportError::toString () const
{ 
    std::ostringstream out;
    out << "error in " << m_file;
    if (m_line)
        out << ":" << m_line;
    if (m_column)
        out << ":" << m_column;
    if (!m_what.empty())
        out << ":" << m_what;
    return out.str();
}


