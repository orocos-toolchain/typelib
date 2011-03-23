#ifndef TYPELIB_CXX_STANDARD_TYPES_HPP
#define TYPELIB_CXX_STANDARD_TYPES_HPP

#include <typelib/registry.hh>

namespace Typelib
{
    namespace CXX
    {
        /** Adds some definitions for standard C++ types that Typelib can handle */
        void addStandardTypes(Typelib::Registry& registry);
    }
}

#endif

