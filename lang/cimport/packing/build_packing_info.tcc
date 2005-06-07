#ifndef PACKING_BUILD_PACKING_INFO_TCC
#define PACKING_BUILD_PACKING_INFO_TCC

namespace { namespace build_packing_info {

    struct nulltype { nulltype(int) {} };
    template<typename Previous, typename Type>
    struct builder : public Previous
    {
        builder(int index)
            : Previous(index + 1)
        {
            packing_info[index].size    = sizeof(Type);
            packing_info[index].packing = packingof< discover<char, Type> >::packing;
        }
    };

    typedef fold 
        < podlist
        , nulltype
        , builder<_1, _2>
        >::type        build_all;

    build_all   instantiate(0);
}}

#endif

