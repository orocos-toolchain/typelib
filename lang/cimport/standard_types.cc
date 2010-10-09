#include "standard_types.hh"
#include "containers.hh"

void Typelib::CXX::addStandardTypes(Typelib::Registry& registry)
{
    if (!registry.has("/std/string"))
        registry.add(new String(registry));
}

