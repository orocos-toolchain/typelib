#ifndef TYPELIB_REGISTRYITERATOR_HH
#define TYPELIB_REGISTRYITERATOR_HH

#include "registry.hh"
#include "typemodel.hh"
#include "typename.hh"
#include <boost/iterator/iterator_facade.hpp>

namespace Typelib
{
    class Registry;

    /** Iterator on the types of the registry */
    class RegistryIterator
        : public boost::iterator_facade
            < RegistryIterator
            , Type const
            , boost::forward_traversal_tag >
    {
        friend class Registry;

    public:
        RegistryIterator(RegistryIterator const& other)
            : m_registry(other.m_registry), m_iter(other.m_iter) {}

        /** The type name */
        std::string getName() const { return m_iter->first; }
        /** The type name without the namespace */
        std::string getBasename() const { return getTypename(m_iter->first); }
        /** The type namespace */
        std::string getNamespace() const { return Typelib::getNamespace(m_iter->first); }
        /** The source ID for this type */
        std::string getSource() const { return m_iter->second.source_id; }
        /** Returns true if the type is an alias for another type */
        bool isAlias() const { return m_iter->first != m_iter->second.type->getName(); }
        /** Returns the aliased type if isAlias() is true */
        Type const& aliased() const { return *m_iter->second.type; }
        /** Returns true if the type should be saved on export. Derived types (arrays, pointers)
         * are not persistent */
        bool isPersistent() const { return m_iter->second.persistent; }

        Registry const& getRegistry() const { return m_registry; }

        Type& get_() { return *m_iter->second.type; }

    private:
        typedef Registry::TypeMap::const_iterator BaseIter;
        Registry const& m_registry;
        BaseIter m_iter;

        explicit RegistryIterator(Registry const& registry, BaseIter init)
            : m_registry(registry), m_iter(init) {}
        friend class boost::iterator_core_access;

        bool equal(RegistryIterator const& other) const
        { return other.m_iter == m_iter; }
        void increment()
        { ++m_iter; }
        Type const& dereference() const
        { return *m_iter->second.type; }
    };
}

#endif

