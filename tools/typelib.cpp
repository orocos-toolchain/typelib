#include <iostream>
#include <stdlib.h>

#include "inspect.h"

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
}
    
int main(int argc, char** argv)
{
    if (argc < 3) 
        usage();

    string mode = argv[1];

    if (mode == "inspect")
        return (inspect(argv[2]) ? 0 : 1);
    else if (mode == "register")
    {
        cerr << "Not implemented" << endl;
        return 1;
    }
    return 0;
};

