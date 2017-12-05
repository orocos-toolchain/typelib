require 'facets/string/snakecase'
require 'facets/string/camelcase'
module Typelib
    module RegistryExport
        class Namespace < Module
            attr_reader :registry

            include RegistryExport

            def reset_registry_export(registry = self.registry, *)
                @registry = registry
                super
            end

            def to_s
                "#{@__typelib_registry_export_typename_prefix}*"
            end

            def pretty_print(pp)
                pp.text to_s
            end

            def method_missing(m, *args, &block)
                if type = resolve_call_from_exported_registry(false, m.to_s, *args)
                    return type
                else
                    super
                end
            rescue NoMethodError
                template_args = RegistryExport.template_args_to_typelib(args)
                raise NotFound, "cannot find a type named #{__typelib_registry_export_typename_prefix}#{m}#{template_args} in registry"
            end
        end

        class NotFound < RuntimeError; end

        attr_reader :__typelib_registry_export_typename_prefix
        attr_reader :__typelib_registry_export_filter_block

        def reset_registry_export(registry = self.registry,
                                  filter_block = @__typelib_registry_export_filter_block,
                                  typename_prefix = @__typelib_registry_export_typename_prefix)
            if registry && (self.registry != registry)
                raise RuntimeError, "setting up #{self} to be an export type from #{registry}, but it is already exporting types from #{self.registry}"
            end

            @__typelib_registry_export_filter_block = filter_block
            @__typelib_registry_export_typename_prefix = typename_prefix
            @__typelib_registry_export_cache = Hash.new
        end

        def initialize_registry_export(mod, name)
            reset_registry_export(mod.registry,
                                  mod.__typelib_registry_export_filter_block,
                                  "#{mod.__typelib_registry_export_typename_prefix}#{name}/")
        end

        def disable_registry_export
            reset_registry_export(nil, nil)
        end

        def self.template_args_to_typelib(args)
            if !args.empty?
                args = args.map do |v|
                    if v.respond_to?(:name)
                        v.name
                    else v.to_s
                    end
                end
                "<#{args.join(",")}>"
            end
        end

        def self.each_candidate_name(relaxed_naming, m, *args)
            template_args = template_args_to_typelib(args)
            yield("#{m}#{template_args}")
            return if !relaxed_naming
            Typelib.warn "possible old-style access on Types, use Types.namespace.of.Type instead of Types::Namespace::Of::Type"
            caller.each do |call|
                Typelib.warn "  #{call}"
            end
            yield("#{m.snakecase}#{template_args}")
            yield("#{m.camelcase}#{template_args}")
        end


        def self.find_namespace(relaxed_naming, typename_prefix, registry, m, *args)
            each_candidate_name(relaxed_naming, m, *args) do |basename|
                if registry.each("#{typename_prefix}#{basename}/").first
                    return basename
                end
            end

            return if !relaxed_naming

            # Try harder ... for weird naming conventions ... but that's costly
            prefix = Typelib.split_typename(typename_prefix)
            template_args = template_args_to_typelib(args)
            basename = "#{m}#{template_args}".snakecase
            registry.each(typename_prefix) do |type|
                name = type.split_typename
                next if name[0, prefix.size] != prefix
                if name[prefix.size].snakecase == basename
                    return name[prefix.size]
                end
            end
            nil
        end

        def self.find_type(relaxed_naming, typename_prefix, registry, m, *args)
            each_candidate_name(relaxed_naming, m, *args) do |basename|
                if registry.include?(typename = "#{typename_prefix}#{basename}")
                    return registry.get(typename)
                end
            end

            return if !relaxed_naming

            # Try harder ... for weird naming conventions ... but that's costly
            template_args = template_args_to_typelib(args)
            registry.each(typename_prefix) do |type|
                if type.name =~ /^#{Regexp.quote(typename_prefix)}[^\/]+$/o
                    if type.basename.snakecase == "#{m.to_s.snakecase}#{template_args}"
                        return type
                    end
                end
            end
            nil
        end

        def RegistryExportDelegate(ruby_class)
            klass = DelegateClass(ruby_class.singleton_class).new(ruby_class)
            m = Module.new do
                include RegistryExport

                attr_reader :registry

                def reset_registry_export(registry = self.registry, *args)
                    @registry = registry
                    super
                end
            end
            klass.extend m
            klass
        end

        def setup_export_type(type, basename)
            if type <= Type
                type.extend RegistryExport
            else
                type = RegistryExportDelegate(type)
            end
            type.initialize_registry_export(self, basename)
            type
        end

        def resolve_call_from_exported_registry(relaxed_naming, m, *args)
            @__typelib_registry_export_cache ||= Hash.new
            if type = @__typelib_registry_export_cache[[m, args]]
                return type
            elsif type = RegistryExport.find_type(relaxed_naming, @__typelib_registry_export_typename_prefix, registry, m, *args)
                exported_type =
                    if type.convertion_to_ruby
                        type.convertion_to_ruby[0] || type
                    else
                        type
                    end

                if filter_block = @__typelib_registry_export_filter_block
                    filtered_type = filter_block.call(type, exported_type)
                    if filtered_type.respond_to?(:registry) && filtered_type.registry != registry
                        raise RuntimeError, "filter block #{filter_block} returned #{filtered_type} which is a type from #{filtered_type.registry} but I was expecting #{registry}"
                    end
                    if filtered_type != exported_type
                        exported_type =
                            if filtered_type.respond_to?(:convertion_to_ruby) && filtered_type.convertion_to_ruby
                                filtered_type.convertion_to_ruby[0] || filtered_type
                            else filtered_type
                            end
                    end
                end
                @__typelib_registry_export_cache[[m, args]] = setup_export_type(exported_type, type.basename)
            elsif basename = RegistryExport.find_namespace(relaxed_naming, @__typelib_registry_export_typename_prefix, registry, m, *args)
                ns = Namespace.new
                ns.initialize_registry_export(self, basename)
                @__typelib_registry_export_cache[[name, []]] = ns
            end
        end

        def method_missing(m, *args, &block)
            if type = resolve_call_from_exported_registry(false, m.to_s, *args)
                return type
            else
                super
            end
        end

        def const_missing(name)
            if type = resolve_call_from_exported_registry(true, name.to_s)
                return type
            else
                raise NotFound, "cannot find a type named #{@__typelib_registry_export_typename_prefix}#{name}, or a type named like that after a CamelCase or snake_case conversion, in registry"
            end
        end
    end
end

