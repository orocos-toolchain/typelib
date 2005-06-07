#include "inspect.hh"
#include "registry.hh"

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

    return true;
}
void Inspect::help(std::ostream& out) const
{
    out << "Usage: typelib inspect registry-file" << endl;
}


