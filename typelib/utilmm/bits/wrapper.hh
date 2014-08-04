/* -*- C++ -*-
 * $Id: wrapper.hh 971 2005-04-08 16:31:47Z sjoyeux $
 */
#ifndef UTILMM_SINGLETON_WRAPPER_HEADER
#define UTILMM_SINGLETON_WRAPPER_HEADER

#include "typelib/utilmm/bits/dummy.hh"

namespace utilmm {
namespace singleton {
namespace details {
template <typename Ty> class wrapper_factory;
}

/** @brief Wrapper for singleton instances
 *
 * This class offer the interface used by @c utilmm::singleton::use to
 * manipulate the instance of type @c Ty
 *
 * @param Ty the type of the singleton instance
 *
 * @author Frédéric Py <fpy@laas.fr>
 *
 * @note All the mathods of this class are for internal use
 * do not call them directly.
 *
 * @ingroup singleton
 */
template <typename Ty> class wrapper : private dummy {
  public:
    /** @brief New attachement to singleton
     *
     * Indicates to the singleton server that there's a
     * new client to the @c Ty singleton.
     *
     * @post The singleton @c Ty exist
     */
    static void attach();
    /** @brief Detachment to singleton
     *
     * Indicates  to the singleton server thet the singleton @c Ty
     * has lost one client
     *
     * @pre The singleton @c Ty exist
     * @post If there's no more client to singleton @c Ty then
     * this one is destroyed
     */
    static void detach();

    /** @brief Acces to the singleton
     *
     * @return A reference to singleton @c Ty instance.
     *
     * @pre singleton @c Ty exist
     */
    static Ty &instance();

  private:
    wrapper();
    ~wrapper();

    static std::string name();

    Ty value;

    friend class details::wrapper_factory<Ty>;
}; // class utilmm::singleton::wrapper<>

} // namespace utilmm::singleton
} // namespace utilmm

#define IN_UTILMM_SINGLETON_WRAPPER_HEADER
#include "typelib/utilmm/bits/wrapper.tcc"
#undef IN_UTILMM_SINGLETON_WRAPPER_HEADER
#endif // UTILMM_SINGLETON_WRAPPER_HEADER

/** @file singleton/bits/wrapper.hh
 * @brief Declaration of utilmm::singleton::wrapper
 *
 * This header declare the @c utilmm::singleton::wrapper class.
 *
 * @author Frédéric Py <fpy@laas.fr>
 * @ingroup singleton
 * @ingroup intern
 */
