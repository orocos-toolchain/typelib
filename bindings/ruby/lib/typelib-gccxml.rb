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
        # A mapping from a node ID to the name of the file that defines it
        attr_reader :id_to_file

        attr_reader :name_to_nodes
        attr_reader :demangled_to_node

        # A mapping from the type ID to the type names
        #
        # Unlike id_to_name, the stored value is not dependent of the fact that
        # the type has to be exported by typelib.
        attr_reader :type_names
        # A mapping from type name to type name. Whenever a type name is
        # emitted, it is checked against that map to be translated into what we
        # actually want in the final XML
        attr_reader :type_aliases

        def node_from_id(id)
            id_to_node[id]
        end

        def initialize
            @opaques      = Set.new
            @id_to_name   = Hash.new
            @id_to_node   = Hash.new
            @type_names   = Hash.new
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

        # Parses the Typelib or C++ type name +name+ and returns basename,
        # template_arguments
        def self.parse_template(name)
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
            name = name.gsub('/', '::').
                gsub('<::', '<').
                gsub(',', ', ')
            if name[0, 2] == "::"
                name[2..-1]
            else
                name
            end
        end

        def cxx_to_typelib(name, absolute = true)
            type_name, template_arguments = GCCXMLLoader.parse_template(name)

            type_name = type_name.gsub('::', '/')
            if absolute && type_name[0, 1] != "/" && type_name !~ /^\d+$/
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

        # Given a full Typelib type name, returns a [name, id] pair where +name+
        # is the type's basename and +id+ the context ID (i.e. the GCCXML
        # namespace ID)
        def resolve_namespace_of(xml, name)
            context = nil
            while name =~ /\/(\w+)\/(.*)/
                ns   = $1
                name = "/#{$2}"
                candidates = name_to_nodes[ns].find_all { |n| n.name == "Namespace" }
                if !context
                    context = candidates.to_a.first
                else
                    context = candidates.find { |node| node['context'].to_s == context }
                end
                if !context
                    break
                else context = context["id"].to_s
                end
            end
            return name, context
        end

        def resolve_namespace(xml, id)
            ns = id_to_node[id]
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

        def warn(msg)
            STDERR.puts "WARN: #{msg}"
        end

        def file_context(xmlnode)
            if (file = xmlnode["file"]) && (line = xmlnode["line"])
                "#{id_to_node[file]["name"]}:#{line}"
            end
        end

        def ignore(xmlnode, msg = nil)
            if msg
                warn("#{file_context(xmlnode)}: #{msg}")
            end
            id_to_name[xmlnode["id"]] = nil
        end

        # Returns if +name+ has been declared as an opaque
        def opaque?(name)
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
                else return ignore(xmlnode)
                end
            elsif kind == "ArrayType"
                if pointed_to_type = resolve_type_id(xml, xmlnode['type'])
                    value = xmlnode["max"]
                    if value =~ /^(\d+)u/
                        size = Integer($1) + 1
                    else
                        raise "expected NUMBERu for the 'max' attribute of an array definition, but got #{value}"
                    end
                    return (id_to_name[id] = "#{pointed_to_type}[#{size}]")
                else return ignore(xmlnode)
                end
            end

            name = cxx_to_typelib(xmlnode['demangled'] || xmlnode['name'])
            if name =~ /gccxml_workaround/
                return
            end

            if name =~ /\/__\w+$/
                # This is defined as private STL/Compiler implementation
                # structures. Just ignore it
                return ignore(xmlnode)
            end

            type_names[id] = name
            id_to_name[id] = name
            if opaque?(name)
                # Nothing to do ...
                return name
            end

            if kind == "Struct" || kind == "Class"
                type_name, template_args = GCCXMLLoader.parse_template(name)
                if type_name == "/std/string"
                    # This is internally known to typelib
                elsif Typelib::Registry.available_containers.include?(type_name)
                    # This is known to Typelib as a container
                    contained_type = typelib_to_cxx(template_args[0])
                    contained_node = demangled_to_node[contained_type]
                    if !contained_node
                        contained_node = name_to_nodes[contained_type].first
                    end
                    if !contained_node
                        contained_basename, contained_context = resolve_namespace_of(xml, template_args[0])
                        contained_node = name_to_nodes[typelib_to_cxx(contained_basename)].
                            find { |node| node['context'].to_s == contained_context }
                    end
                    if !contained_node
                        raise "Internal error: cannot find definition for #{contained_type}"
                    end

                    if resolve_type_id(xml, contained_node["id"])
                        type_def << "<container name=\"#{emit_type_name(name)}\" of=\"#{emit_type_name(template_args[0])}\" kind=\"#{emit_type_name(type_name)}\" source_id=\"#{id_to_file[id]}\"/>"
                    else return ignore("ignoring #{name} as its element type #{contained_type} is ignored as well")
                    end
                else
                    # Make sure that we can digest it. Forbidden are: inheritance,
                    # private members
                    #
                    # TODO: add inheritance support
                    if xmlnode['bases'] && !xmlnode['bases'].empty?
                        return ignore(xmlnode, "ignoring #{name} as it has parent classes")
                    end

                    fields = (xmlnode['members'] || "").split(" ").
                        map { |member_id| node_from_id(member_id) }.
                        find_all { |member_node| member_node.name == "Field" }

                    if fields.empty?
                        return ignore(xmlnode, "ignoring the empty struct/class #{name}")
                    end

                    type_def << "<compound name=\"#{emit_type_name(name)}\" size=\"#{Integer(xmlnode['size']) / 8}\" source_id=\"#{id_to_file[id]}\">"
                    fields.each do |field|
                        if field['access'] == 'private'
                            return ignore(xmlnode, "ignoring #{name} since its field #{field['name']} is private")
                        elsif field_type_name = resolve_type_id(xml, field['type'])
                            type_def << "  <field name=\"#{field['name']}\" type=\"#{emit_type_name(field_type_name)}\" offset=\"#{Integer(field['offset']) / 8}\" />"
                        else
                            return ignore(xmlnode, "ignoring #{name} since its field #{field['name']} is of the ignored type #{type_names[field['type']]}")
                        end
                    end
                    type_def << "</compound>"
                end
            elsif kind == "FundamentalType"
            elsif kind == "Typedef"
                if !(namespace = resolve_context(xml, xmlnode['context']))
                    return ignore(xmlnode, "ignoring typedef #{name} as it is part of #{type_names[xmlnode['context']]} which is ignored")
                end

                full_name =
                    if namespace != "/"
                        "#{namespace}#{name}"
                    else name
                    end

                type_names[id] = full_name
                id_to_name[id] = full_name
                if opaque?(full_name)
                    # Nothing to do ...
                    return full_name
                end

                if !(pointed_to_type = resolve_type_id(xml, xmlnode['type']))
                    return ignore(xmlnode, "cannot create the #{full_name} typedef, as it points to #{type_names[xmlnode['type']]} which is ignored")
                end

                type_def << "<alias name=\"#{emit_type_name(full_name)}\" source=\"#{emit_type_name(pointed_to_type)}\" source_id=\"#{id_to_file[id]}\"/>"

                # And always resolve the typedef as the type it is pointing to
                name = id_to_name[id] = pointed_to_type

            elsif kind == "Enumeration"
                if !(namespace = resolve_context(xml, xmlnode['context']))
                    return ignore(xmlnode, "ignoring typedef #{name} as it is part of #{type_names[xmlnode['context']]} which is ignored")
                end

                full_name =
                    if namespace != "/"
                        "#{namespace}#{name}"
                    else name
                    end

                name = id_to_name[id] = full_name

                type_def << "<enum name=\"#{emit_type_name(full_name)}\" source_id=\"#{id_to_file[id]}\">"
                (xmlnode / "EnumValue").each do |enum_value|
                    type_def << "  <value symbol=\"#{enum_value["name"]}\" value=\"#{enum_value["init"]}\"/>"
                end
                type_def << "</enum>"
            else
                return ignore(xmlnode, "ignoring #{name} as it is of the unsupported GCCXML type #{kind}")
            end

            result.concat(type_def)
            type_names[id] = name
            name
        end

        # This method looks for the real name of opaques
        #
        # The issue with opaques is that they might be either typedefs or
        # templates with default arguments. In both cases, we need to find out
        # their real name 
        #
        # For the typedefs, it it easy
        def resolve_opaques(xml)
            # First do typedefs. Search for the typedefs that are named like our
            # type, if we find one, alias it
            opaques.dup.each do |opaque_name|
                name, context = resolve_namespace_of(xml, opaque_name)
                name = typelib_to_cxx(name)
                name_to_nodes[name].find_all { |n| n.name == "Typedef" }.each do |typedef|
                    next if context && typedef["context"].to_s != context
                    type_node = node_from_id(typedef["type"].to_s)
                    namespace = resolve_context(xml, type_node['context'])
                    base = cxx_to_typelib(type_node["name"])
                    full_name = "#{namespace}#{base}"

                    opaques << full_name
                    self.type_aliases[full_name] = opaque_name
                end
            end
        end

        def load(required_files, xml)
            @result = Array.new

            @id_to_node = Hash.new
            @id_to_file = Hash.new
            @name_to_nodes = Hash.new { |h, k| h[k] = Array.new }
            @demangled_to_node = Hash.new

            # Enumerate all children of the root node in an id-to-node map, to
            # speed up lookup during the type resolution process
            root = (xml / "GCC_XML").first
            types_per_file = Hash.new { |h, k| h[k] = Array.new }
            typedefs_per_file = Hash.new { |h, k| h[k] = Array.new }
            root_file_ids = Array.new
            root.children.each do |child_node|
                id_to_node[child_node["id"].to_s] = child_node
                if (child_node_name = child_node['name'])
                    name_to_nodes[child_node_name] << child_node
                end
                if (child_node_name = child_node['demangled'])
                    demangled_to_node[child_node_name] = child_node
                end

                if child_node.name == "File"
                    if required_files.include?(child_node['name'])
                        root_file_ids << child_node['id']
                    end
                elsif %w{Struct Class Enumeration}.include?(child_node.name) && child_node['incomplete'] != '1'
                    types_per_file[child_node['file']] << child_node
                elsif child_node.name == "Typedef"
                    typedefs_per_file[child_node['file']] << child_node
                end
            end

            all_types = Array.new
            all_typedefs = Array.new
            root_file_ids.each do |file_id|
                all_types.concat(types_per_file[file_id])
                all_typedefs.concat(typedefs_per_file[file_id])
            end
            all_types.each do |type_node|
                id_to_file[type_node["id"].to_s] = id_to_node[type_node['file']]['name']
            end
            all_typedefs.each do |type_node|
                id_to_file[type_node["id"].to_s] = id_to_node[type_node['file']]['name']
            end

            if !opaques.empty?
                # We MUST emit the opaque definitions before calling
                # resolve_opaques as resolve_opaques will add the resolved
                # opaque names to +opaques+
                opaques.each do |type_name|
                    result << "<opaque name=\"#{xml_quote(type_name)}\" size=\"0\"/>"
                end
                resolve_opaques(xml)
            end


            # Resolve structs and classes
            all_types.each do |node|
                resolve_type_definition(xml, node)
            end

            # Look at typedefs
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
                    raise ArgumentError, "gccxml returned an error while parsing #{file}"
                end

                return Nokogiri::XML(io.read)
            end
        end
    end

    class Registry
        # Imports the given C++ file into the registry using GCC-XML
        def self.load_from_gccxml(registry, file, kind, options)
            required_files = (options[:required_files] || [file]).
                map { |f| File.expand_path(f) }

            opaques = Set.new
            registry.each do |type|
                if type.opaque?
                    opaques << type.name
                end
            end

            # Add the standard C++ types (such as /std/string)
            Registry.add_standard_cxx_types(registry)

            xml = GCCXMLLoader.gccxml(file, options)
            converter = GCCXMLLoader.new
            converter.opaques = opaques
            if opaques = options[:opaques]
                converter.opaques |= opaques.to_set
            end
            tlb = converter.load(required_files, xml)

            Tempfile.open('gccxml.tlb') do |io|
                io.write tlb
                io.flush

                registry.import(io.path, 'tlb', :merge => false)
            end
        end

        # Returns true if Registry#import will use GCCXML to load the given
        # file
        def self.uses_gccxml?(path, kind = 'auto')
            (handler_for(path, kind) == method(:load_from_gccxml))
        end

        TYPE_HANDLERS['c'] = method(:load_from_gccxml)
    end
end

