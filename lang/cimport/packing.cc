#include "packing.hh"
#include <typelib/typemodel.hh>
#include <typelib/typevisitor.hh>

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

    throw PackingUnknown();
}
int Typelib::Packing::getOffsetOf(const Compound& current, const Type& append_field)
{
    Compound::FieldList const& fields(current.getFields());
    if (fields.empty())
        return 0;

    return getOffsetOf(fields.back(), append_field);
}

