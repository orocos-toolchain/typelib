/*! \mainpage Typelib: a C++ type and value introspection library

Typelib is a C++ library which allows for introspection on data types and data
values. Its value model is based on the C type model. The library allows to
load definition from various type description files (including plain C), build
types programmatically, create and manipulate values from these types.

The following import/export plugins are available:
  - import plain C plus C++-compatible namespace support
  - import/export into Typelib's own XML format
  - export IDL files (CORBA definition language)

A binding to the Ruby language is provided, which allows to very easily interface
a dynamic library from within Ruby code. This Ruby binding is based on the dyncall
library, whose full source code is provided in this release.

Typelib has been written by Sylvain Joyeux <sylvain.joyeux@m4x.org>

Copyright 2004-2008 LAAS/CNRS <openrobots@laas.fr> and DGA <arnaud.paronian@dga.gouv.fr>

This software is provided under the CeCILL B License, which gives comparable
terms of use than the BSD license. See LICENSE.txt or LICENSE.fr.txt provided
with the source code for the full license texts.

These pages document only the C++ part of the library. The Ruby bindings documentation
is available <a href="ruby/index.html">here</a>.

\section Installation

\subsection source Getting the source code
Releases are available on SourceForge: <a
href="http://sourceforge.net/projects/typelib/">http://sourceforge.net/projects/typelib/</a>.
You can access this project page from anywhere in the documentation by clicking
on the SourceForge.net logo at the bottom of documentation pages.

The development repository is managed by git and is (for now) publicly available in GitHub:

<pre>
  git clone git://github.com/doudou/typelib.git
</pre>

(see <a href="http://github.com/doudou/">this page</a> for more information)

\subsection cpp Building and installing the C++ library
The C++ library depends on the following:
  - the boost library, including boost/filesystem
  - utilmm (utilmm.sf.net)
  - the antlr parser generator
  - cmake and pkg-config
  - doxygen for the documentation (optional)
  - libxml2

When all these dependencies are installed, run
<pre>
    mkdir build
    cd build
    cmake ..
    make
    make doc # to build the documentation, only if doxygen is available
</pre>

and as root,
<pre>
    make install
</pre>

Alternatively, you can add the \c -DCMAKE_INSTALL_PREFIX=path/to/the/installation/target option to cmake
to install the files in a custom directory

\subsection ruby Building and installing Ruby bindings
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
*/
