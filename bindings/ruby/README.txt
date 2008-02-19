== Ruby bindings

The Ruby bindings to the C++ Typelib library allow to represent and manipulate
C types and in-memory values. Based on that capability, the bindings offer a
nice way to dynamically interface with C shared libraries.

The dynamic function calls are using the dyncall library, to get rid of the
many Ruby/DL bugs and limitations, and most importantly its lack of
maintainership. The dyncall source code is provided in the Typelib source
packages. Dyncall is under the BSD license:

    Copyright (c) 2007,2008 Daniel Adler <dadler@uni-goettingen.de>, 
			    Tassilo Philipp <tphilipp@potion-studios.com>

    Permission to use, copy, modify, and distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


= Example
Let's assume that a C dynamic library called <tt>libtest.so</tt> contains a function with
the following prototype: <tt>int test_function(SimpleStruct*)</tt>. The +SimpleStruct+
type is defined in a <tt>test_header.h</tt> file which looks like this:

   typedef struct SimpleStruct {
     int a;
     double d;
   } SimpleStruct;

Using Typelib, a Ruby script can interface with this library this way:

   require 'typelib'
   include Typelib

   # Load all types defined in the C file my_header.h
   registry = Registry.import('test_header.h')
   
   # Get a Library object for the target shared library, and get
   # a function handle in this library. +registry+ is used as the
   # type registry to access functions in this library
   lib = Library.open("libtest.so", registry)

   # This searches the test_function in libtest.so. Note that MyStruct must be
   # already defined in +registry+ (this is the case here).  See the
   # documentation of Typelib::Function for the various ways to define ruby/C
   # function bindings.
   test_function = lib.find('test_function').
		    returns('int').
		    with_arguments('MyStruct*')

   # Get the Ruby description of the MyStruct type
   my_struct = registry.get('MyStruct')

   # Create an uninitialized MyStruct parameter
   arg = my_struct.new
   arg.a = 10
   arg.b = 20.35498
   
   # and call the function, getting the integer return value
   result = test_function[struct]

   # It is also possible to use named parameters
   struct = my_struct.new :a => 10, :b => 20
   # .. or to even create the structure argument on the fly
   result = test_function[ :a => 10, :b => 20 ]

   # In fact, the value of this MyStruct argument is not used by test_function:
   # This argument is used as a buffer for output values. This can be set up
   # in Typelib with:
   test_function = registry.find('test_function').
			returns('int').
			returns('MyStruct*')
   result, struct = test_function[]

   # If the value was both used by +test_function+ and modified by it, we
   # would have used the following:
   test_function = registry.find('test_function').
		        returns('int').
			modifies('MyStruct*')
   result, arg = test_function[arg]

   # ... which can be useful if you want Typelib to build the object on-the-fly
   result, struct = test_function[:a => 10, :b => 20]

   # Then, getting the returned values is as simple as
   puts struct.a
   puts struct.b

Enjoy !

