require 'set'
require 'tempfile'
require 'open3'

module Typelib
    module CLangLoader
        # Imports the given C++ file into the registry using CLANG
        def self.load(registry, file, kind, options)
            # FIXME: the second argument "file" contains the preprocessed
            # output created in an earlier stage. it is not used here, but the
            # list of actual header-files in the "options" hash. not having to
            # pass 100k likes of text might improve performance?
            
            # this gives us an array of opaques
            opaque_registry = Registry.new
            options[:opaques].uniq.each do |opaque_t|
                opaque_registry.create_opaque(opaque_t, 0)
            end
            
            include_dirs = options[:include_paths]
            include_path = include_dirs.map { |d| "-I#{d}" }

            # which files actually to operate on
            #
            # FIXME: calling the importer on this list of files can be
            # parallelized?
            header_files = options[:required_files]
            
            # create a registry from the given opaques, so that the importer can
            # load the informations to use it sduring processing.
            #
            # FIXME: reuse some code for creating a "opaque.tlb"
            Tempfile.open('tlb-clang-opaques-') do |opaque_registry_io|
                opaque_registry_io.write opaque_registry.to_xml
                opaque_registry_io.flush
            
                Tempfile.open('tlb-clang-output-') do |clang_output_io|
                    # calling the the installed tlb-import-tool, hopefully found in the
                    # install-folder via PATH.
                    #
                    # NOTE: the added 'isystem' flag to point clang (3.4) to its own correct
                    # header path. this is needed on ubuntu 14.04 (debian jessie works
                    # without but does not seem to break). to see whats going wrong
                    # compare the output of "echo '#include <stdarg.h>'|clang -xc -v -"
                    # and read here:
                    #   https://github.com/Valloric/YouCompleteMe/issues/303#issuecomment-17656962
                    command_line =
                        ['typelib-clang-tlb-importer',
                         "-opaquePath=#{opaque_registry_io.path}",
                         "-tlbSavePath=#{clang_output_io.path}",
                         *header_files,
                         '--',
                         '-isystem/usr/bin/../lib/clang/3.4/include',
                         *include_path,
                         "-x", "c++"]

                    _, stderr, status = Open3.capture3(*command_line)

                    # and finally call importer-tool
                    if !status.success?
                        STDERR.puts stderr
                        raise RuntimeError, "typelib-clang-tlb-importer failed"
                    end

                    # read the registry created by the tool back into this ruby process
                    result = Registry.from_xml(clang_output_io.read)
                    # and merge it into our registry
                    registry.merge(result)
                end
            end
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

                # just to be sure to have exactly the same compiler as the
                # importer we enforce "clang-3.4"
                result = IO.popen(["clang-3.4", "-E", *includes, *defines, io.path]) do |clang_io|
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
