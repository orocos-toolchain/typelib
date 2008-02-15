Typelib is a C++ library which allows for introspection on data types and data
values. Its value model is based on the C type model. The library allows to
load definition from various type description files (including plain C), build
types programmatically, create and manipulate values from these types.

A binding to the Ruby language is provided, which allows to very easily interface
a dynamic library from within Ruby code. This Ruby binding is based on the dyncall
library, whose full source code is provided in this release.

Typelib has been written by
  Sylvain Joyeux <sylvain.joyeux@m4x.org>

Copyright 2004-2008
  LAAS/CNRS <openrobots@laas.fr>
  DGA <arnaud.paronian@dga.gouv.fr>

This software is provided under the CeCILL B License, which gives comparable terms
of use than the BSD license. See LICENSE.txt or LICENSE.fr.txt for the full license
texts.

== Installation
=== C++ library
The C++ library depends on the following:
 * the boost library, including boost/filesystem
 * utilmm which can be downloaded using Darcs with
     darcs get http://www.laas.fr/~sjoyeux/darcs/utilmm
   (check the INSTALL file)
 * the antlr parser generator
 * cmake and pkg-config
 * doxygen for the documentation (optional)
 * libxml2

When all these dependencies are installed, run
    mkdir build
    cd build
    cmake ..
    make
    make doc # to build the documentation, only if doxygen is available

and as root,
    make install

=== Ruby bindings
The installation of the Ruby bindings require the following:
 * the ruby interpreter version 1.8 and the associated
   development files.
   Under Debian, these are named ruby1.8 and libruby1.8-dev
 * testrb for the test suite
 * rdoc for generating the documentation (optional)

At runtime, the bindings require the following:
 * utilrb, which is best downloaded as a gem. See
     http://www.rubygems.org for information about the RubyGems
     system. This system can be installed by the rubygems
     package on Debian.
 
   When you have installed rubygems, run
     gem install utilrb

   You may have to run it as root if RubyGems is installed
   globally (this is the case for Debian's rubygems package)

   You can also find the sources at
     git clone http://www.laas.fr/~sjoyeux/git/utilrb.git
   read the INSTALL.txt file

=== Quick Debian installation guide

Run as root
  apt-get install build-essential cmake pkg-config libboost-dev libboost-filesystem-dev ruby1.8-dev libtest-unit-ruby rubygems cantlr libantlr-dev doxygen rdoc1.8 libxml2-dev
  gem install utilrb

Run as a normal user
  <go into a source directory>
  darcs get http://www.laas.fr/~sjoyeux/darcs/utilmm
  <install the utilmm library by following instructions in
  utilmm/INSTALL>

  <go into Typelib source directory>
  mkdir build
  cd build
  cmake ..
  make
  make doc

Run as root
  make install

