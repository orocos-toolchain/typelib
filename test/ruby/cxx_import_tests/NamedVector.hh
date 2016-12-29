#ifndef TEST_HEADER_DATA_NAMED_VECTOR_H_
#define TEST_HEADER_DATA_NAMED_VECTOR_H_

#include <string>
#include <vector>

namespace ns_namedVector {

template <class T> struct NamedVector {
    std::vector<std::string> names;
    std::vector<T> elements;
};

namespace samples {
struct Joints : public ns_namedVector::NamedVector<double> {
    double bla;
};
}

namespace commands {
typedef ns_namedVector::samples::Joints Joints;
}
}

#endif /*TEST_HEADER_DATA_NAMED_VECTOR_H_*/
