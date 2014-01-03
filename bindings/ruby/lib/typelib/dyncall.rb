module Typelib
    # An opened C library
    class Library
	# The file name of this library
	attr_reader :name
	# The type registry for this library
        attr_reader :registry

	class << self
	    private :new
	end

	# Open the library at +libname+, associated with the type registry
	# +registry+. This returns a Library object.
        #
        # If no registry a given, an empty one is created.
        #
        # If auto_unload is true (the default), the shared object gets unloaded
        # when the Library object is deleted. Otherwise, it will never be
        # unloaded.
	def self.open(libname, registry = nil, auto_unload = true)
	    libname = File.expand_path(libname)
	    lib = wrap(libname, auto_unload)
            registry ||= Registry.new
	    lib.instance_variable_set("@name", libname)
	    lib.instance_variable_set("@registry", registry)
	    lib
	end
    end

    # A function found in a Typelib::Library. See Library#find
    class Function
	# The library in which this function is defined
	attr_reader :library
	# The function name
	attr_reader :name

	# The return type of this function, or nil if there is no return value
	attr_reader :return_type
	# The set of argument types.
	attr_reader :arguments
	# The set of argument indexes which contain returned data. See #modifies and #returns.
	attr_reader :returned_arguments

	def initialize(library, name)
	    @library = library
	    @name = name
	    @arguments = []
	    @returned_arguments = []
	    @arity = 0
	end

	# Specifies that this function returns a value of type +typename+. The
	# first call to +returns+ specifies the return value of the function
	# (in the C sense).  Subsequent calls specify that the following
	# argument is of type +typename+, and that the argument is a buffer
	# whose sole purpose is to be filled by the function call. In this case,
	# the buffer does not have to be given to #call: the Function object will create
	# one on the fly and #call will return it.
	#
	# For instance, the following standard C function:
	#
	#   int gettimeofday(struct timeval* tv, struct timezone* tz);
	#
	# Is wrapped using
	#
	#   gettimeofday = libc.find('gettimeofday').
	#	returns('int').
	#	returns('struct timeval*').
	#	with_arguments('struct timezone*')
	#
	# And called using
	#
	#   status, time = gettimeofday.call(nil)
	#
	# If the function does not have a return type but *has* a returned argument,
	# the return type must be explicitely specified as 'nil':
	#
	#   my_function.returns(nil).returns('int*')
	#
	def returns(typename)
	    if typename
		type = library.registry.build(typename)
		if type.null?
		    type = nil
		end
	    end

	    if defined? @return_type
		if !type
		    raise ArgumentError, "null type specified as argument"
		end

		@arguments << type
		@returned_arguments << @arguments.size
	    elsif typename
		@return_type = type
	    else
		@return_type = nil
	    end
	    self
	end

	# Specify the type of the following arguments
	def with_arguments(*typenames)
	    types = typenames.map { |n| library.registry.build(n) }
	    @arguments.concat(types)
	    @arity += typenames.size
	    self
	end

	# Specify that the following argument is a buffer which both provides
	# an argument to the function *and* will be modified by the function.
	# For instance, the select standard C function would be wrapped as
	# follows:
	#
	#   select = libc.find('select').
	#	returns('int').modifies('fd_set*').
	#	modifies('fd_set*').modifies('fd_set*').
	#	modifies('struct timeval*')
	#
	# and can then be used with:
	#
	#   status, read, write, except, timeout = 
	#	select.call(read, write, except, timeout)
	#
	# A feature of this library is the possibility to automatically build
	# memory buffers from Ruby values. So, for instance, the following C
	# function
	#
	#   void my_function(int* value);
	#
	# can be used as follows:
	#
	#   my_function = lib.wrap('my_function').modifies('int*')
	#
	#   new_value = my_function.call(5)
	#
	# The library will then automatically convert the Ruby integer '5' into
	# a C memory buffer of type 'int' and return the new Ruby value from
	# this buffer (new_value is a Ruby integer).
	def modifies(typename)
	    @arguments << library.registry.build(typename)
	    @returned_arguments << -@arguments.size
	    @arity += 1
	    self
	end

	# True if this function returns some value (either a return type or
	# some arguments which are also output values)
	def returns_something?; @return_type || !@returned_arguments.empty? end

	# How many arguments #call expects
	attr_reader :arity

	# Calls the function with +args+ for arguments
	def call(*args)
	    filtered_args, vm = prepare_call(args)
	    perform_call(filtered_args, vm)
	end

	alias :[] :call
	# Checks that +args+ is a valid set of arguments for this function
	def filter(*args); Typelib.filter_function_args(args, self) end
	alias :check :filter

	# Returns a CompiledCall object which binds this function to the
	# given argument. See CompiledCall#call
	def compile(*args)
	    filtered_args, vm = prepare_call(args)
	    CompiledCall.new(self, filtered_args, vm)
	end

	def prepare_call(args) # :nodoc:
	    filtered_args = Typelib.filter_function_args(args, self)
	    vm = prepare_vm(filtered_args)
	    return filtered_args, vm
	end

	def perform_call(filtered_args, vm) # :nodoc:
	    retval = vm.call(self)
	    return unless returns_something?

	    # Get the return array
	    ruby_returns = []
	    if return_type
		if MemoryZone === retval
		    ruby_returns << return_type.wrap(retval)
		else
		    ruby_returns << retval
		end
	    end

	    for index in returned_arguments
		ary_idx = index.abs - 1

		value = filtered_args[ary_idx]
		type  = arguments[ary_idx]

		if type < ArrayType
		    ruby_returns << type.wrap(value)
		else
		    ruby_returns << type.deference.wrap(value).to_ruby
		end
	    end

            if ruby_returns.size == 1
                return ruby_returns.first
            else
                return ruby_returns
            end
	end

    end

    # A CompiledCall object is a Function on which some arguments have already
    # been bound
    class CompiledCall
	def initialize(function, filtered_args, vm)
	    @function, @filtered_args, @vm = function, filtered_args, vm
	end

	def call
	    @function.perform_call(@filtered_args, @vm)
	end
    end

    # Implement Typelib's argument handling
    # This method filters a particular argument given the user-supplied value and 
    # the argument expected type. It raises TypeError if +arg+ is of the wrong type
    def self.filter_argument(arg, expected_type) # :nodoc:
	if !(expected_type < IndirectType) && !(Kernel.immediate?(arg) || Kernel.numeric?(arg))
	    raise TypeError, "#{arg.inspect} cannot be used for #{expected_type.name} arguments"
	end

	filtered =  if expected_type < IndirectType
			if MemoryZone === arg
			    arg
			elsif Type === arg
			    filter_value_arg(arg, expected_type)
                        elsif arg.nil?
                            expected_type.create_null
			elsif expected_type.deference.name == CHAR_T.name && arg.respond_to?(:to_str)
			    # Ruby strings ARE null-terminated
			    # The thing which is not checked here is that there is no NULL bytes
			    # inside the string.
			    arg.to_str.to_memory_ptr
			end
		    end

	if !filtered && (Kernel.immediate?(arg) || Kernel.numeric?(arg))
	    filtered = filter_numeric_arg(arg, expected_type)
	end

	if !filtered
          raise TypeError, "cannot use #{arg.inspect} for a #{expected_type.inspect} argument. Object ID for argument #{arg.class.object_id}, object ID for expected type #{expected_type.object_id}"
	end
	filtered
    end

    # Creates an array of objects that can safely be passed to function call mechanism
    def self.filter_function_args(args, function) # :nodoc:
        # Check we have the right count of arguments
        if args.size != function.arity
            raise ArgumentError, "#{function.arity} arguments expected, got #{args.size}"
        end
        
        # Create a Value object to collect the data we'll get
        # from output-only arguments
        function.returned_arguments.each do |index|
            next unless index > 0
            ary_idx = index - 1

	    type = function.arguments[ary_idx]
	    buffer = if type < ArrayType then type.new
		     elsif type < PointerType then type.deference.new
		     else
			 raise ArgumentError, "unexpected output type #{type.name}"
		     end

            args.insert(ary_idx, buffer) # works because return_spec is sorted
        end

	args.each_with_index do |arg, idx|
	    args[idx] = filter_argument(arg, function.arguments[idx])
	end
    end
end

