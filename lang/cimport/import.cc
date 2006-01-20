#include "import.hh"
#include "registry.hh"

#include "CPPLexer.hh"
#include "typesolver.hh"

#include <fstream>
#include <iostream>

#include <utilmm/configfile/configset.hh>
#include <utilmm/system/process.hh>
#include <utilmm/system/system.hh>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

using namespace std;
using namespace antlr;
using namespace Typelib;

using boost::filesystem::path;

namespace 
{
    using namespace std;
    using namespace utilmm;
    using namespace boost::filesystem;

    path runcpp(path const& file, config_set const& config)
    {
        utilmm::process cpp;

        try { 
            tempfile tmpfile("typelib_cimport");
            path tmp_path = tmpfile.path();
            FILE* handle = tmpfile.handle();

            bool keep_output = config.get<bool>("keep_cpp_output");
            if (keep_output)
            {
                std::clog << "cpp output goes to " << tmp_path.native_file_string() << std::endl;
                tmpfile.detach();
            }

            cpp.push("cpp");
            cpp.push(file.native_file_string());

            // Build the command line for cpp
            typedef list<string> strlist;
            list<string> defines  = config.get< list<string> >("define");
            list<string> includes = config.get< list<string> >("include");
            list<string> rawflags = config.get< list<string> >("rawflag");

            for (strlist::const_iterator it = defines.begin(); it != defines.end(); ++it)
                cpp.push("-D" + *it);
            for (strlist::const_iterator it = includes.begin(); it != includes.end(); ++it)
                cpp.push("-I" + *it);
            for (strlist::const_iterator it = rawflags.begin(); it != rawflags.end(); ++it)
                cpp.push(*it);

            cpp.redirect_to(process::Stdout, handle, false);
            cpp.start();
            cpp.wait();
            if (cpp.exit_normal() && !cpp.exit_status())
            {
                if (!keep_output)
                    tmpfile.detach();
                return tmp_path;
            }
            
            return path();
        }
        catch(utilmm::unix_error) { return path(); }
    }

    bool parse(path const& file, Typelib::Registry& registry)
    {
        // Check that the input file can be opened
        ifstream s(file.native_file_string().c_str());
        if (!s)
        {
            cerr << file.native_file_string() << ": error: cannot open for reading\n";
            return false;
        }

        try {
            CPPLexer cpp_lexer(s);

            TypeSolver reader(cpp_lexer, registry);
            reader.init();
            reader.translation_unit();
        }
        catch (ANTLRException const& e) 
        { 
            cerr << file.native_file_string() << ": error: " << e.toString() << endl; 
            return false;
        }
        catch (Typelib::Undefined const& e)
        { 
            cerr << file.native_file_string() << ": error: " << e.getName() << " undefined" << endl; 
            return false;
        }
        catch(std::exception const& e)
        { 
            cerr << file.native_file_string() << ": error: internal error of type " 
                << typeid(e).name() << endl
                << file.native_file_string() << ": " << e.what() << endl;
            return false;
        }

        return true;
    }
}

bool CImport::load
    ( std::string const& file
    , utilmm::config_set const& config
    , Registry& registry)
{
    path temp;
    try
    {
        temp = runcpp(file, config);
        if (temp.empty())
        {
            cerr << "Error while running the preprocessor" << endl;
            return false;
        }

        bool retval = parse(temp, registry);
        remove(temp);
        return retval;
    }
    catch(...) 
    { 
        if (! temp.empty())
            remove(temp); 
        return false;
    }
}


