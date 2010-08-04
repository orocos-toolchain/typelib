require 'typelib'
require 'set'
require 'nokogiri'
require 'tempfile'
module Typelib
    # A converted from the output of GCC-XML to the Typelib's own XML format
    class GCCXMLLoader
        # The set of types that should be considered as opaques by the engine
        attr_accessor :opaques

        # The resulting XML
        attr_reader :result

        # A mapping from the type ID to the corresponding typelib type name
        #
        # If the type name is nil, it means that the type should not be
        # represented in typelib
        attr_reader :id_to_name
        # A mapping from the type ID to the corresponding XML node
        attr_reader :id_to_node
        # A mapping from type name to type name. Whenever a type name is
        # emitted, it is checked against that map to be translated into what we
        # actually want in the final XML
        attr_reader :type_aliases

        def node_from_id(id)
            id_to_node[id]
        end

        def initialize
            @opaques = Set.new
            @id_to_name = Hash.new
            @type_aliases = Hash.new
        end

        def xml_quote(name)
            name.
                gsub('<', '&lt;').
                gsub('>', '&gt;')
        end
    
        def emit_type_name(name)
            if al = self.type_aliases[name]
                emit_type_name(al)
            else
                xml_quote(name)
            end
        end

        def parse_template(name)
            level = 0
            type_name = nil
            arguments = []
            start = nil

            name.size.times do |i|
                char = name[i, 1]
                if char == "<"
                    if level == 0
                        type_name = name[0, i]
                        start = i + 1
                    end
                    level += 1

                elsif char == ","
                    if level == 1
                        arguments << name[start, i - start]
                        start = i + 1
                    end

                elsif char == ">"
                    level -= 1
                    if level == 0
                        arguments << name[start, i - start]
                    end
                end
            end

            type_name ||= name
            return type_name.strip, arguments.map(&:strip)
        end

        def typelib_to_cxx(name)
            name = name.gsub('/', '::')
            if name[0, 2] == "::"
                name[2..-1]
            else
                name
            end
        end

        def cxx_to_typelib(name, absolute = true)
            type_name, template_arguments = parse_template(name)

            type_name = type_name.gsub('::', '/')
            if absolute && type_name[0, 1] != "/"
                type_name = "/#{type_name}"
            end
            template_arguments.map! { |n| cxx_to_typelib(n, absolute) }

            if !template_arguments.empty?
                # std::vector has only one parameter for us ...
                if type_name == "/std/vector"
                    "#{type_name}<#{template_arguments[0]}>"
                elsif type_name == "/std/basic_string"
                    "/std/string"
                else
                    "#{type_name}<#{template_arguments.join(",")}>"
                end
            else
                type_name
            end
        end

        def resolve_namespace(xml, id)
            ns = (xml / "Namespace[id=\"#{id}\"]").first
            name = ns['name']

            if name == "::"
                return []
            end

            (resolve_namespace(xml, ns['context']) << ns['name'])
        end

        def resolve_context(xml, id)
            if id_to_name.has_key?(id)
                return id_to_name[id]
            end

            definition = node_from_id(id)
            if definition.name == "Namespace"
                result = "/" + resolve_namespace(xml, id).join("/")
                id_to_name[id] = result
                result
            else
                resolve_type_id(xml, id)
            end
        end
        
        def resolve_type_id(xml, id)
            if id_to_name.has_key?(id)
                id_to_name[id]
            else
                typedef = node_from_id(id)
                resolve_type_definition(xml, typedef)
            end
        end

        def ignore(id, msg = nil)
            if msg
                STDERR.puts "WARN: #{msg}"
            end
            id_to_name[id] = nil
        end

        # Returns if +name+ has been declared as an opaque
        def opaque?(name)
            STDERR.puts "checking if #{name} is an opaque"
            STDERR.puts "  #{opaques.to_a.join(", ")}"
            opaques.include?(name)
        end

        def resolve_type_definition(xml, xmlnode)
            kind = xmlnode.name
            id   = xmlnode['id']
            if id_to_name.has_key?(id)
                return id_to_name[id]
            end

            type_def = []

            if kind == "PointerType"
                if pointed_to_type = resolve_type_id(xml, xmlnode['type'])
                    return (id_to_name[id] = "#{pointed_to_type}*")
                else return ignore(id)
                end
            elsif kind == "ArrayType"
                ignore(id)
                if pointed_to_type = resolve_type_id(xml, xmlnode['type'])
                    return (id_to_name[id] = "#{pointed_to_type}[#{xmlnode['max']}]")
                else return ignore(id)
                end
            end

            name = cxx_to_typelib(xmlnode['demangled'] || xmlnode['name'])

            if name =~ /\/__\w+$/
                # This is defined as private STL/Compiler implementation
                # structures. Just ignore it
                return ignore(id)
            end

            id_to_name[id] = name
            if opaque?(name)
                # Nothing to do ...
                return name
            end

            if kind == "Struct" || kind == "Class"
                type_name, template_args = parse_template(name)
                if type_name == "/std/string"
                    # This is internally known to typelib
                elsif Typelib::Registry.available_containers.include?(type_name)
                    # This is known to Typelib as a container
                    type_def << "<container name=\"#{emit_type_name(name)}\" of=\"#{emit_type_name(template_args[0])}\" kind=\"#{emit_type_name(type_name)}\"/>"
                else
                    # Make sure that we can digest it. Forbidden are: inheritance,
                    # private members
                    #
                    # TODO: add inheritance support
                    if xmlnode['bases'] && !xmlnode['bases'].empty?
                        return ignore(id, "ignoring #{name} as it has parent classes (#{xmlnode['bases']})")
                    end

                    fields = xmlnode['members'].split(" ").
                        map { |member_id| node_from_id(member_id) }.
                        find_all { |member_node| member_node.name == "Field" }

                    if fields.empty?
                        return ignore(id, "ignoring the empty struct/class #{name}")
                    end

                    type_def << "<compound name=\"#{emit_type_name(name)}\" size=\"#{Integer(xmlnode['size']) / 8}\">"
                    fields.each do |field|
                        if field['access'] == 'private'
                            return ignore(id, "ignoring #{name} since its field #{field['name']} is private")
                        elsif field_type_name = resolve_type_id(xml, field['type'])
                            type_def << "  <field name=\"#{field['name']}\" type=\"#{emit_type_name(field_type_name)}\" offset=\"#{Integer(field['offset']) / 8}\" />"
                        else
                            return ignore(id, "ignoring #{name} since its field #{field['name']} has a type that can't be represented")
                        end
                    end
                    type_def << "</compound>"
                end
            elsif kind == "FundamentalType"
            elsif kind == "Typedef"
                if !(pointed_to_type = resolve_type_id(xml, xmlnode['type']))
                    return ignore(id, "cannot create the #{name} typedef, as it points to a type that can't be represented")
                end

                namespace       = resolve_context(xml, xmlnode['context'])
                full_name =
                    if namespace != "/"
                        "#{namespace}#{name}"
                    else name
                    end
                type_def << "<alias name=\"#{emit_type_name(full_name)}\" source=\"#{emit_type_name(pointed_to_type)}\" />"

                # And always resolve the typedef as the type it is pointing to
                name = id_to_name[id] = pointed_to_type

            elsif kind == "Enumeration"
                namespace       = resolve_context(xml, xmlnode['context'])
                full_name =
                    if namespace != "/"
                        "#{namespace}#{name}"
                    else name
                    end

                name = id_to_name[id] = full_name

                type_def << "<enum name=\"#{emit_type_name(full_name)}\">"
                (xmlnode / "EnumValue").each do |enum_value|
                    type_def << "  <value symbol=\"#{enum_value["name"]}\" value=\"#{enum_value["init"]}\"/>"
                end
                type_def << "</enum>"
            else
                return ignore(id, "ignoring #{name} as it is of the unsupported GCCXML type #{kind}")
            end

            result.concat(type_def)
            name
        end

        # This method looks for the real name of opaques
        #
        # The issue with opaques is that they might be either typedefs or
        # templates with default arguments. In both cases, we need to find out
        # their real name 
        #
        # For the typedefs, it it easy
        #
        # For the templates with default arguments, we have to generate a C++
        # file that we feed back to GCCXML :(
        def resolve_opaques(xml)
            # First do typedefs. Search for the typedefs that are named like our
            # type, if we find one, alias it
            opaques.each do |name|
                name = typelib_to_cxx(name)
                (xml / "Typedef[name=\"#{name}\"]").each do |typedef|
                    full_name = cxx_to_typelib(node_from_id(typedef["type"].to_s)["name"])
                    opaques << full_name
                    STDERR.puts "aliasing #{full_name} to #{name}"
                    self.type_aliases[full_name] = name
                end
            end
        end

        def load(path, xml)
            @result = Array.new

            @id_to_node = Hash.new
            # Enumerate all children of the root node in an id-to-node map, to
            # speed up lookup during the type resolution process
            root = (xml / "GCC_XML").first
            root.children.each do |child_node|
                id_to_node[child_node["id"].to_s] = child_node
            end

            if !opaques.empty?
                # We MUST emit the opaque definitions before calling
                # resolve_opaques as resolve_opaques will add the resolved
                # opaque names to +opaques+
                opaques.each do |type_name|
                    result << "<opaque name=\"#{type_name}\" size=\"0\"/>"
                end
                resolve_opaques(xml)
            end

            # Extract the "root" type definitions, i.e. the type definitions
            # that are actually included in the files we are loading
            files = (xml / "File").find_all do |file|
                file_path = file["name"].to_s
                if file_path != "built-ins"
                    (file_path =~ /^#{Regexp.quote(path)}/) ||
                        (file_path[0, 1] != "/")
                end
            end
            file_ids = files.map { |f| f["id"] }

            all_structs  = []
            all_typedefs = []
            file_ids.each do |id|
                all_structs.concat((xml / "Struct[file=\"#{id}\"]").to_a)
                all_structs.concat((xml / "Class[file=\"#{id}\"]").to_a)
                all_typedefs.concat((xml / "Typedef[file=\"#{id}\"]").to_a)
            end

            # Resolve structs and classes
            puts "#{all_structs.size} structure definitions found"
            all_structs.each do |node|
                resolve_type_definition(xml, node)
            end

            # Look at typedefs
            puts "#{all_typedefs.size} typedefs found"
            all_typedefs.each do |node|
                resolve_type_definition(xml, node)
            end

            "<?xml version=\"1.0\"?>\n<typelib>\n" + result.join("\n") + "\n</typelib>"
        end

        # Runs gccxml on the provided file and with the given options, and
        # return the Nokogiri::XML object representing the result
        #
        # Raises RuntimeError if gccxml failed to run
        def self.gccxml(file, options)
            cmdline = ["gccxml"]
            if raw = options[:rawflags]
                cmdline.concat(raw)
            end

            if defs = options[:define]
                defs.each do |str|
                    cmdline << "-D#{str}"
                end
            end

            if inc = options[:include]
                inc.each do |str|
                    cmdline << "-I#{str}"
                end
            end

            cmdline << file

            tlb = nil
            Tempfile.open('typelib_gccxml') do |io|
                cmdline << "-fxml=#{io.path}"
                if !system(*cmdline)
                    raise "gccxml returned an error while parsing #{file}"
                end

                return Nokogiri::XML(io.read)
            end
        end
    end

    class Registry
        # Imports the given C++ file into the registry using GCC-XML
        def self.load_from_gccxml(registry, file, kind, options)
            base_dir = (options[:base_dir] || file)
            base_dir = File.expand_path(base_dir)


            xml = GCCXMLLoader.gccxml(file, options)
            converter = GCCXMLLoader.new
            if opaques = options[:opaques]
                converter.opaques |= opaques.to_set
            end
            converter.load(base_dir, xml)
        end

        TYPE_HANDLERS['c'] = method(:load_from_gccxml)
    end
end

