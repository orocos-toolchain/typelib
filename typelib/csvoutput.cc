#include "csvoutput.hh"
#include "value.hh"
#include "typevisitor.hh"
#include <utilmm/stringlist.hh>
#include <boost/lexical_cast.hpp>

using namespace Typelib;
using namespace std;

namespace 
{
    using namespace Typelib;
    using namespace std;
    using namespace utilmm;
    class HeaderVisitor : public TypeVisitor
    {
        stringlist m_name;
        stringlist m_headers;

    protected:
        void output()
        {
            string name = join(m_name, ""); 
            m_headers.push_back(name);
        }
        bool visit_ (Numeric const&) { output(); return true; }
        bool visit_ (Enum const&) { output(); return true; }
             
        bool visit_ (Pointer const& type)
        { 
            m_name.push_front("*(");
            m_name.push_back(")");
            TypeVisitor::visit_(type);
            m_name.pop_front();
            m_name.pop_back();
            return true;
        }
        bool visit_ (Array const& type)
        {
            m_name.push_back("[");
            m_name.push_back("");
            m_name.push_back("]");
            stringlist::iterator count = m_name.end();
            --(--count);
            for (size_t i = 0; i < type.getDimension(); ++i)
            {
                *count = boost::lexical_cast<string>(i);
                TypeVisitor::visit_(type);
            }
            m_name.pop_back();
            m_name.pop_back();
            m_name.pop_back();
            return true;
        }

        bool visit_ (Compound const& type)
        {
            m_name.push_back(".");
            TypeVisitor::visit_(type);
            m_name.pop_back();
            return true;
        }
        bool visit_ (Compound const& type, Field const& field)
        {
            m_name.push_back(field.getName());
            TypeVisitor::visit_(type, field);
            m_name.pop_back();
            return true;
        }
 
    public:
        stringlist apply(Type const& type, std::string const& basename)
        {
            m_headers.clear();
            m_name.clear();
            m_name.push_back(basename);
            TypeVisitor::apply(type);
            return m_headers;
        }
    };

    class LineVisitor : public ValueVisitor
    {
        stringlist  m_output;
        
    protected:
        template<typename T>
        bool display(T value)
        {
            m_output.push_back(boost::lexical_cast<string>(value)); 
            return true;
        }
        bool visit_ (int8_t  & value) { return display(value); }
        bool visit_ (uint8_t & value) { return display(value); }
        bool visit_ (int16_t & value) { return display(value); }
        bool visit_ (uint16_t& value) { return display(value); }
        bool visit_ (int32_t & value) { return display(value); }
        bool visit_ (uint32_t& value) { return display(value); }
        bool visit_ (int64_t & value) { return display(value); }
        bool visit_ (uint64_t& value) { return display(value); }
        bool visit_ (float   & value) { return display(value); }
        bool visit_ (double  & value) { return display(value); }
        bool visit_ (Enum::integral_type& v, Enum const& e)
        { 
            try { m_output.push_back(e.get(v)); }
            catch(Typelib::Enum::ValueNotFound)
            { display(v); }
            return true;
        }

    public:
        stringlist apply(Value const& value)
        {
            m_output.clear();
            ValueVisitor::apply(value);
            return m_output;
        }
    };
}


CSVOutput::CSVOutput(Type const& type, std::string const& sep)
    : m_type(type), m_separator(sep) {}

/** Displays the header */
void CSVOutput::header(std::ostream& out, std::string const& basename)
{
    HeaderVisitor visitor;
    out << join(visitor.apply(m_type, basename), m_separator);
}

void CSVOutput::display(std::ostream& out, void* value)
{
    LineVisitor visitor;
    out << join(visitor.apply( Value(value, m_type) ), m_separator);
}

