module Typelib
    # In Typelib, a registry contains a consistent set of types, i.e. the types
    # are that are related to each other.
    #
    # As mentionned in the Typelib module documentation, it is better to
    # manipulate value objects from types from the same registry. That is more
    # efficient, as it removes the need to compare the type definitions whenever
    # the values are manipulated together.
    #
    # I.e., it is better to use a global registry to represent all the types
    # used in your application. In case you need to load different registries,
    # that can be achieved by +merging+ them together (which will work only if
    # the type definitions match between the registries).
    class Registry
        TYPE_BY_EXT = {
            ".c" => "c",
            ".cc" => "c",
            ".cxx" => "c",
            ".cpp" => "c",
            ".h" => "c",
            ".hh" => "c",
            ".hxx" => "c",
            ".hpp" => "c",
            ".tlb" => "tlb"
        }

        TYPE_HANDLERS = Hash.new

        def dup
            copy = self.class.new
            copy.merge(self)
            copy
        end

        # Generates the smallest new registry that allows to define a set of types
        #
        # @overload minimal(type_name, with_aliases = true)
        #   @param [String] type_name the name of a type
        #   @param [Boolean] with_aliases if true, aliases defined in self that
        #     point to types ending up in the minimal registry will be copied
        #   @return [Typelib::Registry] the registry that allows to define the
        #     named type
        #
        # @overload minimal(auto_types, with_aliases = true)
        #   @param [Typelib::Registry] auto_types a registry containing the
        #     types that are not necessary in the result
        #   @param [Boolean] with_aliases if true, aliases defined in self that
        #     point to types ending up in the minimal registry will be copied
        #   @return [Typelib::Registry] the registry that allows to define all
        #     types of self that are not in auto_types. Note that it may contain
        #     types that are in the auto_types registry, if they are needed to
        #     define some other type
	def minimal(type, with_aliases = true)
	    do_minimal(type, with_aliases)
	end

        # Creates a new registry by loading a typelib XML file
        #
        # @see Registry#merge_xml
        def self.from_xml(xml)
            reg = Typelib::Registry.new
            reg.merge_xml(xml)
            reg
        end

        # Enumerate the types contained in this registry
        #
        # @overload each(prefix, :with_aliases => false)
        #   Enumerates the types and not the aliases
        #
        #   @param [nil,String] prefix if non-nil, only types whose name is this prefix
        #     will be enumerated
        #   @yieldparam [Model<Typelib::Type>] type a type
        #
        # @overload each(prefix, :with_aliases => true)
        #   Enumerates the types and the aliases
        #
        #   @param [nil,String] prefix if non-nil, only types whose name is this prefix
        #     will be enumerated
        #   @yieldparam [String] name the type name, it is different from type.name for
        #     aliases
        #   @yieldparam [Model<Typelib::Type>] type a type
        def each(filter = nil, options = Hash.new, &block)
            if filter.kind_of?(Hash)
                filter, options = nil, filter
            end

            options = Kernel.validate_options options,
                :with_aliases => false

            if !block_given?
                enum_for(:each, filter, options)
            else
                each_type(filter, options[:with_aliases], &block)
            end
        end
        include Enumerable

        # Tests for the presence of a type by its name
        #
        # @param [String] name the type name
        # @return [Boolean] true if this registry contains a type named like
        #   this
        def include?(name)
            includes?(name)
        end

        attr_reader :exported_type_to_real_type
        attr_reader :real_type_to_exported_type

        def setup_type_export_module(mod)
            if !mod.respond_to?('find_exported_template')
                mod.extend(TypeExportNamespace)
                mod.registry = self
            end
        end

        def export_solve_namespace(base_module, typename)
            @export_namespaces ||= Hash.new
            @export_namespaces[base_module] ||= Hash.new
            if result = export_namespaces[base_module][typename]
                return result
            end

            namespace = Typelib.split_typename(typename)
            basename = namespace.pop

            setup_type_export_module(base_module)
            
            mod = namespace.inject(base_module) do |mod, ns|
                template_basename, template_args = GCCXMLLoader.parse_template(ns)
                ns = template_basename.gsub(/\s+/, '_').camelcase(:upper)

                if template_args.empty?
                    mod = 
                        if mod.const_defined_here?(ns)
                            mod.const_get(ns)
                        else
                            result = Module.new
                            mod.const_set(ns, result)
                            result
                        end

                    setup_type_export_module(mod)
                    mod
                else
                    # Must already be defined as it is an actual type object,
                    # not a namespace
                    template_args.map! do |s|
                        if s =~ /^\d+$/ then Integer(s)
                        else s
                        end
                    end
                    if !mod.respond_to?(:find_exported_template)
                        return
                    end
                    template = mod.find_exported_template(ns, [], template_args)
                    return if !template
                    template
                end
            end
            export_namespaces[base_module][typename] = [basename, mod]
        end

        attr_reader :export_typemap
        attr_reader :export_namespaces

        class InconsistentTypeExport < RuntimeError
            attr_reader :path
            attr_reader :existing_type
            attr_reader :new_type

            def initialize(path, existing_type, new_type, message = nil)
                super(message)
                @path = path
                @existing_type = existing_type
                @new_type = new_type
            end

            def exception(message = nil)
                if message.respond_to?(:to_str)
                    self.class.new(path, existing_type, new_type, message)
                else
                    self
                end
            end

            def pretty_print(pp)
                pp.text "there is a type registered at #{path} that differs from the one we are trying to register"
                pp.breakable
                pp.text "registered type is"
                pp.nest(2) do
                    pp.breakable
                    existing_type.pretty_print(pp)
                end
                pp.breakable
                pp.text "new type is"
                pp.nest(2) do
                    pp.breakable
                    new_type.pretty_print(pp)
                end
            end
        end

        # Export this registry in the Ruby namespace. The base namespace under
        # which it should be done is given in +base_module+
        def export_to_ruby(base_module = Kernel, options = Hash.new)
            options = Kernel.validate_options options,
                :override => false, :excludes => nil

            override   = options[:override]
            exclude_rx = /^$/
            if options[:excludes].respond_to?(:each)
                rx_elements = []
                options[:excludes].each do |s|
                    if s.respond_to?(:to_str)
                        rx_elements << "^#{Regexp.quote(s)}$"
                    elsif s.kind_of?(Regexp)
                        rx_elements << s.to_s
                    end
                end
                exclude_rx = Regexp.new(rx_elements.join("|"))
            elsif options[:excludes]
                exclude_rx = Regexp.new("^#{Regexp.quote(options[:excludes])}$")
            end

            new_export_typemap = Hash.new
            each(:with_aliases => true) do |name, type|
                next if name =~ exclude_rx

                basename, mod = export_solve_namespace(base_module, name)
                if !mod.respond_to?(:find_exported_template)
                    next
                end
                next if !mod

                exported_type =
                    if type.convertion_to_ruby
                        type.convertion_to_ruby[0] || type
                    else
                        type
                    end

                if block_given?
                    exported_type = yield(name, type, mod, basename, exported_type)
                    next if !exported_type
                    if !exported_type.kind_of?(Class)
                        raise ArgumentError, "the block given to export_to_ruby must return a Ruby class or nil (got #{exported_type})"
                    end

                    if exported_type.respond_to?(:convertion_to_ruby) && exported_type.convertion_to_ruby
                        exported_type = exported_type.convertion_to_ruby[0] || exported_type
                    end
                end

                new_export_typemap[exported_type] ||= Set.new
                new_export_typemap[exported_type] << [type, mod, basename]

                if (existing_exports = @export_typemap[exported_type]) && existing_exports.include?([type, mod, basename])
                    next
                end

                if type <= Typelib::PointerType || type <= Typelib::ArrayType
                    # We ignore arrays for now
                    # export_array_to_ruby(mod, $1, Integer($2), exported_type)
                    next
                end

                template_basename, template_args = GCCXMLLoader.parse_template(basename)
                if template_args.empty?
                    basename = basename.gsub(/\s+/, '_').camelcase(:upper)

                    if mod.const_defined_here?(basename)
                        existing_type = mod.const_get(basename)
                        if override
                            mod.const_set(basename, exported_type)
                        elsif !(existing_type <= exported_type)
                            if existing_type.class == Module
                                # The current value is a namespace, the new a
                                # type. We probably loaded a nested definition
                                # first and then the parent type. Migrate one to
                                # the other
                                mod.const_set(basename, exported_type)
                                setup_type_export_module(exported_type)
                                existing_type.exported_types.each do |name, type|
                                    exported_type.const_set(name, type)
                                end
                            else
                                raise InconsistentTypeExport.new("#{mod.name}::#{basename}", existing_type, exported_type), "there is a type registered at #{mod.name}::#{basename} which differs from the one in the registry, and override is false"
                            end
                        end
                    else
                        mod.exported_types[basename] = type
                        mod.const_set(basename, exported_type)
                    end
                else
                    export_template_to_ruby(mod, template_basename, template_args, type, exported_type)
                end
            end

        ensure
            @export_typemap = new_export_typemap
        end

        # Remove all types exported by #export_to_ruby under +mod+, and
        # de-registers them from the export_typemap
        def clear_exports(export_target_mod)
            if !export_target_mod.respond_to?(:exported_types)
                return
            end

            found_type_exports = ValueSet.new
            
            export_target_mod.constants.each do |c|
                c = export_target_mod.const_get(c)
                if c.respond_to?(:exported_types)
                    clear_exports(c)
                end
            end
            export_target_mod.exported_types.each do |const_name, type|
                found_type_exports.insert(export_target_mod.const_get(const_name))
                export_target_mod.send(:remove_const, const_name)
            end
            export_target_mod.exported_types.clear

            found_type_exports.each do |exported_type|
                set = export_typemap[exported_type]
                set.delete_if do |_, mod, _|
                    export_target_mod == mod
                end
                if set.empty?
                    export_typemap.delete(exported_type)
                end
            end
        end

        module TypeExportNamespace
            attr_writer :registry
            def registry
                if @registry then @registry
                elsif superclass.respond_to?(:registry)
                    superclass.registry
                end
            end
            attribute(:exported_types) { Hash.new }
            attribute(:exported_templates) { Hash.new }

            def find_exported_template(template_basename, args, remaining)
                if remaining.empty?
                    return exported_templates[[template_basename, args]]
                end

                head, *tail = remaining

                # Accept Ruby types in place of the corresponding Typelib type,
                # and accept type objects in place of type names
                if head.kind_of?(Class)
                    if real_types = registry.export_typemap[head]
                        head = real_types.map do |t, _|
                            t.name
                        end
                    else
                        raise ArgumentError, "#{arg} is not a type exported from a Typelib registry"
                    end
                end

                if head.respond_to?(:each)
                    args << nil
                    for a in head
                        args[-1] = a
                        if result = find_exported_template(template_basename, args, tail)
                            return result
                        end
                    end
                    return nil
                else
                    args << head
                    return find_exported_template(template_basename, args, tail)
                end
            end
        end

        def export_template_to_ruby(mod, template_basename, template_args, actual_type, exported_type)
            template_basename = template_basename.gsub(/\s+/, '_').camelcase(:upper)

            if !mod.respond_to?(template_basename)
                mod.singleton_class.class_eval do
                    define_method(template_basename) do |*args|
                        if !(result = find_exported_template(template_basename, [], args))
                            raise ArgumentError, "there is no template instanciation #{template_basename}<#{args.join(",")}> available"
                        end
                        result
                    end
                end
            end

            template_args = template_args.map do |arg|
                if arg =~ /^\d+$/
                    Integer(arg)
                else
                    arg
                end
            end
            exports = mod.exported_templates
            exports[[template_basename, template_args]] = exported_type
        end

	# Returns the file type as expected by Typelib from 
	# the extension of +file+ (see TYPE_BY_EXT)
	#
	# Raises RuntimeError if the file extension is unknown
        def self.guess_type(file)
	    ext = File.extname(file)
            if type = TYPE_BY_EXT[ext]
		type
            else
                raise "Cannot guess file type for #{file}: unknown extension '#{ext}'"
            end
        end

	# Format +option_hash+ to the form expected by do_import
	# (Yes, I'm lazy and don't want to handles hashes in C)
        def self.format_options(option_hash) # :nodoc:
            option_hash.to_a.collect do |opt|
                if opt[1].kind_of?(Array)
                    if opt[1].first.kind_of?(Hash)
                        [ opt[0].to_s, opt[1].map { |child| format_options(child) } ]
                    else
                        [ opt[0].to_s, opt[1].map { |child| child.to_s } ]
                    end
                elsif opt[1].kind_of?(Hash)
                    [ opt[0].to_s, format_options(opt[1]) ]
                else
                    [ opt[0].to_s, opt[1].to_s ]
                end
            end
        end

        # Shortcut for
        #   registry = Registry.new
        #   registry.import(args)
        #
        # See Registry#import
        def self.import(*args)
            registry = Registry.new
            registry.import(*args)
            registry
        end

        def initialize(load_plugins = nil)
	    if load_plugins || (load_plugins.nil? && Typelib.load_type_plugins?)
            	Typelib.load_typelib_plugins
            end
            @export_typemap = Hash.new
	    super()
        end

        # Returns the handler that will be used to import that file. It can
        # either be a string, in which case we use a Typelib internal importer,
        # or a Ruby object responding to 'call' in which case Registry#import
        # will use that object to do the importing.
        def self.handler_for(file, kind = 'auto')
	    file = File.expand_path(file)
            if !kind || kind == 'auto'
                kind    = Registry.guess_type(file)
            end
            if handler = TYPE_HANDLERS[kind]
                return handler
            end
            return kind
        end

        # Imports the +file+ into this registry. +kind+ is the file format or
        # nil, in which case the file format is guessed by extension (see
        # TYPE_BY_EXT)
	# 
        # +options+ is an option hash. The Ruby bindings define the following
        # specific options:
	# merge:: 
        #   merges +file+ into this repository. If this is false, an exception
        #   is raised if +file+ contains types already defined in +self+, even
        #   if the definitions are the same.
	#
	#     registry.import(my_path, 'auto', :merge => true)
	#
	# The Tlb importer has no options
	#
        # The C importer defines the following options: preprocessor:
        #
	# define:: 
        #   a list of VAR=VALUE or VAR options for cpp
	#     registry.import(my_path, :define => ['PATH=/usr', 'NDEBUG'])
	# include:: 
        #   a list of path to add to cpp's search path
	#     registry.import(my_path, :include => ['/usr', '/home/blabla/prefix/include'])
	# rawflags:: 
        #   flags to be passed as-is to cpp. For instance, the two previous
        #   examples can be written
	#
	#   registry.import(my_path, 'auto',
	#     :rawflags => ['-I/usr', '-I/home/blabla/prefix/include', 
	#                  -DPATH=/usr', -DNDEBUG])
	# debug::
        #   if true, debugging information is outputted on stdout, and the
        #   preprocessed output is kept.
        #
        # merge::
        #   load the file into its own registry, and merge the result back into
        #   this one. If it is not set, types defined in +file+ that are already
        #   defined in +self+ will generate an error, even if the two
        #   definitions are the same.
	#
        def import(file, kind = 'auto', options = {})
	    file = File.expand_path(file)

            handler = Registry.handler_for(file, kind)
            if handler.respond_to?(:call)
                return handler.call(self, file, kind, options)
            else
                kind = handler
            end

            do_merge = 
                if options.has_key?('merge') then options.delete('merge')
                elsif options.has_key?(:merge) then options.delete(:merge)
                else true
                end

            options = Registry.format_options(options)

            do_import(file, kind, do_merge, options)
        end

        # Resizes the given type to the given size, while updating the rest of
        # the registry to keep it consistent
        #
        # In practice, it means it modifies the compound field offsets and
        # sizes, and modifies the array sizes so that it matches the new sizes.
        #
        # +type+ must either be a type class or a type name, and to_size the new
        # size for it.
        #
        # See #resize to resize multiple types in one call.
        def resize_type(type, to_size)
            resize(type => to_size)
        end

        # Resize a set of types, while updating the rest of the registry to keep
        # it consistent
        #
        # In practice, it means it modifies the compound field offsets and
        # sizes, and modifies the array sizes so that it matches the new sizes.
        #
        # The given type map must be a mapping from a type name or type class to
        # the new size for that type.
        #
        # See #resize to resize multiple types in one call.
        def resize(typemap)
            new_sizes = typemap.map do |type, size|
                if type.respond_to?(:name)
                    [type.name, size]
                else
                    [type.to_str, size]
                end
            end

            do_resize(new_sizes)
            nil
        end

	# Exports the registry in the provided format, into a Ruby string. The
	# following formats are allowed as +format+:
	# 
	# +tlb+:: Typelib's own XML format
	# +idl+:: CORBA IDL
	# 
	# +options+ is an option hash, which is export-format specific. See the C++
	# documentation of each exporter for more information.
	def export(kind, options = {})
            options = Registry.format_options(options)
            do_export(kind, options)
	end

        # Export the registry into Typelib's own XML format
        def to_xml
            export('tlb')
        end

        # Helper class for Registry#create_compound
        class CompoundBuilder
            # The compound type name
            attr_reader :name
            # The registry on which we are creating the new compound
            attr_reader :registry
            # The compound fields, registered at each called of #method_missing
            # and #add. The new type is registered when #build is called.
            attr_reader :fields
            # The compound size, as specified on object creation. If zero, it is
            # automatically computed
            attr_reader :size

            def initialize(name, registry, size = 0)
                @name, @registry, @size = name, registry, size
                @fields = []
            end
            
            # Create the type on the underlying registry
            def build
                if @fields.empty?
                    raise ArgumentError, "trying to create an empty compound"
                end

                # Create the offsets, if they are not provided
                current_offset = 0
                fields.each do |field_def|
                    field_def[2] ||= current_offset
                    current_offset = field_def[2] + field_def[1].size
                end
                registry.do_create_compound(name, fields, size)
            end

            # Adds a new field
            #
            # @param [String] name the field name
            # @param [Type,String] type the field's typelib type
            # @param [Integer,nil] offset the field offset. If nil, it is
            #   automatically computed in #build so as to follow the previous
            #   field.
            def add(name, type, offset = nil)
                if type.respond_to?(:to_str) || type.respond_to?(:to_sym)
                    type = registry.build(type.to_s)
                end
                @fields << [name.to_s, type, offset]
            end

            # See {Registry#add} for the alternative to #add
            def method_missing(name, *args, &block)
                if name.to_s =~ /^(.*)=$/
                    type = args.first
                    name = $1
                    add($1, type)
                else
                    super
                end
            end
        end

        # Creates a new compound type with the given name on this registry
        #
        # @yield [CompoundBuilder] the compound building helper, see below for
        #   examples. Only the #add method allows to set offsets. When no
        #   offsets are given, they are computed from the previous field.
        # @return [Type] the type representation
        #
        # @example create a compound using #add
        #   registry.create_compound "/new/Compound" do |c|
        #     c.add "field0", "/int", 15
        #     c.add "field1", "/another/Compound", 20
        #   end
        #
        # @example create a compound using the c.field = type syntax
        #   registry.create_compound "/new/Compound" do |c|
        #     c.field0 = "/int"
        #     c.field1 = "/another/Compound"
        #   end
        #
        def create_compound(name, size = 0)
            recorder = CompoundBuilder.new(name, self, size)
            yield(recorder)
            recorder.build
        end

        # Creates a new container type on this registry
        #
        # @param [String] container_type the name of the container type
        # @param [String,Type] element_type the type of the container elements,
        #   either as a type or as a type name
        #
        # @example create a new std::vector type
        #   registry.create_container "/std/vector", "/my/Container"
        def create_container(container_type, element_type, size = 0)
            if element_type.respond_to?(:to_str)
                element_type = build(element_type)
            end
            return define_container(container_type.to_str, element_type, size)
        end

        # Creates a new array type on this registry
        #
        # @param [String,Type] base_type the type of the array elements,
        #   either as a type or as a type name
        # @param [Integer] size the array size
        #
        # @example create a new array of 10 elements
        #   registry.create_array "/my/Container", 10
        def create_array(base_type, element_count, size = 0)
            if base_type.respond_to?(:name)
                base_type = base_type.name
            end
            return build("#{base_type}[#{element_count}]", size)
        end

        # Helper class to build new enumeration types
        class EnumBuilder
            # [String] The enum type name
            attr_reader :name
            # [Registry] The registry on which the enum should be defined
            attr_reader :registry
            # [Array<Array(String,Integer)>] the enumeration name-to-integer
            #   mapping. Values are added with #add
            attr_reader :symbols
            # Size, in bytes. If zero, it is automatically computed
            attr_reader :size

            def initialize(name, registry, size)
                @name, @registry, @size = name, registry, size
                @symbols = []
            end
            
            # Creates the new enum type on the registry
            def build
                if @symbols.empty?
                    raise ArgumentError, "trying to create an empty enum"
                end

                # Create the values, if they are not provided
                current_value = 0
                symbols.each do |sym_def|
                    sym_def[1] ||= current_value
                    current_value = sym_def[1] + 1
                end
                registry.do_create_enum(name, symbols, size)
            end

            # Add a new symbol to this enum
            def add(name, value = nil)
                symbols << [name.to_s, (Integer(value) if value)]
            end

            # Alternative method to add new symbols. See {Registry#create_enum}
            # for examples.
            def method_missing(name, *args, &block)
                if name.to_s =~ /^(.*)=$/
                    value = args.first
                    add($1, value)
                elsif args.empty?
                    add(name.to_s)
                else
                    super
                end
            end
        end

        # Creates a new enum type with the given name on this registry
        #
        # @yield [EnumBuilder] the enum building helper. In both methods, if a
        #   symbol's value is not provided, it is computed as last_value + 1
        # @return [Type] the type representation
        #
        # @example create an enum using #add
        #   registry.create_enum "/new/Enum" do |c|
        #     c.add "sym0", 15
        #     c.add "sym1", 2
        #     # sym2 will be 3
        #     c.add "sym2"
        #   end
        #
        # @example create an enum using the c.sym = value syntax
        #   registry.create_enum "/new/Enum" do |c|
        #     c.sym0 = 15
        #     c.sym1 = 2
        #     # sym2 will be 3
        #     c.sym2
        #   end
        #
        def create_enum(name, size = 0)
            recorder = EnumBuilder.new(name, self, size)
            yield(recorder)
            recorder.build
        end
    end
end
