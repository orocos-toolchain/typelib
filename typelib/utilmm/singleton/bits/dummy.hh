/* -*- C++ -*-
 * $Id: dummy.hh 990 2005-09-14 15:17:04Z sjoyeux $
 */
#ifndef UTILMM_SINGLETON_DUMMY_HEADER
# define UTILMM_SINGLETON_DUMMY_HEADER

# include <string>
#include <boost/utility.hpp>

#include "typelib/utilmm/singleton/bits/server_fwd.hh"

namespace utilmm {
  namespace singleton {
    class dummy;

    namespace details {
      struct dummy_factory {
        virtual ~dummy_factory() {};
        virtual dummy* create() const = 0;
      };
    }

    /** @brief base class for @c utilmm::singleton::wrapper
     *
     * This class is the base class for all the singletons wrapper and
     * the "public" interface to the singleton server.
     *
     * @author Frédéric Py <fpy@laas.fr>
     * @ingroup singleton
     * @ingroup intern
     */
    class dummy :public ::boost::noncopyable {
    protected:
      /** @brief Default Constructor */
      dummy();
      /** @brief Destructor */
      virtual ~dummy() =0;

      /** @brief Attach a new singleton.
       *
       * @param name Internal id of the singleton.
       * @param inst Candidate as singleton.
       *
       * This function called by @c wrapper::attach try to create a
       * new singleton with @a name as unique id. If there's allready
       * a singleton @a name then @a inst is deleted
       */
      static void attach(std::string const &name, details::dummy_factory const& factory);
      /** @brief Detach to a singleton
       *
       * @param name Internal id of a singleton
       * 
       * This function called by wrapper::detach indicate to the
       * singleton server that the singleton identified as @a name has
       * lost one client. 
       */
      static void detach(std::string const &name);

      /** @brief Singleton generic access
       *
       * @param name Internal id of a singleton
       * 
       * @return a pointer to the dummy wrapper of the singleton
       * attached to @a name
       */
      static dummy *instance(std::string const &name);

    private:
      void incr_ref() const;
      bool decr_ref() const;

      mutable size_t ref_counter;

      friend class utilmm::singleton::server;
    }; // class utilmm::singleton::dummy

  } // namespace utilmm::singleton
} // namespace utilmm

#endif // UTILMM_SINGLETON_DUMMY_HEADER

/** @file singleton/bits/dummy.hh
 * @brief Declaration of utilmm::singleton::dummy
 *
 * This header declare the @c utilmm::singleton::dummy class.
 *
 * @author Frédéric Py <fpy@laas.fr>
 * @ingroup singleton
 * @ingroup intern
 */
