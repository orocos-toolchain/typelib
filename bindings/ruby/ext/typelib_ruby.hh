#ifndef TYPELIB_RUBY_EXT_HH
#define TYPELIB_RUBY_EXT_HH

#include <typelib/value.hh>

// Public interface to the Typelib extension for Ruby

/* Returns the Typelib::Value object wrapped by +value+ */
Typelib::Value typelib_get(VALUE value);
/* Converts a Typelib::Value to Ruby's VALUE. +registry+ is the wrapper of the
 * Registry on which +v+ is defined */
VALUE typelib_to_ruby(Typelib::Value v, VALUE registry, VALUE parent);
/* Initializes the memory held by +value+ by the Ruby value +new_value+ */
VALUE typelib_from_ruby(Typelib::Value value, VALUE new_value);

#endif
