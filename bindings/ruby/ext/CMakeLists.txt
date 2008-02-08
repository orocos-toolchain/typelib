INCLUDE(RubyExtensions)

ADD_RUBY_EXTENSION(typelib_ruby typelib_ruby.cc value.cc strings.cc
    convert.cc functions.cc specialized_types.cc registry.cc)

