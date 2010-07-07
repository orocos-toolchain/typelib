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

    /** It seems that the following rule apply with struct size rounding: the
     * size is rounded so that the biggest element in the structure is properly
     * aligned.
     */

    // To get the size of an empty struct
    struct EmptyStruct { };
    // Rounded size is 24
    struct StructSizeDiscovery1 { int64_t a; int64_t z; int8_t end; };
    struct StructSizeDiscovery2 { int32_t a; int32_t b; int64_t z; int8_t end; };
    // Rounded size is 20
    struct StructSizeDiscovery3 { int32_t a; int32_t b; int32_t c; int32_t d; int8_t end; };
    struct StructSizeDiscovery4 { int32_t a[4]; int8_t end; };
    struct StructSizeDiscovery5 { int16_t a[2]; int32_t b[3]; int8_t end; };
    // Rounded size is 18
    struct StructSizeDiscovery6 { int16_t a[2]; int16_t b[6]; int8_t end; };
    struct StructSizeDiscovery7 { int8_t a[4]; int16_t b[6]; int8_t end; };
    // Rounded size is 17
    struct StructSizeDiscovery8 { int8_t a[4]; int8_t b[12]; int8_t end; };

    BOOST_STATIC_ASSERT(
			(sizeof(long) == 8 && sizeof(StructSizeDiscovery1) == 24) ||
			(sizeof(long) == 4 && sizeof(StructSizeDiscovery1) == 20));
    BOOST_STATIC_ASSERT(
			(sizeof(long) == 8 && sizeof(StructSizeDiscovery2) == 24) ||
			(sizeof(long) == 4 && sizeof(StructSizeDiscovery2) == 20));

    BOOST_STATIC_ASSERT(sizeof(StructSizeDiscovery3) == 20);
    BOOST_STATIC_ASSERT(sizeof(StructSizeDiscovery4) == 20);
    BOOST_STATIC_ASSERT(sizeof(StructSizeDiscovery5) == 20);

    BOOST_STATIC_ASSERT(sizeof(StructSizeDiscovery6) == 18);
    BOOST_STATIC_ASSERT(sizeof(StructSizeDiscovery7) == 18);

    BOOST_STATIC_ASSERT(sizeof(StructSizeDiscovery8) == 17);

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
        int size;

        GetPackingSize(Type const& base_type)
            : size(-1)
        { apply(base_type); }

        bool visit_(Numeric const& value) 
        { size = value.getSize();
            return false; }
        bool visit_(Enum const& value)
        { size = value.getSize();
            return false; }
        bool visit_(Pointer const& value)
        { size = value.getSize();
            return false; }
        bool visit_(Compound const& value)
        { 
            typedef Compound::FieldList Fields;
            Fields const& fields(value.getFields());
            if (fields.empty())
                throw Packing::FoundNullStructure();

            // TODO: add a static check for this
            // we assume that unions are packed as their biggest field
            
            int max_size = 0;
            for (Fields::const_iterator it = fields.begin(); it != fields.end(); ++it)
            {
                GetPackingSize recursive(it->getType());
                if (recursive.size == -1)
                    throw std::runtime_error("cannot compute the packing size for " + value.getName());

                max_size = std::max(max_size, recursive.size);
            }
            size = max_size;
            return true;
        }
        bool visit_(Container const& value)
        {
            CollectionPackingInfo collection_packing_info[] = {
                { "/std/vector", DiscoverCollectionPacking< std::vector<uint16_t> >::get() },
                { "/std/set",    DiscoverCollectionPacking< std::set<uint8_t> >::get() },
                { "/std/map",    DiscoverCollectionPacking< std::map<uint8_t, uint8_t> >::get() },
                { "/std/string", DiscoverCollectionPacking< std::string >::get() },
                { 0, 0 }
            };

            for (CollectionPackingInfo* info = collection_packing_info; info->name; ++info)
            {
                if (info->name == std::string(value.getName(), 0, std::string(info->name).size()))
                {
                    size = info->packing;
                    return true;
                }
            }

            throw Packing::PackingUnknown("cannot compute the packing of " + boost::lexical_cast<std::string>(value.getName()));
        }
    };
};

#include <iostream>
using std::cout;
using std::endl;

int Typelib::Packing::getOffsetOf(const Field& last_field, const Type& append_field, size_t packing)
{
    if (packing == 0)
        return 0;
    int base_offset = last_field.getOffset() + last_field.getType().getSize();
    return (base_offset + (packing - 1)) / packing * packing;
}
int Typelib::Packing::getOffsetOf(Compound const& compound, const Type& append_field, size_t packing)
{
    Compound::FieldList const& fields(compound.getFields());
    if (fields.empty())
        return 0;

    return getOffsetOf(fields.back(), append_field, packing);
}

int Typelib::Packing::getOffsetOf(const Field& last_field, const Type& append_field)
{

    GetPackingSize visitor(append_field);
    if (visitor.size == -1)
        throw PackingUnknown("cannot compute the packing of " + boost::lexical_cast<std::string>(append_field.getName()));
        
    size_t const size(visitor.size);

    for (int i = 0; i < packing_info_size; ++i)
    {
        if (packing_info[i].size == size)
            return getOffsetOf(last_field, append_field, packing_info[i].packing);
    }
    throw PackingUnknown("cannot compute the packing of " + boost::lexical_cast<std::string>(append_field.getName()));
}
int Typelib::Packing::getOffsetOf(const Compound& current, const Type& append_field)
{
    Compound::FieldList const& fields(current.getFields());
    if (fields.empty())
        return 0;

    return getOffsetOf(fields.back(), append_field);
}

struct AlignmentBaseTypeVisitor : public TypeVisitor
{
    Type const* result;

    bool handleType(Type const& type)
    {
        if (!result || result->getSize() < type.getSize())
            result = &type;
        return true;
    }

    virtual bool visit_ (NullType const& type) { throw UnsupportedType(type, "cannot represent alignment of null types"); }
    virtual bool visit_ (OpaqueType const& type) { throw UnsupportedType(type, "cannot represent alignment of opaque types"); };
    virtual bool visit_ (Numeric const& type) { return handleType(type); }
    virtual bool visit_ (Enum const& type)    { return handleType(type); }

    virtual bool visit_ (Pointer const& type) { return handleType(type); }
    virtual bool visit_ (Container const& type) { return handleType(type); }
    // arrays and compound are handled recursively

    static Type const* find(Type const& type)
    {
        AlignmentBaseTypeVisitor visitor;
        visitor.result = NULL;
        visitor.apply(type);
        return visitor.result;
    }
};

int Typelib::Packing::getSizeOfCompound(Compound const& compound)
{
    // Find the biggest type in the compound
    Compound::FieldList const& fields(compound.getFields());
    if (fields.empty())
        return sizeof(EmptyStruct);

    Type const* biggest_type = AlignmentBaseTypeVisitor::find(compound);

    return getOffsetOf(compound, *biggest_type);
}

