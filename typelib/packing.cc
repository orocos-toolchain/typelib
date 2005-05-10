#include "packing.hh"
#include "typemodel.hh"
#include "visitor.hh"

////////////////////////////////////////////////////////////////////////////////
//
// Check some assumptions we make in the packing code
//

#include <boost/mpl/size.hpp>
#include "packing/tools.tcc"
#include "packing/check_arrays.tcc"
#include "packing/check_struct_in_struct.tcc"
#include "packing/check_size_criteria.tcc"

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

    struct GetPackingSize : public Visitor
    {
        size_t size;

        bool visit_(Numeric const& value) 
        { return false; }
        bool visit_(Enum const& value)
        { return false; }
        bool visit_(Pointer const& value)
        { return false; }
        bool visit_(Array const& value)
        { 
            size = value.getIndirection().getSize();
            return false;
        }
        bool visit_(Union const& value)
        { throw Packing::FoundUnion(); }
        bool visit_(Struct const& value)
        {
            Compound::FieldList const& fields(value.getFields());
            if (fields.empty())
                throw Packing::FoundNullStructure();

            size = fields.front().getType().getSize();
            return false;
        }
    };
};

int Typelib::Packing::getOffsetOf(const Field& last_field, const Field& append_field)
{
    int base_offset = last_field.getOffset() + last_field.getType().getSize();

    GetPackingSize visitor;
    visitor.visit(append_field.getType());

    for (int i = 0; i < packing_info_size; ++i)
    {
        if (packing_info[i].size == visitor.size)
            return base_offset + base_offset % packing_info[i].packing;
    }

    throw PackingUnknown();
}
int Typelib::Packing::getOffsetOf(const Compound& current, const Field& append_field)
{
    Compound::FieldList const& fields(current.getFields());
    if (fields.empty())
        return 0;

    return getOffsetOf(fields.front(), append_field);
}

