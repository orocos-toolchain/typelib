#include "registry.h"
#include "parsing.h"

#include <iostream>
using namespace std;
using namespace Typelib;

bool inspect(const std::string& repository)
{
    Registry registry;
    try
    {
        registry.load(repository);
    }
    catch(Parsing::ParsingError& error)
    {
        cerr << error.toString() << endl;
        return false;
    }
    catch(Undefined error)
    {
        cerr << "Undefined type " << error.getName() << endl;
        return false;
    }

    registry.dump(cout, Registry::AllType | Registry::WithSourceId);
    return true;
}

