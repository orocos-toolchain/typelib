#ifndef __TYPEBUILDER_H__
#define __TYPEBUILDER_H__

#include <string>
#include <list>

#include "typemodel.hh"

namespace Typelib
{
    class Registry;

    /** Helper class to easily build a derived type (pointer, array) */
    class TypeBuilder
    {
        std::string m_basename;
        Type* m_type;

        struct Modifier
        {
            Type::Category category;
            int size; // Size of an array, deference count on multi-dim pointers
        };
        typedef std::list<Modifier> ModifierList;
        typedef std::pair<const Type*, ModifierList> TypeSpec;

        static TypeSpec parse(const Registry& registry, const std::string& full_name);
        static const Type& build(Registry& registry, const TypeSpec& spec, int size);

        Registry& m_registry;

    public:
        /** Initializes the type builder
         * This constructor builds the canonical name based on @c base
         * an gets its initial type from @c registry. It throws 
         * Undefined(typename) if @c base is not defined
         *
         * @arg registry the registry to act on
         * @arg base the base type
         */
        TypeBuilder(Registry& registry, const std::list<std::string>& base);

        /** Initializes the type builder
         * @arg registry the registry to act on
         * @arg base_type the base type
         */
        TypeBuilder(Registry& registry, const Type* base_type);

        /** Builds a level-deferenced pointer of the current type */
        void addPointer(int level);
        /** Add an outermost dimension to the current type (if it is not
	 * an array, builds an array */
        void addArrayMajor(int size);
        /** Add an innermost dimension to the current type (if it is not
	 * an array, builds an array */
        void addArrayMinor(int size);
        /** Sets the size of the current type */
        void setSize(int size);

        /** Get the current type */
        const Type& getType() const;

	/** Build a type from its full name 
	 * @return the new type or 0 if it can't be built */
        static const Type* build(Registry& registry, const std::string& full_name, int size = 0);
	/** Get base name, that is the type \c full_name is derived from */
        static std::string getBaseTypename(const std::string& full_name);
	/** Get base type, that is the type \c full_name is derived from
	 * @return the new type or 0 if it can't be built */
        static const Type* getBaseType(const Registry& registry, const std::string& full_name);
};
};

#endif

