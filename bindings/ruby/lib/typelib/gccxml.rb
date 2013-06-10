require 'typelib'
require 'set'
require 'nokogiri'
require 'tempfile'
module Typelib
    # A converted from the output of GCC-XML to the Typelib's own XML format
    class GCCXMLLoader
        # The set of types that should be considered as opaques by the engine
        attr_accessor :opaques

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
        attr_reader :ignore_message

        # The registry that is being filled by parsing GCCXML output
        attr_reader :registry

        # A mapping from the type ID to the type names
        #
        # Unlike id_to_name, the stored value is not dependent of the fact that
        # the type has to be exported by typelib.
        attr_reader :type_names

        def node_from_id(id)
            id_to_node[id]
        end

        def initialize
            @opaques      = Set.new
            @id_to_name   = Hash.new
            @id_to_node   = Hash.new
            @type_names   = Hash.new
            @ignore_message = Hash.new
        end

        def xml_quote(name)
            name.
                gsub('<', '&lt;').
                gsub('>', '&gt;')
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

        # Parses the Typelib or C++ type name +name+ and returns basename,
        # template_arguments
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

        def typelib_to_cxx(name)
            tokens = GCCXMLLoader.template_tokenizer(name)
            tokens.each do |tk|
                case tk
                when /[<,>]/, /^\d/
                else
                    tk.gsub!('/', '::')
                end
            end

            name = tokens.join("").gsub('<::', '<').
                gsub(',::', ',').
                gsub(',([^\s])', ', \1')
            if name[0, 2] == "::"
                name[2..-1]
            else
                name
            end
        end

        def self.cxx_to_typelib(name, absolute = true)
            name = name.gsub('::', '/')
            name = name.gsub('> >', '>>')

            tokens = GCCXMLLoader.template_tokenizer(name)
            tokenized_cxx_to_typelib(tokens)
        end

        def self.tokenized_cxx_to_typelib(tokens, absolute = true)
            result = []
            while !tokens.empty?
                tk = tokens.shift
                if tk =~ /^\/?std\/vector/
                    template_arguments = collect_template_arguments(tokens)
                    arg = tokenized_cxx_to_typelib(template_arguments.first, absolute)
                    result << "#{tk}<#{arg}>"
                elsif tk =~ /^\/?std\/basic_string/
                    collect_template_arguments(tokens)
                    result << "/std/string"
                elsif tk =~ /^[-\d<>,]+/
                    result << tk.gsub(/[-x]/, "0")
                elsif tk[0, 1] != "/"
                    result << "/#{tk}"
                else
                    result << tk
                end
            end
            result = result.join("")
            if absolute && result[0, 1] != "/"
                result = "/#{result}"
            end
            result
        end

        def cxx_to_typelib(name, absolute = true)
            self.class.cxx_to_typelib(name, absolute)
        end

        # Given a full Typelib type name, returns a [name, id] pair where +name+
        # is the type's basename and +id+ the context ID (i.e. the GCCXML
        # namespace ID)
        def resolve_namespace_of(xml, name)
            context = nil
            while name =~ /\/(\w+)\/(.*)/
                ns   = "/#{$1}"
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
            if !ns
                return []
            end

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
            Typelib.warn msg
        end

        def file_context(xmlnode)
            if (file = xmlnode["file"]) && (line = xmlnode["line"])
                "#{id_to_node[file]["name"]}:#{line}"
            end
        end

        def ignore(xmlnode, msg = nil)
            if msg
                if file = file_context(xmlnode)
                    warn("#{file}: #{msg}")
                else
                    warn(msg)
                end
            end
            ignore_message[xmlnode['id']] = msg
            id_to_name[xmlnode["id"]] = nil
        end

        # Returns if +name+ has been declared as an opaque
        def opaque?(name)
            opaques.include?(name)
        end

        def resolve_qualified_type(xml, xmlnode)
            spec = []
            if xmlnode['const'] == '1'
                spec << 'const'
            end
            if xmlnode['volatile'] == "1"
                spec << 'volatile'
            end
            type_names[xmlnode['id']] = "#{spec.join(" ")} #{resolve_type_id(xml, xmlnode['type'])}"
        end

        def set_source_file(type, xmlnode)
            id   = xmlnode['id']
            file = id_to_file[id]
            if !file
                binding.pry
                raise ArgumentError, "not type registered with ID #{id}"
            elsif line = xmlnode["line"]
                type.metadata.add('source_file_line', file + ":" + line)
            else
                type.metadata.add('source_file_line', file)
            end
        end

        def resolve_type_definition(xml, xmlnode)
            kind = xmlnode.name
            id   = xmlnode['id']
            if id_to_name.has_key?(id)
                return id_to_name[id]
            end

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
            elsif kind == "CvQualifiedType"
                name = resolve_qualified_type(xml, xmlnode)
                return ignore(xmlnode, "#{name}: cannot represent qualified types")
            end

            cxx_typename = (xmlnode['demangled'] || xmlnode['name'])
            if !cxx_typename
                raise "Internal error: #{xmlnode} has neither a demangled nor a name field"
            end

            name = cxx_to_typelib(cxx_typename)
            if name =~ /gccxml_workaround/
                return
            end

            if kind != "Typedef" && name =~ /\/__\w+$/
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
                    contained_type = template_args[0]
                    contained_node = demangled_to_node[contained_type]
                    if !contained_node
                        contained_node = name_to_nodes[contained_type].first
                    end
                    if !contained_node
                        contained_basename, contained_context = resolve_namespace_of(xml, template_args[0])
                        contained_node = name_to_nodes[contained_basename].
                            find { |node| node['context'].to_s == contained_context }
                    end
                    if !contained_node
                        raise "Internal error: cannot find definition for #{contained_type}"
                    end

                    if resolve_type_id(xml, contained_node["id"])
                        type = registry.create_container type_name, template_args[0]
                        set_source_file(type, xmlnode)
                    else return ignore("ignoring #{name} as its element type #{contained_type} is ignored as well")
                    end
                else
                    # Make sure that we can digest it. Forbidden are: inheritance,
                    # non-public members
                    #
                    # TODO: add inheritance support
                    if xmlnode['bases'] && !xmlnode['bases'].empty?
                        return ignore(xmlnode, "ignoring #{name} as it has parent classes")
                    end

                    fields = (xmlnode['members'] || "").split(" ").
                        map { |member_id| node_from_id(member_id) }.
                        compact.find_all { |member_node| member_node.name == "Field" }

                    if fields.empty?
                        return ignore(xmlnode, "ignoring the empty struct/class #{name}")
                    end

                    field_defs = fields.map do |field|
                        if field['access'] != 'public'
                            return ignore(xmlnode, "ignoring #{name} since its field #{field['name']} is private")
                        elsif field_type_name = resolve_type_id(xml, field['type'])
                            [field['name'], field_type_name, Integer(field['offset']) / 8]
                        else
                            return ignore(xmlnode, "ignoring #{name} since its field #{field['name']} is of the ignored type #{type_names[field['type']]}")
                        end
                    end
                    type = registry.create_compound(name, Integer(xmlnode['size']) / 8) do |c|
                        field_defs.each do |name, type, offset|
                            c.add(name, type, offset)
                        end
                    end
                    set_source_file(type, xmlnode)
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

                if full_name != registry.get(pointed_to_type).name
                    registry.alias(full_name, registry.get(pointed_to_type).name)

                    # And always resolve the typedef as the type it is pointing to
                    name = id_to_name[id] = pointed_to_type
                else
                    name = full_name
                end

            elsif kind == "Enumeration"
                if xmlnode['name'] =~ /^\._(\d+)$/ # this represents anonymous enums
                    return ignore(xmlnode, "ignoring anonymous enumeration, as they can't be represented in Typelib")
                elsif !(namespace = resolve_context(xml, xmlnode['context']))
                    return ignore(xmlnode, "ignoring enumeration #{name} as it is part of #{type_names[xmlnode['context']]} which is ignored")
                end

                full_name =
                    if namespace != "/"
                        "#{namespace}#{name}"
                    else name
                    end

                name = id_to_name[id] = full_name


                type = registry.create_enum full_name do |e|
                    (xmlnode / "EnumValue").each do |enum_value|
                        e.add(enum_value["name"], enum_value['init'])
                    end
                end
                set_source_file(type, xmlnode)
            else
                return ignore(xmlnode, "ignoring #{name} as it is of the unsupported GCCXML type #{kind}, XML node is #{xmlnode}")
            end

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
                name_to_nodes[name].find_all { |n| n.name == "Typedef" }.each do |typedef|
                    next if context && typedef["context"].to_s != context
                    type_node = node_from_id(typedef["type"].to_s)
                    namespace = resolve_context(xml, type_node['context'])
                    base = cxx_to_typelib(type_node["name"])
                    full_name = "#{namespace}#{base}"

                    opaques << full_name
                    registry.alias full_name, opaque_name
                end
            end
        end

        IGNORED_NODES = %w{Method OperatorMethod Destructor Constructor Function OperatorFunction}.to_set

        def load(required_files, xml)
            @registry = Typelib::Registry.new
            Typelib::Registry.add_standard_cxx_types(registry)

            @id_to_node = Hash.new
            @id_to_file = Hash.new
            @name_to_nodes = Hash.new { |h, k| h[k] = Array.new }
            @demangled_to_node = Hash.new

            # Enumerate all children of the root node in an id-to-node map, to
            # speed up lookup during the type resolution process
            root = (xml / "GCC_XML").first

            # Check here whether the GCC_XML node could be found
            # The actual temporary xml-file might not be empty, e.g. containing the xml version line only
            # thus need to check on the first node and any available children here
            if !root || !root.respond_to?("children")
                raise "gccxml generated incomplete xml, please verify that your /tmp folder has enough space left"
            end

            types_per_file = Hash.new { |h, k| h[k] = Array.new }
            typedefs_per_file = Hash.new { |h, k| h[k] = Array.new }
            root_file_ids = Array.new
            root.children.each do |child_node|
                next if IGNORED_NODES.include?(child_node.name)

                id_to_node[child_node["id"].to_s] = child_node
                if child_node.name != "File"
                    if (child_node_name = child_node['name'])
                        name_to_nodes[cxx_to_typelib(child_node_name)] << child_node
                    end
                    if (child_node_name = child_node['demangled'])
                        demangled_to_node[cxx_to_typelib(child_node_name)] = child_node
                    end
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
                    registry.create_opaque type_name, 0
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

            registry.to_xml
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

