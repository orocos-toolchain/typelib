#ifndef PARSING_H
#define PARSING_H

#include <sstream>
#include <string>

namespace Parsing
{
    using Typelib::ImportError;
    class MalformedXML : public ImportError
    {
    public:
        MalformedXML(const std::string& file = "") 
            : ImportError(file, "malformed XML") {}
    };

    class UnexpectedElement : public ImportError
    {
        std::string m_found, m_expected;
    public:
        UnexpectedElement(const std::string& found_, const std::string expected_, const std::string& file = "")
            : ImportError(file, "unexpected element " + found_ + ", was expecting " + expected_)
            , m_found(found_), m_expected(expected_) { }
        ~UnexpectedElement() throw() {}

        std::string found() const    { return m_found; }
        std::string expected() const { return m_expected; }
    };

    class BadRootElement : public ImportError
    {
        std::string m_found, m_expected;
    public:
        BadRootElement(const std::string& found_, const std::string expected_, const std::string& file = "")
            : ImportError(file, "this document is not a Typelib type library: found " + found_ + " instead of " + expected_)
            , m_found(found_), m_expected(expected_) {}
        ~BadRootElement() throw() {}

        std::string found() const    { return m_found; }
        std::string expected() const { return m_expected; }
    };

    class MissingAttribute : public ImportError
    {
        std::string m_attribute;
    public:
        MissingAttribute(const std::string& attribute, const std::string& file = "")
            : ImportError(file, "missing attribute " + attribute), m_attribute(attribute) {}
        ~MissingAttribute() throw() {}
    };
};

#endif

