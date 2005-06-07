#ifndef TYPELIB_REGISTRYITERATOR_HH
#define TYPELIB_REGISTRYITERATOR_HH

#include "registry.hh"
#include "typemodel.hh"
#include <boost/iterator/iterator_facade.hpp>

namespace Typelib
{
    class Registry;
    class RegistryIterator 
        : public boost::iterator_facade
            < RegistryIterator
            , Type const
            , boost::forward_traversal_tag >
    {
        friend class Registry;

    public:
        RegistryIterator(RegistryIterator const& other)
            : m_iter(other.m_iter) {}

        std::string getName() const { return m_iter->first; }
        std::string getSource() const { return m_iter->second.source_id; }
        bool isAlias() const { return m_iter->first != m_iter->second.type->getName(); }
        bool isPersistent() const { return m_iter->second.persistent; }

    private:
        typedef Registry::TypeMap::const_iterator BaseIter;
        BaseIter m_iter;
            
        explicit RegistryIterator(BaseIter init)
            : m_iter(init) {}
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

