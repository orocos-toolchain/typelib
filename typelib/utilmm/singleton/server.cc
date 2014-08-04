/* -*- C++ -*-
 * $Id: server.cc 970 2005-04-08 16:30:46Z sjoyeux $
 */
#include "server.hh"

using namespace std;

using namespace utilmm::singleton;
using boost::recursive_mutex;

server *utilmm::singleton::server::the_instance = 0;

boost::recursive_mutex utilmm::singleton::server::sing_mtx;

/*
 * class utilmm::singleton::server
 */
// structors
server::server() { the_instance = this; }

server::~server() { the_instance = 0; }

// modifiers
void server::attach(std::string const &name,
                    details::dummy_factory const &factory) {
    single_map::iterator it = singletons.find(name);
    if (it == singletons.end())
        it = singletons.insert(single_map::value_type(name, factory.create()))
                 .first;

    it->second->incr_ref();
}

bool server::detach(std::string const &name) {
    single_map::iterator i = singletons.find(name);

    if (i->second->decr_ref()) {
        dummy *to_del = i->second;

        singletons.erase(i);
        delete to_del;
        return singletons.empty();
    }
    return false;
}

// observers
dummy *server::get(std::string const &name) const {
    return singletons.find(name)->second;
}

// statics
server &server::instance() {
    if (0 == the_instance)
        new server;
    return *the_instance;
}
