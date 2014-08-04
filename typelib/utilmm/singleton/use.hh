/* -*- C++ -*-
 * $Id: use.hh 957 2005-03-07 16:01:20Z sjoyeux $
 */
#ifndef UTILMM_SINGLETON_USE_HEADER
#define UTILMM_SINGLETON_USE_HEADER

namespace utilmm {
/** @brief Singleton manipulation
 *
 * This namespace include all the classes used to manipulate
 * singletons.
 *
 * @ingroup singleton
 */
namespace singleton {

/** @brief Access point to singleton.
 *
 * This class is the used to acces to a singleton
 * with type @c Ty . It also manage the singleton life time
 * indicating to the @c server that this singleton has a
 * new client.
 *
 * @author Frédéric Py <fpy@laas.fr>
 *
 * @ingroup singleton
 */
template <typename Ty> struct use {
    /** @brief Constructor
     *
     * Create a new instance and eventually create the
     * singleton. This constructor will indicate to
     * server that thze singleton @c Ty has a new client.
     * If it is the first client then the sezrver will
     * create the singleton @c Ty.
     */
    use();
    /** @brief Copy constructor
     *
     * This constructor only indicate to server that there's a
     * new client to sinbgleton @c Ty.
     */
    use(use const &other);

    /** @brief Destructor
     *
     * Indicate to server that singlerton @c Ty has lost one
     * client if the number of client to @c Ty becomes null then
     * the singleton is destroyed.
     */
    ~use();

    /** @brief Access to singleton
     *
     * @return A reference to the singleton @c Ty
     */
    Ty &instance() const;

    /** @brief Access to singleton
     * @sa instance() const
     */
    Ty &operator*() const;
    /** @brief Access to singleton
     *
     * This operator allow client to directly access to singlton's
     * attributes.
     *
     * @sa instance() const
     */
    Ty *operator->() const;
}; // struct utilmm::singleton::use<>

} // namespace utilmm::singleton
} // namespace utilmm

#define IN_UTILMM_SINGLETON_USE_HEADER
#include "typelib/utilmm/singleton/bits/use.tcc"
#undef IN_UTILMM_SINGLETON_USE_HEADER
#endif // UTILMM_SINGLETON_USE_HEADER
       /** @file singleton/use.hh
        * @brief Declararation of @c utilmm::singleton::use class
        *
        * This header defclare the @c utilmm::singleton::use
        * class.
        *
        * @author Frédéric Py <fpy@laas.fr>
        * @ingroup singleton
        */

/** @defgroup singleton Singleton pattern design.
 *
 * @brief Implementation of designe pattern which can be compared to
 * phoenix singleton with advanced instance life management.
 *
 * This module includes all the classes used to implement a phoenix
 * singleton pattern design supporting shared libraires.
 *
 * The phoenix singleton pattern design offers the possibility to have
 * a special class with these properties :
 * @li At any time there's no more thazn one instance of the singleton
 * @li The singleton instance exists if and only if it is needed
 * @li If the singleton instance was destroyed but there are new clients
 * then it is recreated.
 *
 * This implementation offer also a support for shared libraries using
 * one ingleton server with advanced controller of life time for each
 * instances. This controller is a basic phoenix singleton stored in
 * a shared library.
 *
 * @par Accessing to a singleton
 *
 * To access to a singleton we need to use the @c utilmm::singleton::use
 * class. This class will offer an acces to singleton instance and will
 * manage the singleton life (it can be seen as a reference counting
 * smart pointer.
 *
 * For example if one class @c foo need to access to a singleton of type
 * @c bar . User can code it asd this :
 *
 * @code
 * #include "utilmm/singleton/use.hh"
 *
 * class foo {
 * private:
 *   utilmm::singleton::use<bar> bar_s;
 *   int value;
 *
 * public:
 *  foo():value(0) {}
 *  explicit foo(int v):value(v) {}
 *  ~foo() {}
 *
 *  void send_to_bar() {
 *    bar_s.instance().send(value);
 *    // or bar_s->send(value);
 *  }
 * };
 *
 * @endcode
 *
 * @par Making a class a pure singleton
 *
 * To define a class singleton as a pure singleton (ie it is impossible
 * to access to it except by @c utilmm::singleton::use ) one can follow
 * this example :
 *
 * @code
 * #include "utilmm/singleton/wrapper_fwd.hh"
 *
 * class pure_singleton {
 * public:
 *  // [...]
 * private:
 *   pure_singleton();
 *   ~pure_singleton();
 *
 *  // [...]
 *
 *   friend class utilmm::singleton::wrapper<pure_singleton>;
 * };
 * @endcode
 *
 * All the structors are defined as private then only firends
 * (ie @c utilmm::singleton::wrapper) can create and destroy
 * the instance then we have the guarantee that this class is
 * a pure phoenix singleton.
 */
