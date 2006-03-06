= Ruby bindings

== Limitations
For now, double and float support for the Ruby/DL interface is disabled
since it does not work well on non-i386. Trying that raises an ArgumentError

== Example
   include Typelib

   # Load all types from my_header.h
   registry = Registry.import('my_header.h')
   my_function = registry.wrap('my_function', 'int', 'MyStruct*')

   # Get the type definition
   my_struct = registry.get('MyStruct')
   struct = my_struct.new
   struct.a = 10
   struct.b = 20
   
   result = my_function[struct]

   # Additionally, we can construct structures using named parameters
   struct = my_struct.new :a => 10, :b => 20
   # Additionally, we can construct structures on-the-fly
   result = my_function[ :a => 10, :b => 20 ]

   # If a parameter is output-only, you can tell it to Typelib and 
   # don't pass it to the wrapper
   my_function = registry.wrap('my_function', [ 'int', 1 ], 'MyStruct*')
   result, struct = my_function[]

   # It can also be input-output, and then you must pass an object yourself. The same
   # object is returned by the wrapper, to show that the argument is modified
   my_function = registry.wrap('my_function', [ 'int', -1 ], 'MyStruct*')
   result, struct = my_function[struct]

   # ... which can be useful if you make Typelib build the object on-the-fly
   result, struct = my_function[:a => 10, :b => 20]

