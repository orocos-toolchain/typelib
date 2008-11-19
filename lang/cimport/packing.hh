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

        /** Returns the offset, in bytes, of a new field of type \c append to be
         * appended after the given field. The field is expected to be aligned
         * on \c packing bytes.
         */
        int getOffsetOf(const Field& last_field, const Type& append_field, size_t packing);

        /** Returns the offset, in bytes, of a new field of type \c append to be
         * appended at the bottom of the given compound. The field is expected
         * to be aligned on \c packing bytes.
         */
        int getOffsetOf(Compound const& last_field, const Type& append_field, size_t packing);

        /** Returns the offset, in bytes, of a new field of type \c append to be
         * appended to the given compound. The alignment value is computed
         * automatically
         */
        int getOffsetOf(Compound const& compound, Type const& append);
        /** Returns the offset, in bytes, of a new field of type \c append to be
         * appended after the field \c last_field. The alignment value is
         * computed automatically
         */
        int getOffsetOf(Field const& last_field, Type const& append);
        /** Returns the size in bytes of \c compound, as the compiler would
         * round it
         */
        int getSizeOfCompound(Compound const& compound);
    }
};

#endif

