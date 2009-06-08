#include "exporter.hh"
#include "registry.hh"
#include "registryiterator.hh"
#include <utilmm/configfile/configset.hh>
#include <utilmm/stringtools.hh>
#include <typelib/typevisitor.hh>
#include <set>
#include <fstream>

using namespace Typelib;
using namespace std;
using utilmm::join;

void Exporter::save(std::string const& file_name, utilmm::config_set const& config, Registry const& registry)
{
    std::ofstream file(file_name.c_str(), std::ofstream::trunc);
    save(file, config, registry);
}

void Exporter::save(std::ostream& stream, utilmm::config_set const& config, Registry const& registry)
{
    begin(stream, registry);

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

		save(stream, it);
	    }
	}
    }

    if (done.size() != registry.size())
    {
        list<string> remaining;
	RegistryIterator const it_end(registry.end());
	for (RegistryIterator it = registry.begin(); it != it_end; ++it)
	{
	    if (done.find(it.getName()) == done.end())
                remaining.push_back(it.getName());
        }

        throw ExportError(join(remaining) + " seem to be (a) recursive type(s). Exporting them is not supported yet");
    }

    end(stream, registry);
}

bool Exporter::save( std::ostream& stream, Registry const& registry )
{
    utilmm::config_set config;
    try { save(stream, config, registry); }
    catch(UnsupportedType) { return false; }
    catch(ExportError) { return false; }
    return true;
}

void Exporter::begin(std::ostream& stream, Registry const& registry) {}
void Exporter::end  (std::ostream& stream, Registry const& registry) {}

