#ifndef __TYPELIB_PACKING_H__
#define __TYPELIB_PACKING_H__

namespace Typelib
{
    class Compound;
    class Field;

    namespace Packing
    {
        struct PackingUnknown {};

        /** For now, you can't ask for the offset of an empy 
         * structure (struct {};) in a structure */
        struct FoundNullStructure : public PackingUnknown {};
        /** For now, you can't ask for the offset of an union 
         * in a structure */
        struct FoundUnion : public PackingUnknown {};

        int getOffsetOf(Compound const& compound, Field const& append);
        int getOffsetOf(Field const& last_field, Field const& append);
    }
};

#endif

