#ifndef __TYPELIB_PACKING_H__
#define __TYPELIB_PACKING_H__

#include "type.h"
#include <list>

namespace Typelib
{
    template<class A, class B>
    struct packingof
    {
        typedef A    First;
        typedef B    Second;
        
        struct discover
        { A field0;
          B field1; };

        static const int packing = offsetof(discover, field1);
    };

    struct Packing
    {
        Type::Category category;
        size_t         size;

        int            packing;
    };

    struct PackingInfo
    {
        static std::list< Packing > packing;
        static void dump();

        static int  getOffsetOf(const Field& cur_field, const Field& append_field);
    };


    class PackingUnknown : public std::exception
    {
        const Type::Category m_category;
        const size_t         m_size;
    public:
        PackingUnknown(Type::Category cat, size_t size)
            : m_category(cat), m_size(size) {}
    };
};

#endif

