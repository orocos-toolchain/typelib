/* -*- C++ -*-
 * $Id: dummy.cc 970 2005-04-08 16:30:46Z sjoyeux $
 */
#include "typelib/utilmm/singleton/server.hh"

using namespace utilmm::singleton;

/*
 * class utilmm::singleton::dummy
 */

// structors
dummy::dummy()
  :ref_counter(0ul) {}

dummy::~dummy() {}

// modifiers
void dummy::incr_ref() const {
  ++ref_counter;
}

bool dummy::decr_ref() const {
  return (ref_counter--)<=1;
}

// statics
void dummy::attach(std::string const &name, details::dummy_factory const& inst) {
  server::lock_type locker(server::sing_mtx);
  server::instance().attach(name, inst);
}

void dummy::detach(std::string const &name) {
  server::lock_type locker(server::sing_mtx);
  if( server::instance().detach(name) )
    delete server::the_instance;
}

dummy *dummy::instance(std::string const &name) {
  server::lock_type locker(server::sing_mtx);
  return server::instance().get(name);
}
