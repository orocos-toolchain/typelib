#include <iostream>
#include <stdlib.h>

#include "inspect.h"
#include "import.h"

#include "genom.h"

using namespace std;

namespace
{
    void usage()
    {
        cout << 
            "Usage: typelib [mode] [mode-args]\n"
            "\twhere mode is one of: inspect, register\n"
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
    registerMode(new Inspect);
    Mode* regmode = new Import;
    registerMode(regmode);
    regmode -> addPlugin( new GenomPlugin );

    if (argc < 2) 
        usage();

    string mode_name = argv[1];
    ModeMap::iterator it = modes.find(mode_name);
    if (it == modes.end())
        usage();

    return 
        ((it->second) -> main(argc - 1, argv + 1) ? 1 : 0);
};

