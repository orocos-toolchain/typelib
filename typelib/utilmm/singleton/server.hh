/* -*- C++ -*-
 * $Id: server.hh 970 2005-04-08 16:30:46Z sjoyeux $
 */
#ifndef UTILMM_SINGLETON_SERVER_HEADER
# define UTILMM_SINGLETON_SERVER_HEADER

# include <map>

# include "boost/thread/recursive_mutex.hpp"

# include "typelib/utilmm/singleton/bits/dummy.hh"

namespace utilmm {
  namespace singleton {

    /** @brief Singleton server
     *
     * The central server for singleton instances.
     *
     * @ingroup singleton
     * @ingroup intern
     *
     * @author Frédéric Py <fpy@laas.fr>
     */
    class server :public ::boost::noncopyable {
    private:
      server();
      ~server();

      static server &instance();

      void attach(std::string const &name, details::dummy_factory const& factory);
      bool detach(std::string const &name);

      dummy *get(std::string const &name) const;

      typedef std::map<std::string, dummy *> single_map;

      single_map singletons;

      static server *the_instance;

      typedef boost::recursive_mutex mutex_type;
      typedef mutex_type::scoped_lock lock_type;

      static mutex_type sing_mtx;

      friend class utilmm::singleton::dummy;
    }; // class utilmm::singleton::server

  } // namespace utilmm::singleton
} // namespace utilmm

#endif // UTILMM_SINGLETON_SERVER_HEADER
/** @file src/singleton/server.hh
 * @brief Definition of @c utilmm::singleton::server
 *
 * This private header defines the @c utilmm::singleton::server class.
 *
 * @author Frédéric Py <fpy@laas.fr>
 * @ingroup intern
 * @ingroup singleton
 */

/** @defgroup intern Library internal utilities
 * @brief This group include all the component
 * of the library only used for the internal implementation
 * of the library.
 */
