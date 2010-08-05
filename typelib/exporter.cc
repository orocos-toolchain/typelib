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

    std::set<Type const*> saved_types;

    typedef std::map
        < const std::string
        , RegistryIterator
        , bool (*) (const std::string&, const std::string&)
        >     TypeMap;
    TypeMap types(nameSort);
       
    RegistryIterator const it_end(registry.end());
    for (RegistryIterator it = registry.begin(); it != it_end; ++it)
        types.insert(make_pair(it.getName(), it));

    // Some exporters need to limit how many times a namespace appears. So, the
    // main algorithm (here) is meant to limit switching between namespaces.
    TypeMap free_types(nameSort);
    do
    {
        TypeMap::iterator it = types.begin();
        TypeMap::iterator const end = types.end();
        while (it != end)
        {
	    bool done_dependencies = false;
	    if (it->second.isAlias())
		done_dependencies = (saved_types.find(&(*it->second)) != saved_types.end());
	    else
	    {
                std::set<Type const*> dependencies = it->second->dependsOn();
		done_dependencies = includes(saved_types.begin(), saved_types.end(),
			dependencies.begin(), dependencies.end());
	    }

            if (done_dependencies)
            {
                free_types.insert(*it);
                types.erase(it++);
            }
            else ++it;
        }

        if (free_types.empty())
        {
            list<string> remaining;
            for (TypeMap::iterator it = types.begin(); it != types.end(); ++it)
                remaining.push_back(it->first);
            throw ExportError(join(remaining) + " seem to be recursive type(s). Exporting them is not supported yet");
        }

        it = free_types.begin();
        saved_types.insert(&(*it->second));
        if (it->second.isPersistent())
            save(stream, it->second);
        free_types.erase(it);
    }
    while (!types.empty());

    // Now save the remaining types in free_types
    for (TypeMap::iterator it = free_types.begin(); it != free_types.end(); ++it)
    {
        if (it->second.isPersistent())
            save(stream, it->second);
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

