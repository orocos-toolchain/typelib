require 'set'
require 'tempfile'

module Typelib

    class Registry
        # Imports the given C++ file into the registry using CLANG
        def self.load_from_clang(registry, file, kind, options)

            # FIXME: the second argument "file" contains the preprocessed
            # output created in an earlier stage. it is not used here, but the
            # list of actual header-files in the "options" hash. not having to
            # pass 100k likes of text might improve performance?
            
            # this gives us an array of opaques
            opaques = options[:opaques]
            
            # create a registry from the given opaques, so that the importer can
            # load the informations to use it sduring processing.
            #
            # FIXME: reuse some code for creating a "opaque.tlb"
            opaqueReg = Tempfile.open("tlb-import-opaques-")
            opaqueReg << "<typelib>"
            opaques.each do |opaque|
                opaqueReg << "<opaque name=\"#{opaque.gsub('<', '&lt;').gsub('>', '&gt;')}\" size=\"0\" />"
            end
            opaqueReg << "</typelib>"
            opaqueReg.flush
            
            # a file where the importer can store its results for merging into
            # our database
            tmpReg = Tempfile.open("tlb-save-registry-")
            
            include_dirs = options[:include_paths]
            include_path = include_dirs.map { |d| "-I#{d}" }.join(" ")

            # which files actually to operate on
            #
            # FIXME: calling the importer on this list of files can be
            # parallelized?
            header_files = options[:required_files].join(" ") 
            
            # calling the the installed tlb-import-tool, hopefully found in the
            # install-folder via PATH.
            #
            # NOTE: the added 'isystem' flag to point clang (3.4) to its own correct
            # header path. this is needed on ubuntu 14.04 (debian jessie works
            # without but does not seem to break). to see whats going wrong
            # compare the output of "echo '#include <stdarg.h>'|clang -xc -v -"
            # and read here:
            #   https://github.com/Valloric/YouCompleteMe/issues/303#issuecomment-17656962
            finalCmd = "typelib-clang-tlb-importer -opaquePath=\"#{opaqueReg.path}\" -tlbSavePath=\"#{tmpReg.path}\" #{header_files} -- -isystem/usr/bin/../lib/clang/3.4/include #{include_path} -x c++"

            # and finally call importer-tool
            retval = system(finalCmd)
            if retval != true
                raise RuntimeError, "typelib-clang-tlb-importer failed!"
            end
            
            # read the registry created by the tool back into this ruby process
            result = Registry.from_xml(tmpReg.read())
            # and merge it into our registry
            registry.merge(result)
        end
        TYPE_HANDLERS['c'] = method(:load_from_clang)
    end

    # some functions copied fs-is rom the original implementation of
    # "gccxml.rb" for reusing. they do string-manipulation and convenience
    # stuff on cxx-names.
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
