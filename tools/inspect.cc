#include "inspect.hh"
#include "registry.hh"
#include "parsing.hh"

#include <iostream>
using namespace std;
using namespace Typelib;

Inspect::Inspect()
    : Mode("inspect") { }
    
bool Inspect::apply(int argc, char* const argv[])
{
    if (argc < 2)
    {
        help(cerr);
        return false;
    }

    std::string repository = argv[1];
    Registry registry;
    try { registry.load(repository); }
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
void Inspect::help(std::ostream& out) const
{
    out << "Usage: typelib inspect registry-file" << endl;
}


