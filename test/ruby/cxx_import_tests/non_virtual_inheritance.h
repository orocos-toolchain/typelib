#include <stdint.h>
struct FirstBase {
    uint64_t first;
};
struct SecondBase {
    uint64_t second;
};
struct Derived : public FirstBase, public SecondBase {
    uint64_t third;
};
