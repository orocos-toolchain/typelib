#ifndef TYPELIB_VALUE_CAST_HH
#define TYPELIB_VALUE_CAST_HH

#include "value.hh"
#include "normalized_numerics.hh"

namespace Typelib {
/** This visitor checks that a given Value object
 * is of the C type T. It either returns a reference to the
 * typed value or throws BadValueCast
 */
template <typename T> class CastingVisitor : public ValueVisitor {
    bool m_found;
    T m_value;

    bool visit_(typename normalized_numeric_type<T>::type &v) {
        m_value = v;
        m_found = true;
        return false;
    }

  public:
    CastingVisitor() : ValueVisitor(false), m_found(false), m_value(){};
    T &apply(Value v) {
        m_found = false;
        ValueVisitor::apply(v);
        if (!m_found)
            throw BadValueCast();

        return m_value;
    }
};

/** Casts a Value object to a given simple type T
 * @throws BadValueCast */
template <typename T> T &value_cast(Value v) {
    CastingVisitor<T> caster;
    return caster.apply(v);
}

/** Casts a pointer to a given simple type T using \c type as the type for \c
 * *ptr
 * @throws BadValueCast */
template <typename T> T &value_cast(void *ptr, Type const &type) {
    return value_cast<T>(Value(ptr, type));
}
}

#endif
