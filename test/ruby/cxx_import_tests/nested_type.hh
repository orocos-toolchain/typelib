#include <vector>

namespace nested_types {
    struct Outside
    {
        struct Inside
        {
            int a;
        };
        int b;
    };

    struct User
    {
        std::vector<Outside::Inside> vector;
    };
}

