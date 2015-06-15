require 'facets/string/snakecase'
require 'facets/string/camelcase'
module Typelib
    module RegistryExport
        class Namespace < Module
            include RegistryExport
        end

        class NotFound < RuntimeError; end

        attr_reader :registry
        attr_reader :typename_prefix
        attr_reader :filter_block

        def reset_registry_export(registry = self.registry, filter_block = self.filter_block)
            @registry = registry
            @filter_block = filter_block
            @typename_prefix = '/'
            @__typelib_cache ||= Hash.new
            @__typelib_cache.clear
        end

        def disable_registry_export
            reset_registry_export(Typelib::Registry.new, nil)
        end

        def initialize_registry_export_namespace(mod, name)
            @registry = mod.registry
            @typename_prefix = "#{mod.typename_prefix}#{name}/"
            @filter_block = mod.filter_block
        end

        def to_s
            "#{typename_prefix}*"
        end

        def pretty_print(pp)
            pp.text to_s
        end

        def self.setup_subnamespace(parent, mod, name)
            mod.extend RegistryExport
            mod.initialize_registry_export_namespace(parent, name)
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

        def resolve_call_from_exported_registry(relaxed_naming, m, *args)
            @__typelib_cache ||= Hash.new
            if type = @__typelib_cache[[m, args]]
                return type
            elsif type = RegistryExport.find_type(relaxed_naming, typename_prefix, registry, m, *args)
                exported_type =
                    if type.convertion_to_ruby
                        type.convertion_to_ruby[0] || type
                    else
                        type
                    end

                if filter_block
                    exported_type = filter_block.call(type, exported_type)
                end
                exported_type = @__typelib_cache[[m, args]] =
                    if exported_type.respond_to?(:convertion_to_ruby) && exported_type.convertion_to_ruby
                        exported_type.convertion_to_ruby[0] || exported_type
                    else exported_type
                    end
                RegistryExport.setup_subnamespace(self, exported_type, type.basename)
                exported_type
            elsif basename = RegistryExport.find_namespace(relaxed_naming, typename_prefix, registry, m, *args)
                ns = Namespace.new
                RegistryExport.setup_subnamespace(self, ns, basename)
                @__typelib_cache[[name, []]] = ns
            end
        end

        def method_missing(m, *args, &block)
            if type = resolve_call_from_exported_registry(false, m.to_s, *args)
                return type
            else
                template_args = RegistryExport.template_args_to_typelib(args)
                raise NotFound, "cannot find a type named #{typename_prefix}#{m}#{template_args} in registry"
            end
        end

        def const_missing(name)
            if type = resolve_call_from_exported_registry(true, name.to_s)
                return type
            else
                raise NotFound, "cannot find a type named #{typename_prefix}#{name}, or a type named like that after a CamelCase or snake_case conversion, in registry"
            end
        end
    end
end

