#ifndef TEST_HEADER_DATA_STRINGS_H
#define TEST_HEADER_DATA_STRINGS_H

#include <string>

namespace strings {

    struct S1 {
        std::string myS;
    };

    // this uses "wchar_t" is currently ignored in the clang-tlb-importer as it
    // is not supported by the typelib "/std/string" container.
    struct S2 {
        std::wstring W;
    };

    // just to be sure that the actual wchar_t type is supported...
    struct C3 {
        wchar_t arr[3];
    };

}

#endif /*TEST_HEADER_DATA_STRINGS_H*/
