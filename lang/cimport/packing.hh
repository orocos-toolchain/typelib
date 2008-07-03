#ifndef __TYPELIB_PACKING_H__
#define __TYPELIB_PACKING_H__

#include <stdexcept>

namespace Typelib
{
    class Compound;
    class Field;
    class Type;

    namespace Packing
    {
        struct PackingUnknown : std::runtime_error
        {
            PackingUnknown(std::string const& what)
                : std::runtime_error(what) {}
        };

        /** For now, you can't ask for the offset of an empy 
         * structure (struct {};) in a structure */
        struct FoundNullStructure : public PackingUnknown
        {
            FoundNullStructure()
                : PackingUnknown("queried the packing of a null type") {}
        };
        /** For now, you can't ask for the offset of an union 
         * in a structure */
        struct FoundUnion : public PackingUnknown
        {
            FoundUnion()
                : PackingUnknown("queried the packing of an union") {}
        };

        int getOffsetOf(Compound const& compound, Type const& append);
        int getOffsetOf(Field const& last_field, Type const& append);
        int getSizeOfCompound(Compound const& compound);
    }
};

#endif

