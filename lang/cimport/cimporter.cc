#include "cimporter.hh"
#include "registry.hh"

#include "CPPLexer.hh"
#include "typesolver.hh"

#include <fstream>
#include <iostream>

#include <utilmm/configfile/configset.hh>
#include <utilmm/system/tempfile.hh>
#include <utilmm/system/process.hh>

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

        // Get the temp file
        path tempfile;
        int handle = utilmm::tempfile("typelib_cimport", tempfile);
        if (handle == -1)
            return path();

        cpp.set_redirection(process::Stdout, tempfile.native_file_string(), handle, true);

        // Build the command line for cpp
        typedef list<string> strlist;
        list<string> defines = config.get_list_string("defines");
        list<string> includes = config.get_list_string("includes");

        for (strlist::const_iterator it = defines.begin(); it != defines.end(); ++it)
            cpp.push(" -D" + *it);
        for (strlist::const_iterator it = includes.begin(); it != includes.end(); ++it)
            cpp.push(" -I" + *it);

        cpp.start();
        cpp.wait();
        if (cpp.exit_normal())
            return tempfile;
        return path();
    }

    bool parse(path const& file, Typelib::Registry& registry)
    {
        try {
            ifstream s(file.native_file_string().c_str());
            if (!s)
            {
                cerr << "Input stream could not be opened for " << file.native_file_string() << "\n";
                exit(1);
            }

            CPPLexer cpp_lexer(s);
            cpp_lexer.setFilename(file.native_file_string());

            TypeSolver reader(cpp_lexer, registry);
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
}

bool CImporter::load
    ( std::string const& file
    , utilmm::config_set const& config
    , Registry& registry)
{
    path temp = runcpp(file, config);
    if (temp.empty())
        return false;

    bool retval = parse(temp, registry);
    remove(temp);
    return retval;
}


