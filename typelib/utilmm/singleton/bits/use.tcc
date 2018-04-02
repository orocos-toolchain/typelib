/* -*- C++ -*-
 * $Id: use.tcc 957 2005-03-07 16:01:20Z sjoyeux $
 */
#ifndef IN_UTILMM_SINGLETON_USE_HEADER
# error "Cannot include template files directly"
#else

#include "typelib/utilmm/singleton/bits/wrapper.hh"

namespace utilmm {
  namespace singleton {

    /*
     * struct utilmm::singleton::use<>
     */
    template<typename Ty>
    use<Ty>::use() {
      wrapper<Ty>::attach();
    }

    template<typename Ty>
    use<Ty>::use(use<Ty> const &other) {
      wrapper<Ty>::attach();
    }

    template<typename Ty>
    use<Ty>::~use() {
      wrapper<Ty>::detach();
    }

    template<typename Ty>
    Ty &use<Ty>::instance() const {
      return wrapper<Ty>::instance();
    }

    template<typename Ty>
    Ty &use<Ty>::operator* () const {
      return instance();
    }

    template<typename Ty>
    Ty *use<Ty>::operator->() const {
      return &operator* ();
    }

  } // namespace utilmm::singleton
} // namespace utilmm

#endif // IN_UTILMM_SINGLETON_USE_HEADER
