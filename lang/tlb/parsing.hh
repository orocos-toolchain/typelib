#ifndef PARSING_H
#define PARSING_H

#include <sstream>
#include <string>

namespace Parsing
{
    class ParsingError
    {
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

    class MalformedXML : public ParsingError
    {
    public:
        MalformedXML(const std::string& file) 
            : ParsingError(file) {}

        virtual std::string toString() const
        { return "XML error in " + getFile(); }
    };

    class UnexpectedElement : public ParsingError
    {
        std::string m_found, m_expected;
    public:
        UnexpectedElement(const std::string& file, const std::string& found, const std::string expected)
            : ParsingError(file), m_found(found), m_expected(expected) {}

        std::string getFound() const    { return m_found; }
        std::string getExpected() const { return m_expected; }

        virtual std::string toString() const
        { 
            std::ostringstream out("Unexpected element ");
            out << m_found;
            if (!m_expected.empty()) out << " instead of " << m_expected;
            out << " in " + getFile();

            return out.str();
        }
    };

    class BadRootElement : public UnexpectedElement
    {
    public:
        BadRootElement(const std::string& file, const std::string& found, const std::string expected)
            : UnexpectedElement(file, found, expected) {}

        virtual std::string toString() const
        { 
            std::ostringstream out("Bad root element ");
            out << getFound();
            if (!getExpected().empty()) out << " instead of " << getExpected();
            out << " in " + getFile();

            return out.str();
        }
    };

    class MissingAttribute : public ParsingError
    {
        std::string m_attribute;
    public:
        MissingAttribute(const std::string& file, const std::string& attribute)
            : ParsingError(file), m_attribute(attribute) {}

        virtual std::string toString() const
        { return "Missing attribute '" + m_attribute + "' in " + getFile(); }
    };
};

#endif

