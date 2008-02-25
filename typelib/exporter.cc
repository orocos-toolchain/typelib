#include "exporter.hh"
#include "registry.hh"
#include "registryiterator.hh"
#include <set>

using namespace Typelib;

bool Exporter::save(std::ostream& stream, Registry const& registry)
{
    if (! begin(stream, registry))
        return false;

    std::set<std::string> done;
    std::set<Type const*> saved_types;

    bool done_something = true;
    while (done_something)
    {
	done_something = false;

	RegistryIterator const it_end(registry.end());
	for (RegistryIterator it = registry.begin(); it != it_end; ++it)
	{
	    if (done.find(it.getName()) != done.end())
		continue;

	    bool done_dependencies = false;
	    if (it.isAlias())
		done_dependencies = (saved_types.find(&(*it)) != saved_types.end());
	    else
	    {
		std::set<Type const*> dependencies = it->dependsOn();
		done_dependencies = includes(saved_types.begin(), saved_types.end(),
			dependencies.begin(), dependencies.end());
	    }

	    if (done_dependencies)
	    {
		done.insert(it.getName());
		saved_types.insert(&(*it));
		done_something = true;

		if (! it.isPersistent())
		    continue;

		if (! save(stream, it))
		    return false;
	    }
	}
    }

    if (! end(stream, registry))
        return false;

    return true;
}

bool Exporter::begin(std::ostream& stream, Registry const& registry) 
{ return true; }
bool Exporter::end  (std::ostream& stream, Registry const& registry) 
{ return true; }

