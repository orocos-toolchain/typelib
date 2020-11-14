#include <vector>

namespace _foo {
    namespace _bar {
        namespace _pkg {
            typedef short _TypedefedType;
        }

        enum _EnumType { _EnumValue };

        template <typename T>
        struct _TemplateType {
            T _foo;
        };

        struct _LocalType {
            int _one;
            ::_foo::_bar::_EnumType _two;
            _pkg::_TypedefedType _three;
            _EnumType _four;
            _TemplateType<_EnumType> _five;
            int _six[1];
            std::vector<_EnumType> _seven;
        };

        typedef _LocalType _Alias;
    }
}

