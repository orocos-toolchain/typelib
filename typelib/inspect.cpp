#include <stdlib.h>
#include "registry.h"
#include "parsing.h"

#include <iostream>
using namespace std;
using namespace Typelib;

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 2)
    {
        cerr << "Usage: typelib-inspect <repository>" << endl;
        exit(1);
    }
        
    std::string file = argv[1];
    Registry registry;
    try
    {
        registry.load(file);
    }
    catch(Parsing::ParsingError& error)
    {
        cerr << error.toString() << endl;
    }
    catch(Undefined error)
    {
        cerr << "Undefined type " << error.getName() << endl;
    }

    registry.dump(cout, Registry::AllType | Registry::WithSourceId);
    registry.save("/home/doudou/test-save.tlb");
}

