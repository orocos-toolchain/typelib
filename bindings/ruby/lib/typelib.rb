require 'enumerator'
require 'utilrb/logger'
require 'utilrb/object/singleton_class'
require 'utilrb/kernel/options'
require 'utilrb/module/attr_predicate'
require 'utilrb/module/const_defined_here_p'
require 'delegate'
require 'pp'
require 'facets/string/camelcase'
require 'set'
require 'utilrb/value_set'

if !defined?(Infinity)
    Infinity = 1e200 ** 200
end
if !defined?(Inf)
    Inf = Infinity
end
if !defined?(NaN)
    NaN = Infinity / Infinity
end

# Typelib is the main module for Ruby-side Typelib functionality.
#
# Typelib allows to do two things:
#
# * represent types (it is a <i>type system</i>). These representations will be
#   referred to as _types_ in the documentation.
# * manipulate in-memory values represented by these types. These are
#   referred to as _values_ in the documentation.
#
# As types may depend on each other (for instance, a structure depend on the
# types used to define its fields), Typelib maintains a consistent set of types
# in a so-called registry. Types in a registry can only refer to other types in
# the same registry.
#
# On the Ruby side, a _type_ is represented as a subclass of one of the
# specialized subclasses of Typelib::Type (depending of what kind of type it
# is). I.e.  a _type_ itself is a class, and the methods that are available on
# Type objects are the singleton methods of the Type class (or its specialized
# subclasses).  Then, a value is simply an instance of that same class.
#
# Typelib specializes for the following kinds of types:
#
# * structures and unions (Typelib::CompoundType)
# * static length arrays (Typelib::ArrayType)
# * dynamic containers (Typelib::ContainerType)
# * mappings from strings to numerical values (Typelib::EnumType)
#
# In other words:
#
#   registry = <load the registry>
#   type  = registry.get 'A' # Get the Type subclass that represents the A
#                            # structure
#   value = type.new         # Create an uninitialized value of type A
#
#   value.class == type # => true
#   type.ancestors # => [type, Typelib::CompoundType, Typelib::Type]
#
# Each class representing a type can be further specialized using
# Typelib.specialize_model and Typelib.specialize
# 
module Typelib
    extend Logger::Root('Typelib', Logger::WARN)

    class << self
	# If true (the default), typelib will load its type plugins. Otherwise,
        # it will not
        attr_predicate :load_type_plugins, true
    end
    @load_type_plugins = true

    # The namespace separator character used by Typelib
    NAMESPACE_SEPARATOR = '/'

    # Returns the basename part of +name+, i.e. the type name
    # without the namespace part.
    #
    # See also Type.basename
    def self.basename(name, separator = Typelib::NAMESPACE_SEPARATOR)
        name = do_basename(name)
        if separator && separator != Typelib::NAMESPACE_SEPARATOR
            name.gsub!(Typelib::NAMESPACE_SEPARATOR, separator)
        end
        name
    end

    # Returns the namespace part of +name+.  If +separator+ is
    # given, the namespace components are separated by it, otherwise,
    # the default of Typelib::NAMESPACE_SEPARATOR is used. If nil is
    # used as new separator, no change is made either.
    def self.namespace(name, separator = Typelib::NAMESPACE_SEPARATOR, remove_leading = false)
        ns = do_namespace(name)
        if remove_leading
            ns = ns[1..-1]
        end
        if separator && separator != Typelib::NAMESPACE_SEPARATOR
            ns.gsub!(Typelib::NAMESPACE_SEPARATOR, separator)
        end
        ns
    end

    def self.filter_methods_that_should_not_be_defined(on, reference_class, names, allowed_overloadings, msg_name, with_raw, &block)
        names.find_all do |n|
            candidates = [n, "#{n}="]
            if with_raw
                candidates.concat(["raw_#{n}", "raw_#{n}="])
            end
            candidates.all? do |method_name|
                if !reference_class.method_defined?(method_name) || allowed_overloadings.include?(method_name)
                    true
                else
                    msg_name ||= "instances of #{reference_class.name}"
                    Typelib.warn "NOT defining #{candidates.join(", ")} on #{msg_name} as it would overload a necessary method"
                    false
                end
            end
        end
    end

    def self.define_method_if_possible(on, reference_class, name, allowed_overloadings = [], msg_name = nil, &block)
        if !reference_class.method_defined?(name) || allowed_overloadings.include?(name)
            on.send(:define_method, name, &block)
            true
        else
            msg_name ||= "instances of #{reference_class.name}"
            Typelib.warn "NOT defining #{name} on #{msg_name} as it would overload a necessary method"
            false
        end
    end

    # Set of classes that have a #dup method but on which dup is forbidden
    DUP_FORBIDDEN = [TrueClass, FalseClass, Fixnum, Float, Symbol]

    def self.load_typelib_plugins
        if !ENV['TYPELIB_RUBY_PLUGIN_PATH'] || (@@typelib_plugin_path == ENV['TYPELIB_RUBY_PLUGIN_PATH'])
            return
        end

        ENV['TYPELIB_RUBY_PLUGIN_PATH'].split(':').each do |dir|
            specific_file = File.join(dir, "typelib_plugin.rb")
            if File.exists?(specific_file)
                require specific_file
            else
                Dir.glob(File.join(dir, '*.rb')) do |file|
                    require file
                end
            end
        end

        @@typelib_plugin_path = ENV['TYPELIB_RUBY_PLUGIN_PATH'].dup
    end
    @@typelib_plugin_path = nil
end

# Type models
require 'typelib/type'
require 'typelib/indirect_type'
require 'typelib/opaque_type'
require 'typelib/pointer_type'
require 'typelib/numeric_type'
require 'typelib/array_type'
require 'typelib/compound_type'
require 'typelib/enum_type'
require 'typelib/container_type'

require 'typelib/registry'
require 'typelib/cxx_registry'
require 'typelib/specializations'
require 'typelib_ruby'

require 'typelib/standard_convertions'

require 'typelib/path'
require 'typelib/accessor'

module Typelib
    # Generic method that converts a Typelib value into the corresponding Ruby
    # value.
    def self.to_ruby(value, original_type = nil)
        if value.respond_to?(:typelib_to_ruby)
            value = value.typelib_to_ruby
        end

        if (t = (original_type || value.class)).respond_to?(:to_ruby)
            # This allows to override Typelib's internal type convertion (mainly
            # for numerical types).
            t.to_ruby(value)
        else
            value
        end
    end

    # call-seq:
    #  Typelib.copy(to, from) => to
    # 
    # Proper copy of a value to another. +to+ and +from+ do not have to be from the
    # same registry, as long as the types can be casted into each other
    def self.copy(to, from)
        if to.respond_to?(:invalidate_changes_from_converted_types)
            to.invalidate_changes_from_converted_types
        end
        if from.respond_to?(:apply_changes_from_converted_types)
            from.apply_changes_from_converted_types
        end
        do_copy(to, from)
        to
    end

    # Initializes +expected_type+ from +arg+, where +arg+ can either be a value
    # of expected_type, a value that can be casted into a value of
    # expected_type, or a Ruby value that can be converted into a value of
    # +expected_type+.
    def self.from_ruby(arg, expected_type)      
        if arg.respond_to?(:apply_changes_from_converted_types)
            arg.apply_changes_from_converted_types
        end

        if arg.kind_of?(expected_type)
            return arg
        elsif arg.class < Type && arg.class.casts_to?(expected_type)
            return arg.cast(expected_type)
        elsif convertion = expected_type.convertions_from_ruby[arg.class]
            converted = convertion.call(arg, expected_type)
        elsif expected_type.respond_to?(:from_ruby)
            converted = expected_type.from_ruby(arg)
        else
            if !(expected_type < NumericType) && !arg.kind_of?(expected_type)
                reason =
                    if arg.class.name != expected_type.name
                        "types differ and there are not convertions from one to the other"
                    else
                        "the types have the same name but different definitions"
                    end
                raise ArgumentError, "cannot convert #{arg} to #{expected_type.name}: #{reason}"
            end
            converted = arg
        end
        if !(expected_type < NumericType) && !converted.kind_of?(expected_type)
            raise InternalError, "invalid conversion of #{arg} to #{expected_type.name}"
        end

        converted
    end

    # A raw, untyped, memory zone
    class MemoryZone
	def to_s
	    "#<MemoryZone:#{object_id} ptr=0x#{zone_address.to_s(16)}>"
	end
    end
end

if Typelib.with_dyncall?
require 'typelib/dyncall'
end

if ENV['TYPELIB_USE_GCCXML'] != '0'
    require 'typelib/gccxml'
end

