require 'set'
require 'tempfile'

module Typelib

    class Registry
        # Imports the given C++ file into the registry using CLANG
        def self.load_from_clang(registry, file, kind, options)
            
            #this gives us an array of opaques
            opaques = options[:opaques]
            
            #create a registry from it
#            opaqueReg = Tempfile.open("tmp_opaques")
            opaqueReg = File.open("/home/scotch/item/opaques", "w+")
            opaqueReg << "<typelib>"
            opaques.each do |opaque|
                typedef = "<opaque name=\"#{opaque.gsub('<', '&lt;').gsub('>', '&gt;')}\" size=\"#{0}\" />"
                opaqueReg << typedef
            end
            opaqueReg << "</typelib>"
            opaqueReg.flush
            
            tmpReg = File.open("/home/scotch/item/registry", "w+")
            
            puts("test ?")
            
            puts("Loading options are #{options}")
            include_dirs = options[:include_paths]
            include_path = include_dirs.map { |d| "-I #{d}" }.join(" ")
            puts("Include Path #{include_path}")

            header_files = options[:required_files].join(" ") 
            
            finalCmd = "/home/scotch/item/tools/typelib/build/tools/tlbBuilder/extractor -opaquePath=\"#{opaqueReg.path}\" -tlbSavePath=\"#{tmpReg.path}\" #{header_files} -- #{include_path} -x c++ -isystem /usr/lib/clang/3.4/include"
            
            puts("Cmd is : #{finalCmd}")
            
            #call extractor
            retVal = `#{finalCmd}`
            
            result = Registry.new()
            
            result.from_xml(tmpReg.load())
            
            registry.merge(result)
        end

        TYPE_HANDLERS['clang'] = method(:load_from_clang)
    end
end