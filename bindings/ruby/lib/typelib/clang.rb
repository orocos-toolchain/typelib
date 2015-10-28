require 'set'
require 'tempfile'
require 'rbconfig'


module Typelib
    module CLangLoader
	def self.macos?
            RbConfig::CONFIG["host_os"] =~ %r!([Dd]arwin)!
	end

        # Imports the given C++ file into the registry using CLANG
        def self.load(registry, file, kind, opaques: Set.new, include_paths: Array.new, **options)
            #Checking if the clang importer is installed and can be found on the system
            if !system("which typelib-clang-tlb-importer > /dev/null 2>&1")
                raise RuntimeError, "typelib-clang-tlb-importer is not installed in PATH"
            end

            # FIXME: the second argument "file" contains the preprocessed
            # output created in an earlier stage. it is not used here, but the
            # list of actual header-files in the "options" hash. not having to
            # pass 100k likes of text might improve performance?
            
            # this gives us an array of opaques
            opaque_registry = Registry.new
            opaques.to_a.uniq.each do |opaque_t|
                opaque_registry.create_opaque(opaque_t, 0)
            end
            
            include_dirs = options[:include_paths] || []
            include_path = include_dirs.map { |d| "-I#{d}" }

            # which files actually to operate on
            #
            # FIXME: calling the importer on this list of files can be
            # parallelized?
            header_files = options[:required_files] || [file]
            
            # create a registry from the given opaques, so that the importer can
            # load the informations to use it sduring processing.
            #
            # FIXME: reuse some code for creating a "opaque.tlb"
            Tempfile.open('tlb-clang-opaques-') do |opaque_registry_io|
                opaque_registry_io.write opaque_registry.to_xml
                opaque_registry_io.flush

                Tempfile.open('tlb-clang-output-') do |clang_output_io|
                    # NOTE: the added 'isystem' flag to point clang (3.4) to its own correct
                    # header path. this is needed on ubuntu 14.04 (debian jessie works
                    # without but does not seem to break). to see whats going wrong
                    # compare the output of "echo '#include <stdarg.h>'|clang -xc -v -"
                    # and read here:
                    #   https://github.com/Valloric/YouCompleteMe/issues/303#issuecomment-17656962
		    command_line =
			['typelib-clang-tlb-importer',
			 "-silent",
			 "-opaquePath=#{opaque_registry_io.path}",
			 "-tlbSavePath=#{clang_output_io.path}",
			 *header_files,
			 '--',
			 *include_path,
			 "-x", "c++"]

		    if macos?
                        command_line <<  '-resource-dir=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/6.0'
                        command_line <<  '-stdlib=libc++'
		    else
                        command_line << '-isystem/usr/bin/../lib/clang/3.4/include'
		    end

                    # and finally call importer-tool
                    if !system(*command_line)
                        raise RuntimeError, "typelib-clang-tlb-importer failed!"
                    end

                    # read the registry created by the tool back into this ruby process
                    result = Registry.from_xml(clang_output_io.read)
                    # and merge it into our registry
                    registry.merge(result)
                end
            end
        end

        def self.clang_binary_name
            if system("which clang-3.4 > /dev/null 2>&1")
                return "clang-3.4"
            elsif system("which clang > /dev/null 2>&1")
                IO.popen(['clang', '--version']) do |clang_io|
                    clang_version = clang_io.read
                    if clang_version.include?('clang version 3.4')
                        return "clang"
                    end
                end
            end
            raise RuntimeError, "Couldn't find clang 3.4 compiler binary!"
        end

        # NOTE: preprocessing of a group of header-files by the actual compiler
        # is needed by orogen to get the resolved top-level include-files. once
        # this is done by the clang-based importer the preprocessing can be
        # removed.
        def self.preprocess(files, kind, options)
            includes = options[:include].map { |v| "-I#{v}" }
            defines  = options[:define].map { |v| "-D#{v}" }

            # create a tempfile where all files used in the typekit are just
            # "dummy-included" to create one big compile unit. this stems from
            # the gccxml era and was supposed to trick the compiler into beeing
            # faster... but doing that prevents parallel batch-analyzing in the
            # case of many headers on a multi-core system.
            #
            # note that we force the ending of the tempfile to be ".hpp" so that
            # the clang-based tool detects the content as c++ source-code
            Tempfile.open(['clang_preprocess_', '.hpp']) do |io|
                files.each do |path|
                    io.puts "#include <#{path}>"
                end
                io.flush

                additional_opts = []

                if macos?
                    additional_opts = ['-resource-dir=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/../lib/clang/6.0',  '-stdlib=libc++']
                end

                result = IO.popen([clang_binary_name, *additional_opts ,"-E", *includes, *defines, io.path]) do |clang_io|
                    clang_io.read
                end

                if !$?.success?
                    raise ArgumentError, "resolve_toplevel_include_mapping(): Failed to preprocess one of #{files.join(", ")}"
                end
                result
            end
        end
    end
end
