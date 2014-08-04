#include <vector>

template <typename T> struct BaseTemplate { T field; };

struct Subclass : BaseTemplate<std::vector<double> > {
    std::vector<double> another_field;
};
