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
            
            puts("clang.rb: Loading options are #{options}")
            include_dirs = options[:include_paths]
            include_path = include_dirs.map { |d| "-I #{d}" }.join(" ")

            header_files = options[:required_files].join(" ") 
            
            # FIXME: this should be the installed tool
            finalCmd = "#{ENV["AUTOPROJ_CURRENT_ROOT"]}/tools/typelib/build/tools/typelib-clang-tlb-importer/typelib-clang-tlb-importer -opaquePath=\"#{opaqueReg.path}\" -tlbSavePath=\"#{tmpReg.path}\" #{header_files} -- #{include_path} -x c++"
            
            puts("clang.rb: Cmd is : #{finalCmd}")
            
            #call extractor
            retVal = system(finalCmd)
            
            result = Registry.from_xml(tmpReg.read())
            
            registry.merge(result)
        end

        TYPE_HANDLERS['clang'] = method(:load_from_clang)
    end
end
