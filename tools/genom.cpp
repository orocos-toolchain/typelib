#include "genom.h"

#include <iostream>

#include "genommodule.h"
#include "preprocess.h"

#include "configset.h"

#include <boost/tokenizer.hpp>

using namespace std;
using namespace Utils;
using namespace boost;
using Typelib::Registry;

GenomPlugin::GenomPlugin()
    : Plugin("genom", Register) {}

list<string> GenomPlugin::getOptions() const
{
    static const char* arguments[] = 
    { "*:I:include=string|include search path" };
    return list<string>(arguments, arguments + 1);
}

bool GenomPlugin::apply(const OptionList& remaining, const ConfigSet& options, Registry* registry)
{
    if (remaining.empty())
    {
        cerr << "No file found on command line. Aborting" << endl;
        return false;
    }

    list<string> cppargs;
    
    string includes = options.getString("include");
    if (! includes.empty())
    {
        // split at ':'
        
        typedef tokenizer< char_separator<char> >  Splitter;
        Splitter splitter(includes, char_separator<char> (":"));
        for (Splitter::const_iterator it = splitter.begin(); it != splitter.end(); ++it)
            cppargs.push_back("-I" + *it);
    }

    string file   = remaining.front();
    string i_file = preprocess(file, cppargs);
    if (i_file.empty())
    {
        cerr << "Could not preprocess " << file << ", aborting" << endl;
        return false;
    }


    try
    {
        GenomModule reader(registry);
        int old_count = registry -> getCount();

        if (reader.read(i_file))
        {
            // Dump type list
            cout << "Found " << registry -> getCount() - old_count << " types in " << file << ":" << endl;
            registry -> dump(std::cout, Registry::AllType, "*");

            reader.dump(cout);
        }
        return true;
    }
    catch(Typelib::RegistryException& e)
    {
        cerr << "Error in type management: " << e.toString() << endl;
        return false;
    }
    catch(std::exception& e)
    {
        cerr << "Error parsing file " << file << ":\n\t"
            << typeid(e).name() << endl;
        return false;
    }
}

