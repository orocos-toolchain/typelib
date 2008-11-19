#include <typelib/registry.hh>

#include <CPPLexer.hpp>
#include "typesolver.hh"
#include "import.hh"

#include <fstream>
#include <iostream>
#include <utilmm/stringtools.hh>

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

            bool debug = config.get<bool>("debug");

            if (debug)
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
            list<string> rawflags = config.get< list<string> >("rawflags");
            { // for backward compatibility
                list<string> rawflag = config.get< list<string> >("rawflag");
                rawflags.splice(rawflags.end(), rawflag);
            }

            for (strlist::const_iterator it = defines.begin(); it != defines.end(); ++it)
                cpp.push("-D" + *it);
            for (strlist::const_iterator it = includes.begin(); it != includes.end(); ++it)
                cpp.push("-I" + *it);
            for (strlist::const_iterator it = rawflags.begin(); it != rawflags.end(); ++it)
                cpp.push(*it);

            if (debug)
            {
                std::clog << "will run the following command: \n"
                    << "  " << utilmm::join(cpp.cmdline(), " ") << std::endl;
            }


            cpp.redirect_to(process::Stdout, handle, false);
            cpp.start();
            cpp.wait();
            if (cpp.exit_normal() && !cpp.exit_status())
            {
                if (!debug)
                    tmpfile.detach();
                return tmp_path;
            }
            
            return path();
        }
        catch(utilmm::unix_error) { return path(); }
    }
}

void CImport::load
    ( std::istream& stream
    , utilmm::config_set const& config
    , Registry& registry )
{
    try {
        CPPLexer cpp_lexer(stream);

        TypeSolver reader(cpp_lexer, registry, config.get<bool>("cxx", true));
        reader.setupOpaqueHandling(config.get<bool>("opaques_forced_alignment", true),
                config.get<bool>("opaques_ignore", false));

        list<config_set const*> opaque_defs = config.children("opaques");
        for (list<config_set const*>::const_iterator it = opaque_defs.begin(); it != opaque_defs.end(); ++it)
        {
            reader.defineOpaqueAlignment((*it)->get<string>("name"),
                    (*it)->get<size_t>("alignment"));
        }

        reader.init();
        reader.translation_unit();
    }
    catch (ANTLRException const& e) 
    { throw ImportError("", "syntax error: " + e.toString()); }
}
    

void CImport::load
    ( std::string const& file
    , utilmm::config_set const& config
    , Registry& registry)
{
    path temp;
    try
    {
        temp = runcpp(utilmm::clean_path(file), config);
        if (temp.empty())
            throw ImportError("", "error running the preprocessor");

        // Check that the input file can be opened
        ifstream s(temp.native_file_string().c_str());
        if (!s)
            throw ImportError("", "cannot open for reading");

        load(s, config, registry);
        remove(temp);
    }
    catch(...) 
    { 
        if (! temp.empty())
            remove(temp); 
        throw;
    }
}

