#include "exporter.hh"
#include "registry.hh"
#include "registryiterator.hh"

using namespace Typelib;

bool Exporter::save(std::ostream& stream, Registry const& registry)
{
    if (! begin(stream, registry))
        return false;

    RegistryIterator const it_end(registry.end());
    for (RegistryIterator it = registry.begin(); it != it_end; ++it)
    {
        if (! it.isPersistent())
            continue;

        if (! save(stream, it))
            return false;
    }
    
    if (! end(stream, registry))
        return false;

    return true;
}

bool Exporter::begin(std::ostream& stream, Registry const& registry) 
{ return true; }
bool Exporter::end  (std::ostream& stream, Registry const& registry) 
{ return true; }

