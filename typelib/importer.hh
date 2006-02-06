#ifndef TYPELIB_IMPORTER_HH
#define TYPELIB_IMPORTER_HH

#include <string>

namespace utilmm { class config_set; }

namespace Typelib
{
    class Registry;

    class ReadError : public std::exception {};
    
    class Importer
    {
    public:
        virtual ~Importer() {}

        virtual void load
            ( std::istream& stream
            , utilmm::config_set const& config
            , Registry& registry) = 0;

        virtual void load
            ( std::string const& path
            , utilmm::config_set const& config
            , Registry& registry);
    };

    class ImportError : public std::exception
    {
        std::string m_file;
        int m_line, m_column;
        std::string m_what;
        char* m_buffer;

    public:
        ImportError(const std::string& file, const std::string& what_ = "", int line = 0, int column = 0);
        virtual ~ImportError() throw();

        void setFile(const std::string& path);
        std::string getFile() const;
        int getLine() const;
        int getColumn() const;

        virtual char const* what() throw();
        virtual std::string toString () const;
    };

}

#endif

