require 'set'
require 'tempfile'

module Typelib

    class Registry
        # Imports the given C++ file into the registry using CLANG
        def self.load_from_clang(registry, file, kind, options)
            
            #this gives us an array of opaques
            opaques = options[:opaques]
            
            #create a registry from it
            # FIXME: instead of usign the "Tempfile", just use a fixed known file in the rock-install for human inspection. to be removed!
            #opaqueReg = Tempfile.open("tmp_opaques")
            opaqueReg = File.open("#{ENV["AUTOPROJ_CURRENT_ROOT"]}/opaques", "w+")
            opaqueReg << "<typelib>"
            opaques.each do |opaque|
                typedef = "<opaque name=\"#{opaque.gsub('<', '&lt;').gsub('>', '&gt;')}\" size=\"#{0}\" />"
                opaqueReg << typedef
            end
            opaqueReg << "</typelib>"
            opaqueReg.flush
            
            # FIXME: see above
            tmpReg = File.open("#{ENV["AUTOPROJ_CURRENT_ROOT"]}/registry", "w+")
            
            include_dirs = options[:include_paths]
            include_path = include_dirs.map { |d| "-I #{d}" }.join(" ")

            header_files = options[:required_files].join(" ") 
            
            # calling the the installed töb-import-tool, found in the
            # install-folder via PATH.
            finalCmd = "typelib-clang-tlb-importer -opaquePath=\"#{opaqueReg.path}\" -tlbSavePath=\"#{tmpReg.path}\" #{header_files} -- #{include_path} -x c++"

            # call extractor tool
            retval = system(finalCmd)
            if retval != true
                raise RuntimeError, "typelib-clang-tlb-importer failed!"
            end
            
            result = Registry.from_xml(tmpReg.read())
            
            registry.merge(result)
        end
        TYPE_HANDLERS['c'] = method(:load_from_clang)
    end

    class ClangLoader
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

    end
end
