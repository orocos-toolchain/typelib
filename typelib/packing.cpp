#include "packing.h"

#include <boost/mpl/fold.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/list.hpp>
#include <boost/static_assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/deref.hpp>

#include <iostream>

namespace Typelib
{
    std::list< Packing > PackingInfo::packing;
};

using namespace Typelib;
using namespace boost::mpl;
namespace
{
    struct instantiate_list_base { };
    template<class Previous, class Item, template<typename> class F>
    struct instantiate_list : public Previous
    { F<Item> instance; };


    using std::make_pair;

    typedef std::pair<Type::Category, size_t> typelib_typeinfo;

    template<typename POD> typelib_typeinfo get_typeinfo() {};
    template<> typelib_typeinfo get_typeinfo<char>()   { return make_pair(Type::SInt, sizeof(char)); };
    template<> typelib_typeinfo get_typeinfo<short>()  { return make_pair(Type::SInt, sizeof(short)); };
    template<> typelib_typeinfo get_typeinfo<int>()    { return make_pair(Type::SInt, sizeof(int)); };
    template<> typelib_typeinfo get_typeinfo<long>()   { return make_pair(Type::SInt, sizeof(long)); };
    template<> typelib_typeinfo get_typeinfo<float>()  { return make_pair(Type::Float, sizeof(float)); };
    template<> typelib_typeinfo get_typeinfo<double>() { return make_pair(Type::Float, sizeof(double)); };
    template<> typelib_typeinfo get_typeinfo<char*>()  { return make_pair(Type::Pointer, sizeof(char*)); };

    typedef list<char, short, int, long, float, double, char*> podlist;

    // takes a type and a typelist, and builds
    // the typelist of packingof<type, it> for each
    // type in SecondList
    template<class First, class SecondList>
    struct iterate_second
    {
        typedef typename fold
            < SecondList
            , list<>
            , push_front< _1, packingof<First, _2> >
            >::type type;
    };

    // gets the packing info contained in the packingof object
    // Packing, and saves it in the PackingInfo::info (object) list
    template<class Packing>
    struct register_packing
    {
        typedef Packing     item_type;

        register_packing()
        {
            typelib_typeinfo second_info = get_typeinfo<typename Packing::Second>();

            Typelib::Packing packing = 
            {
                second_info.first,
                second_info.second,
                Packing::packing 
            };
            PackingInfo::packing.push_back( packing );
        }
    };

    template<class Previous, class Item>
    struct instantiate_register : public instantiate_list<Previous, Item, register_packing> {};


    // registers packing info for all <char, pod_type> pairs
    typedef iterate_second<char, podlist>::type charlist;
    fold
        < charlist
        , instantiate_list_base
        , instantiate_register<_1, _2>
        >::type do_char_packing;




    ////////////////////////////////////////////////////////////////////////////////
    //
    // Now, check some assumptions we make in the packing code
    //
    
    //
    // Check that struct { A struct { } } is NOT aligned 
    //
    struct empty_struct {};
    BOOST_STATIC_ASSERT(( packingof<char, empty_struct>::packing == 1 ));

    //
    // Check that struct { A struct { B } } is packed as struct { A B } is
    //
    
    template<typename T> struct simple_structure { typedef T inner_type; T value; };

    // Checks that the <first, simple_struct<second>> is packed as <first, second> is
    // packingof<first, simple_struct<second> > is in Packing
    template<class Packing>
    struct check_struct_packing
    {
        BOOST_STATIC_ASSERT(( 
            Packing::packing == 
            packingof
                < typename Packing::First
                , typename Packing::Second::inner_type
                >::packing
            ));
    };
    
    // Builds the typelist of simple_structure<pod> for each pod in podlist
    typedef fold< podlist, list<>, push_front<_1, simple_structure<_2> > >::type podstructs;
    // Applies check_struct_packing for <first, struct > for each struct in podstructs
    // It is equivalent as applying it to <first, simple_struct<pod> > for each pod in podlist
    struct do_check_struct_packing
    {
        typedef fold
            < iterate_second<char, podstructs>::type
            , list<>
            , push_front< _1, check_struct_packing<_2> >
            >::type type;
    };


    //
    // Check that struct { A B[1] } is packed as struct { A B } is
    // We use here that struct { A struct { B } } is packed as struct { A B }
    // even if we didn't checked that for arrays (TODO: do it)
    //
    
    template<typename T> struct simple_array { typedef T inner_type; T array[1]; };

    // Builds the typelist of simple_structure<pod> for each pod in podlist
    typedef fold< podlist, list<>, push_front<_1, simple_array<_2> > >::type podarrays;
    // Applies check_struct_packing for <first, struct > for each struct in podstructs
    // It is equivalent as applying it to <first, simple_struct<pod> > for each pod in podlist
    struct do_check_array_packing
    {
        typedef fold
            < iterate_second<char, podarrays>::type
            , list<>
            , push_front< _1, check_struct_packing<_2> >
            >::type type;
    };





    //////////////////////////////////////////////////////////////////////////
    //
    // Helpers for client code
    //
 
    std::string category_name(Type::Category cat)
    {
        switch(cat)
        {
            case Type::SInt:  return "signed integer";
            case Type::UInt:  return "unsigned integer";
            case Type::Float: return "floating point number";
            case Type::Pointer: return "pointer";
            default:          return "unhandled type";
        };
    }


    int   get_base_offset(const Type* type)
    {
        if (type -> getCategory() != Type::Struct)
            return type->getSize();

        if (type->getFields().empty())
            return 0;

        const Field& field = type->getFields().back();
        const Type* field_type   = field.getType();

        return field.getOffset() + get_base_offset(field_type);
    };

    void  get_packed_type(const Type* append_type, Type::Category* type_cat, size_t* type_size)
    {
        if (append_type -> isSimple())
        {
            Type::Category cat  = append_type->getCategory();
            size_t         size = append_type->getSize();
            if (cat == Type::UInt)
                cat = Type::SInt;
            else if (cat == Type::Array)
                get_packed_type(append_type -> getNextType(), &cat, &size);

            *type_cat  = cat;
            *type_size = size;
            return;
        }

        if (append_type -> getFields().empty())
        {
            *type_cat = Type::NullType;
            *type_size = 0;
            return;
        }

        const Field& field = append_type->getFields().front();
        get_packed_type( field.getType(), type_cat, type_size );
    }

};

namespace Typelib
{
    int  PackingInfo::getOffsetOf(const Field& cur_field, const Field& append_field)
    {
        int base_offset = cur_field.getOffset() + cur_field.getType() -> getSize();

        Type::Category append_cat;
        size_t         append_size;
        get_packed_type(append_field.getType(), &append_cat, &append_size);

        for (std::list<Packing>::const_iterator it = packing.begin(); it != packing.end(); ++it)
            if (it->category == append_cat && it->size == append_size)
            {
                int packing = it->packing;
                if (! packing) return base_offset;
                
                return (base_offset + (packing - 1)) / packing * packing;
            }

        throw PackingUnknown(append_cat, append_size);
    }
    
    void PackingInfo::dump()
    {
        typedef std::list<Packing> PackList;
        for (PackList::iterator it = PackingInfo::packing.begin(); it != PackingInfo::packing.end(); ++it)
        {
            Type::Category cat = it->category;
            size_t size = it->size;

            std::cout 
                << "{ char " 
                << ", " 
                << category_name(cat) << " [" << size << "]" 
                << "}: " << it->packing
                << std::endl;
        }
    }
}

