/* -*- C++ -*-
 * $Id: wrapper.tcc 971 2005-04-08 16:31:47Z sjoyeux $
 */
#ifndef IN_UTILMM_SINGLETON_WRAPPER_HEADER
# error "Cannot include template files directly"
#else

# include <typeinfo>
#include <iostream>

namespace utilmm {
  namespace singleton {

    namespace details {
      template<typename Ty>
      class wrapper_factory : public dummy_factory {
      public:
          virtual dummy* create() const { return new wrapper<Ty>; }
      };
    }

    /*
     * class utilmm::singleton::wrapper<>
     */
    // structors
    template<typename Ty>
    wrapper<Ty>::wrapper() {}

    template<typename Ty>
    wrapper<Ty>::~wrapper() {}

    // statics
    template<typename Ty>
    inline std::string wrapper<Ty>::name() {
      return typeid(Ty).name();
    }

    template<typename Ty>
    void wrapper<Ty>::attach() {
      dummy::attach(name(), details::wrapper_factory<Ty>());
    }

    template<typename Ty>
    void wrapper<Ty>::detach() {
      dummy::detach(name());
    }

    template<typename Ty>
    Ty &wrapper<Ty>::instance() {
        // dynamic_cast fails on gcc 3.3.5. Don't know why, the types
        // seem right :(
        // Fall back to static_cast
      // return dynamic_cast<wrapper *>(dummy::instance(name()))->value;
      return static_cast<wrapper *>(dummy::instance(name()))->value;
    }

  } // namespace utilm::singleton
} // namespace utilmm

#endif // IN_UTILMM_SINGLETON_WRAPPER_HEADER
