require 'typelib/gccxml'
require 'typelib/clang'

module Typelib
    module CXX
        def self.parse_template(name)
            tokens = template_tokenizer(name)

            type_name = tokens.shift
            arguments = collect_template_arguments(tokens)
            arguments.map! do |arg|
                arg.join("")
            end
            return type_name, arguments
        end

        def self.collect_template_arguments(tokens)
            level = 0
            arguments = []
            current = []
            while !tokens.empty?
                case tk = tokens.shift
                when "<"
                    level += 1
                    if level > 1
                        current << "<" << tokens.shift
                    else
                        current = []
                    end
                when ">"
                    level -= 1
                    if level == 0
                        arguments << current
                        current = []
                        break
                    else
                        current << ">"
                    end
                when ","
                    if level == 1
                        arguments << current
                        current = []
                    else
                        current << "," << tokens.shift
                    end
                else
                    current << tk
                end
            end
            if !current.empty?
                arguments << current
            end

            return arguments
        end

        def self.template_tokenizer(name)
            suffix = name
            result = []
            while !suffix.empty?
                suffix =~ /^([^<,>]*)/
                match = $1.strip
                if !match.empty?
                    result << match
                end
                char   = $'[0, 1]
                suffix = $'[1..-1]

                break if !suffix

                result << char
            end
            result
        end

        CXX_LOADERS = Hash[
            'gccxml' => GCCXMLLoader,
            'clang'  => CLangLoader
        ]

        class << self
            # Explicitly sets {loader}
            attr_writer :loader
        end

        # Returns the current C++ loader object
        #
        # The value of {loader} is initialized either by setting it explicitely
        # with {loader=} or by setting the TYPELIB_CXX_LOADER to the name of a
        # loader registered in {CXX_LOADERS}.
        #
        # The default is currently GCCXMLLoader
        #
        # @return [#load,#preprocess] a loader object suitable for operating
        #   on C++ files
        def self.loader
            if instance_variable_defined?(:@loader)
                @loader
            elsif cxx_loader_name = ENV['TYPELIB_CXX_LOADER']
                cxx_loader = CXX_LOADERS[cxx_loader_name]
                if !cxx_loader
                    raise ArgumentError, "#{cxx_loader_name} is not a known C++ loader, known loaders are '#{CXX_LOADERS.keys.sort.join("', '")}'"
                end
                cxx_loader
            else
                GCCXMLLoader
            end
        end

        # Loads a C++ file and imports it in the given registry, based on the
        # current C++ importer setting
        def self.load(registry, file, kind, options)
            loader.load(registry, file, kind, options)
        end

        def self.preprocess(files, kind, options)
            loader.preprocess(files, kind, options)
        end

        Registry::TYPE_HANDLERS['c'] = method(:load)
    end
end

