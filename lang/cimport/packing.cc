#include "packing.hh"
#include <typelib/typemodel.hh>
#include <typelib/typevisitor.hh>
#include <boost/lexical_cast.hpp>
#include <typelib/typedisplay.hh>


////////////////////////////////////////////////////////////////////////////////
//
// Check some assumptions we make in the packing code
//

#include <boost/mpl/size.hpp>
#include "packing/tools.tcc"
#include "packing/check_arrays.tcc"
#include "packing/check_struct_in_struct.tcc"
#include "packing/check_size_criteria.tcc"

#include <vector>
#include <set>
#include <map>

namespace
{
    struct PackingInfo
    {
        size_t size;
        size_t packing;
        PackingInfo()
            : size(0), packing(0) {}
    };

    int const packing_info_size = ::boost::mpl::size<podlist>::type::value;
    PackingInfo packing_info[packing_info_size];

    template<typename T>
    struct DiscoverCollectionPacking
    {
        int8_t v;
        T      collection;

        static size_t get() {
            DiscoverCollectionPacking<T> obj;
            return reinterpret_cast<uint8_t*>(&obj.collection) - reinterpret_cast<uint8_t*>(&obj);
        }
    };

    struct CollectionPackingInfo
    {
        char const* name;
        size_t packing;
    };

    /** Try to determine some structure size rounding rules */
    struct CompoundSizeInfo
    {
        size_t base_size;
        size_t rounded_size;
    };
    template<int i> struct StructSizeDiscovery;
    template<> struct StructSizeDiscovery<0> { };
    template<> struct StructSizeDiscovery<1> { int8_t a; };
    template<> struct StructSizeDiscovery<2> { short a; };
    template<> struct StructSizeDiscovery<3> { short a; int8_t b; };
    template<> struct StructSizeDiscovery<4> { int32_t a; };
    template<> struct StructSizeDiscovery<5> { int32_t a; int8_t b; };
    template<> struct StructSizeDiscovery<6> { int32_t a; int16_t b; };
    template<> struct StructSizeDiscovery<7> { int32_t a; int16_t b; int8_t c; };
    template<> struct StructSizeDiscovery<8> { int32_t a; int32_t d; };
    template<> struct StructSizeDiscovery<9> { int64_t x; int8_t a; };
    template<> struct StructSizeDiscovery<10> { int64_t x; short a; };
    template<> struct StructSizeDiscovery<11> { int64_t x; short a; int8_t b; };
    template<> struct StructSizeDiscovery<12> { int64_t x; int32_t a; };
    template<> struct StructSizeDiscovery<13> { int64_t x; int32_t a; int8_t b; };
    template<> struct StructSizeDiscovery<14> { int64_t x; int32_t a; int16_t b; };
    template<> struct StructSizeDiscovery<15> { int64_t x; int32_t a; int16_t b; int8_t c; };
    static const int STRUCT_SIZE_DISCOVERY_LAST = 15;

    /** These two following structures are to check the rounding after 16 (i.e.
     * how 17 is packed determines how the whole thing will work from now on)
     */
    template<> struct StructSizeDiscovery<17> { int64_t x; int64_t b; int8_t c; };
    static const size_t STRUCT_SIZE_BASE_SIZE = sizeof(StructSizeDiscovery<17>) - 16;

    CompoundSizeInfo compound_size_info[STRUCT_SIZE_DISCOVERY_LAST + 1];
    template <int i>
    struct doStructSizeDiscovery : doStructSizeDiscovery<i - 1>
    {
        doStructSizeDiscovery()
        {
            compound_size_info[i].base_size = i;
            compound_size_info[i].rounded_size = sizeof(StructSizeDiscovery<i>);
        }
    };
    template<> struct doStructSizeDiscovery<-1> { };

    doStructSizeDiscovery<STRUCT_SIZE_DISCOVERY_LAST> do_it_now;
}
#include "packing/build_packing_info.tcc"

// build_packing_info builds a 
//      PackingInfo[] packing_info 
//  
// where the first element is the size to 
// consider and the second its packing

   
namespace {
    //////////////////////////////////////////////////////////////////////////
    //
    // Helpers for client code
    //
 
    using namespace Typelib;

    struct GetPackingSize : public TypeVisitor
    {
        size_t size;

        GetPackingSize(Type const& base_type)
            : size(base_type.getSize())
        { apply(base_type); }

        bool visit_(Numeric const& value) 
        { return false; }
        bool visit_(Enum const& value)
        { return false; }
        bool visit_(Pointer const& value)
        { return false; }
        bool visit_(OpaqueType const& value)
        { return false; }
        bool visit_(Array const& value)
        { 
            size = value.getIndirection().getSize();
            return TypeVisitor::visit_(value);
        }
        bool visit_(Compound const& value)
        { 
            typedef Compound::FieldList Fields;
            Fields const& fields(value.getFields());
            if (fields.empty())
                throw Packing::FoundNullStructure();

            // TODO: add a static check for this
            // we assume that unions are packed as their biggest field
            
            size_t max_size = 0;
            for (Fields::const_iterator it = fields.begin(); it != fields.end(); ++it)
            {
                if (it->getOffset() != 0)
                    continue;

                size = it->getType().getSize();
                TypeVisitor::visit_(value);
                max_size = std::max(max_size, size);
            }
            size = max_size;
            return true;
        }
    };
};

#include <iostream>
using std::cout;
using std::endl;

int Typelib::Packing::getOffsetOf(const Field& last_field, const Type& append_field)
{
    int base_offset = last_field.getOffset() + last_field.getType().getSize();

    GetPackingSize visitor(append_field);
    size_t const size(visitor.size);

    for (int i = 0; i < packing_info_size; ++i)
    {
        if (packing_info[i].size == size)
        {
            size_t const packing(packing_info[i].packing);
            return (base_offset + (packing - 1)) / packing * packing;
        }
    }

    CollectionPackingInfo collection_packing_info[] = {
        { "/std/vector", DiscoverCollectionPacking< std::vector<uint8_t> >::get() },
        { "/std/set",    DiscoverCollectionPacking< std::set<uint8_t> >::get() },
        { "/std/map",    DiscoverCollectionPacking< std::map<uint8_t, uint8_t> >::get() },
        { 0, 0 }
    };

    if (append_field.getCategory() == Type::Opaque)
    {
        for (CollectionPackingInfo* info = collection_packing_info; info->name; ++info)
        {
            if (info->name == std::string(append_field.getName(), 0, std::string(info->name).size()))
            {
                size_t const packing = info->packing;
                return (base_offset + (packing - 1)) / packing * packing;
            }
        }
    }

    throw PackingUnknown("cannot compute the packing of " + boost::lexical_cast<std::string>(append_field));
}
int Typelib::Packing::getOffsetOf(const Compound& current, const Type& append_field)
{
    Compound::FieldList const& fields(current.getFields());
    if (fields.empty())
        return 0;

    return getOffsetOf(fields.back(), append_field);
}

int Typelib::Packing::getSizeOfCompound(Compound const& compound)
{
    if (compound.getSize() <= STRUCT_SIZE_DISCOVERY_LAST)
        return compound_size_info[compound.getSize()].rounded_size;

    return (compound.getSize() + STRUCT_SIZE_BASE_SIZE - 1) / STRUCT_SIZE_BASE_SIZE * STRUCT_SIZE_BASE_SIZE;
}

