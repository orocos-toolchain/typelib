#ifndef __TYPELIB_PACKING_H__
#define __TYPELIB_PACKING_H__

#include "type.h"
#include <list>

namespace Typelib
{
    template<class Discover>
    struct packingof
    {
        typedef typename Discover::First    First;
        typedef typename Discover::Second   Second;

        static const int packing = offsetof(Discover, second);
    };

    template<typename A, typename B>
    struct discover
    {
        typedef A First;
        typedef B Second;

        A first;
        B second;
    };

    template<typename A, typename B>
    struct discover_arrays
    {
        typedef A  First;
        typedef B  Second;

        A first;
        B second[10];
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

