#ifndef PARSING_H
#define PARSING_H

#include <sstream>
#include <string>

namespace Parsing
{
    struct ParsingError
    {
    private: 
        std::string m_file;
        int m_line, m_column;

    public:
        ParsingError(const std::string& file, int line = 0, int column = 0)
            : m_file(file), m_line(line), m_column(column) {}
        virtual ~ParsingError() {}

        void setFile(const std::string& path) { m_file = path; }
        std::string getFile() const { return m_file; }
        int getLine() const { return m_line; }
        int getColumn() const { return m_column; }

        virtual std::string toString () const
        { 
            std::ostringstream out;
            out << "Parsing error in " << m_file;
            if (m_line)
                out << " " << m_line;
            if (m_column)
                out << ":" << m_column;
            return out.str();
        }
    };

    struct MalformedXML : public ParsingError
    {
    public:
        MalformedXML(const std::string& file) 
            : ParsingError(file) {}
        virtual std::string toString() const
        { return "XML error in " + getFile(); }
    };

    struct UnexpectedElement : public ParsingError
    {
        std::string m_found, m_expected;
    public:
        UnexpectedElement(const std::string& file, const std::string& found, const std::string expected)
            : ParsingError(file), m_found(found), m_expected(expected) {}

        virtual std::string toString() const
        { 
            if (m_expected.empty())
                return "Unexpected element " + m_found + " in " + getFile();
            else
                return "Unexpected element " + m_found + " instead of " + m_expected + " in " + getFile();
        }
    };

    struct MissingAttribute : public ParsingError
    {
        std::string m_attribute;
    public:
        MissingAttribute(const std::string& file, const std::string& attribute)
            : ParsingError(file), m_attribute(attribute) {}

        virtual std::string toString() const
        { return "Missing attribute " + m_attribute + " in " + getFile(); }
    };
};

#endif

