#include <iostream>
#include <cstdlib>

#include "inspect.hh"
#include "import.hh"
#include "typelib/pluginmanager.hh"

#include "genom.hh"

using namespace std;

namespace
{
    void usage()
    {
        cout << 
            "Usage: typelib [mode] [mode-args]\n"
            "\twhere mode is one of: inspect, import\n"
            "\tcall typelib [mode] help for information on a particular mode" << endl;
        exit(1);
    }

    typedef map<string, Mode*> ModeMap;
    ModeMap modes;
    void registerMode(Mode* mode)
    {
        ModeMap::iterator it = modes.find(mode->getName());
        if (it != modes.end())
            delete it->second;
        modes[mode->getName()] = mode;
    }
}

int main(int argc, char** argv)
{
    // Instanciate the plugin manager, since everybody would want to use it
    Typelib::PluginManager::self the_manager;
    
    registerMode(new Inspect);
    Mode* regmode = new Import;
    registerMode(regmode);

    if (argc < 2) 
        usage();

    string mode_name = argv[1];
    ModeMap::iterator it = modes.find(mode_name);
    if (it == modes.end())
        usage();

    return 
        ((it->second) -> main(argc - 1, argv + 1) ? 1 : 0);
};

