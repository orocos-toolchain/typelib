require 'set'
require 'tempfile'
require 'shellwords'

module Typelib
    # Intermediate representation of a parsed GCCXML output, containing only
    # the information we need (and not the full XML representation)
    class GCCXMLInfo
        # During parsing, the last node that has been discovered
        attr_reader :current_node
        # @return [{String=>String}] the set of files that have been directly
        #   required, as a mapping from the full file path to the GCCXML ID that
        #   represents this file. The IDs are only filled after #parse is called
        attr_reader :required_files
        # @return [{String=>Node}] a mapping from a type ID to the XML node that
        #   stores its definition
        attr_reader :id_to_node
        # @return [{String=>Array<Node>}] a mapping from a mangled type name to the XML
        #   nodes that store its definition
        attr_reader :name_to_nodes
        # @return [{String=>Node}] a mapping from a demangled type name to the XML
        #   node that store its definition
        attr_reader :demangled_to_node
        # @return [{String=>String}] a mapping from all
        #   virtual methods/destructors/operators that have been found. It is
        #   used during resolution to reject the compounds that use them
        attr_reader :virtual_members
        # A mapping from a type ID to the XML node information about its base
        # classes
        attr_reader :bases
        # A mapping from an enum type ID to the XML node information about the
        # values that define it
        attr_reader :enum_values
        # @return [{String=>Array<Node>}] mapping from a file ID to the set of
        #   type-definition nodes that are defined within this file
        attr_reader :types_per_file
        # @return [{String=>Array<Node>}] mapping from a file ID to the set of
        #   typdef-definition nodes that are defined within this file
        attr_reader :typedefs_per_file

        Node = Struct.new :name, :attributes do
            def [](name)
                attributes[name]
            end
        end

        STORED_TAGS = %w{Namespace Typedef Enumeration Struct Class ArrayType FundamentalType PointerType}

        def initialize(required_files)
            @required_files = Hash.new
            required_files.each do |full_path|
                @required_files[full_path] = nil
            end
            @id_to_node = Hash.new
            @name_to_nodes = Hash.new { |h, k| h[k] = Array.new }
            @demangled_to_node = Hash.new
            @virtual_members = Set.new

            @bases = Hash.new { |h, k| h[k] = Array.new }
            @enum_values = Hash.new { |h, k| h[k] = Array.new }

            @types_per_file = Hash.new { |h, k| h[k] = Array.new }
            @typedefs_per_file = Hash.new { |h, k| h[k] = Array.new }
        end

        def file(attributes)
            file_name = attributes['name']
            if required_files.has_key?(file_name)
                required_files[file_name] = attributes['id']
            end
        end

        def tag_start(name, attributes)
            if name == "File"
                id_to_node[attributes['id']] = Node.new(name, attributes)
                return file(attributes)
            elsif name == "Field" || name == "FundamentalType"
                id_to_node[attributes['id']] = Node.new(name, attributes)
            elsif name == "Base"
                bases[current_node['id']] << Node.new(name, attributes)
            elsif name == "EnumValue"
                enum_values[current_node['id']] << Node.new(name, attributes)
            elsif attributes['virtual'] == '1'
                virtual_members << attributes['id']
            end

            return if !STORED_TAGS.include?(name)

            child_node = Node.new(name, attributes)
            id_to_node[child_node['id']] = child_node
            @current_node = child_node
            if (child_node_name = child_node['name'])
                STDOUT.puts "name #{child_node_name}"
                name_to_nodes[GCCXMLLoader.cxx_to_typelib(child_node_name)] << child_node
            end
            if (child_node_name = child_node['demangled'])
                STDOUT.puts "Got demangeled #{child_node_name}"
                demangled_to_node[GCCXMLLoader.cxx_to_typelib(child_node_name)] = child_node
            end

            if name == "Typedef"
                typedefs_per_file[attributes['file']] << child_node
            elsif %w{Struct Class Enumeration}.include?(name)
                types_per_file[attributes['file']] << child_node
            end
        end

        def parse(xml)
            lines = xml.split("\n")
            lines.shift
            root_tag = lines.shift
            if root_tag !~ /<GCC_XML/
                raise RuntimeError, "the provided XML input does not look like a GCCXML output (expected a root GCC_XML tag but got #{root_tag.chomp})"
            end

            lines.each do |l|
                if match = /<(\w+)/.match(l)
                    name = match[1]
                    parsing_needed = %w{File Field Base EnumValue}.include?(name) ||
                        STORED_TAGS.include?(name)

                    if !parsing_needed
                        if l =~ /virtual="1"/
                            l =~ /id="([^"]+)"/
                            tag_start(name, Hash['id' => $1, 'virtual' => '1'])
                        end
                    else
                        raw_attributes = l.gsub(/&lt;/, "<").gsub(/&gt;/, ">").scan(/\w+="[^"]+"/)
                        attributes = Hash.new
                        raw_attributes.each do |attr|
                            attr_name, attr_value = attr.split("=")
                            attributes[attr_name] = attr_value[1..-2]
                        end
                        tag_start(name, attributes)
                    end
                end
            end
        end
    end

    # A converted from the output of GCC-XML to the Typelib's own XML format
    class GCCXMLLoader
        # @return [GCCXMLInfo] The raw information contained in the GCCXML output
        attr_reader :info
        # The set of types that should be considered as opaques by the engine
        attr_accessor :opaques

        # A mapping from the type ID to the corresponding typelib type name
        #
        # If the type name is nil, it means that the type should not be
        # represented in typelib
        attr_reader :id_to_name

        # @return [{String=>String}] mapping from a type ID to the message
        #   explaining why it cannot be represented (it is "ignored")
        # @see {ignore}
        attr_reader :ignore_message

        # The registry that is being filled by parsing GCCXML output
        attr_reader :registry

        # A mapping from the type ID to the type names
        #
        # Unlike id_to_name, the stored value is not dependent of the fact that
        # the type has to be exported by typelib.
        attr_reader :type_names

        # Cached file contents (used to parse documentation)
        attr_reader :source_file_contents

        def node_from_id(id)
            info.id_to_node[id]
        end

        def initialize
            @opaques      = Set.new
            @id_to_name   = Hash.new
            @type_names   = Hash.new
            @ignore_message = Hash.new
            @source_file_contents = Hash.new
        end

        def self.template_tokenizer(name)
            CXX.template_tokenizer(name)
        end

        # Parses the Typelib or C++ type name +name+ and returns basename,
        # template_arguments
        def self.parse_template(name)
            CXX.parse_template(name)
        end

        def self.collect_template_arguments(tokens)
            CXX.collect_template_arguments(tokens)
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

        def self.cxx_to_typelib(name)
            name = name.gsub('::', '/')
            name = name.gsub('> >', '>>')

            tokens = GCCXMLLoader.template_tokenizer(name)
            tokenized_cxx_to_typelib(tokens)
        end

        def self.tokenized_cxx_to_typelib(tokens)
            result = []
            while !tokens.empty?
                tk = tokens.shift
                if tk =~ /^\/?std\/vector/
                    template_arguments = collect_template_arguments(tokens)
                    arg = tokenized_cxx_to_typelib(template_arguments.first)
                    if tk[0, 1] != "/"
                        tk = "/#{tk}"
                    end
                    result << "#{tk}<#{arg}>"
                elsif tk =~ /^[-\d<>,]+/
                    result << tk.gsub(/[-x]/, "0")
                elsif tk[0, 1] != "/"
                    result << "/#{tk}"
                else
                    result << tk
                end
            end
            result = result.join("")
            if result[0, 1] != "/"
                result = "/#{result}"
            end
            result
        end

        def cxx_to_typelib(name)
            self.class.cxx_to_typelib(name)
        end

        # Given a full Typelib type name, returns a [name, id] pair where +name+
        # is the type's basename and +id+ the context ID (i.e. the GCCXML
        # namespace ID)
        def resolve_namespace_of(name)
            context = nil
            while name =~ /\/(\w+)\/(.*)/
                ns   = "/#{$1}"
                name = "/#{$2}"
                candidates = info.name_to_nodes[ns].find_all { |n| n.name == "Namespace" }
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

        def resolve_node_name_parts(id_or_node)
            node = if id_or_node.respond_to?(:to_str)
                       info.id_to_node[id_or_node]
                   else
                       id_or_node
                   end

            if !node
                return []
            elsif name = id_to_name[node['id']]
                return name
            else
                name = cxx_to_typelib(node['name'])
                if name == "::"
                    return []
                end
                # Remove the leading slash
                id_to_name[node['id']] = (resolve_node_name_parts(node['context']) + [name[1..-1]])
            end

        end

        def resolve_node_typelib_name(id_or_node)
            resolve_node_name_parts(id_or_node).join("/")
        end

        def resolve_type_id(id)
            if id_to_name.has_key?(id.to_str)
                id_to_name[id.to_str]
            elsif typedef = node_from_id(id)
                resolve_type_definition(typedef)
            end
        end

        def warn(msg)
            Typelib.warn msg
        end

        def file_context(xmlnode)
            if (file = xmlnode["file"]) && (line = xmlnode["line"])
                "#{info.id_to_node[file]["name"]}:#{line}"
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

        def resolve_qualified_type(xmlnode)
            spec = []
            if xmlnode['const'] == '1'
                spec << 'const'
            end
            if xmlnode['volatile'] == "1"
                spec << 'volatile'
            end
            type_names[xmlnode['id']] = "#{spec.join(" ")} #{resolve_type_id(xmlnode['type'])}"
        end

        def source_file_for(xmlnode)
            if file = info.id_to_node[xmlnode['file']]['name']
                File.realpath(file)
            end
        rescue Errno::ENOENT
            File.expand_path(file)
        end

        def source_file_content(file)
            if source_file_contents.has_key?(file)
                source_file_contents[file]
            else
                if File.file?(file)
                    source_file_contents[file] = File.readlines(file, :encoding => 'utf-8')
                else
                    source_file_contents[file] = nil
                end
            end
        end

        def set_source_file(type, xmlnode)
            file = source_file_for(xmlnode)
            return if !file

            if (line = xmlnode["line"]) && (content = source_file_content(file))
                line = Integer(line)
                # GCCXML reports the file/line of the opening bracket for
                # struct/class/enum. We prefer the line of the
                # struct/class/enum definition.
                #
                # Moreover, gccxml's line numbering is 1-based (as it is the
                # common one for editors)
                while line >= 0 && content[line - 1] =~ /^\s*{?\s*$/
                    line = line - 1
                end
            end

            if line
                type.metadata.add('source_file_line', "#{file}:#{line}")
            else
                type.metadata.add('source_file_line', file)
            end
        end

        def resolve_type_definition(xmlnode)
            kind = xmlnode.name
            id   = xmlnode['id']
            if id_to_name.has_key?(id)
                return id_to_name[id]
            end

            if kind == "PointerType"
                if pointed_to_type = resolve_type_id(xmlnode['type'])
                    pointed_to_name = (id_to_name[id] = "#{pointed_to_type}*")
                    registry.build(pointed_to_name)
                    return pointed_to_name
                else return ignore(xmlnode)
                end
            elsif kind == "ArrayType"
                if pointed_to_type = resolve_type_id(xmlnode['type'])
                    value = xmlnode["max"]
                    if value =~ /^(\d+)u/
                        size = Integer($1) + 1
                    else
                        raise "expected NUMBERu for the 'max' attribute of an array definition, but got #{value}"
                    end
                    array_typename = (id_to_name[id] = "#{pointed_to_type}[#{size}]")
                    registry.create_array(pointed_to_type, size)
                    return array_typename
                else return ignore(xmlnode)
                end
            elsif kind == "CvQualifiedType"
                name = resolve_qualified_type(xmlnode)
                return ignore(xmlnode, "#{name}: cannot represent qualified types")
            end

            name = resolve_node_typelib_name(xmlnode)
            if name =~ /gccxml_workaround/
                return
            elsif name =~ /^\/std\/basic_string/
                registry.alias(name, '/std/string')
                name = "/std/string"
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
                type = registry.get(name)
                if name == type.name
                    set_source_file(type, xmlnode)
                end
                return name
            end

            if kind == "Struct" || kind == "Class"
                type_name, template_args = GCCXMLLoader.parse_template(name)
                if type_name == "/std/string"
                    # This is internally known to typelib
                elsif Typelib::Registry.available_containers.include?(type_name)
                    # This is known to Typelib as a container
                    contained_type = template_args[0]
                    contained_node = info.demangled_to_node[contained_type]
                    if !contained_node
                        contained_node = info.name_to_nodes[contained_type].first
                    end
                    if !contained_node
                        contained_basename, contained_context = resolve_namespace_of(template_args[0])
                        contained_node = info.name_to_nodes[contained_basename].
                            find { |node| node['context'].to_s == contained_context }
                    end
                    if !contained_node
                        raise "Internal error: cannot find definition for #{contained_type}"
                    end

                    if resolve_type_id(contained_node["id"])
                        type = registry.create_container type_name, template_args[0]
                        set_source_file(type, xmlnode)
                        registry.alias(name, type.name)
                        name = type.name
                    else return ignore("ignoring #{name} as its element type #{contained_type} is ignored as well")
                    end
                else
                    # Make sure that we can digest it. Forbidden are: non-public members
                    base_classes = info.bases[xmlnode['id']].map do |child_node|
                        if child_node['virtual'] != '0'
                            return ignore(xmlnode, "ignoring #{name}, it has virtual base classes")
                        elsif child_node['access'] != 'public'
                            return ignore(xmlnode, "ignoring #{name}, it has private base classes")
                        end
                        if base_type_name = resolve_type_id(child_node['type'])
                            base_type = registry.get(base_type_name)
                            [base_type, Integer(child_node['offset'])]
                        else
                            return ignore(xmlnode, "ignoring #{name}, it has ignored base classes")
                        end
                    end

                    member_ids = (xmlnode['members'] || '').split(" ")
                    member_ids.each do |id|
                        if info.virtual_members.include?(id)
                            return ignore(xmlnode, "ignoring #{name}, it has virtual methods")
                        end
                    end

                    fields = member_ids.map do |member_id|
                        if field_node = info.id_to_node[member_id]
                            if field_node.name == "Field"
                                field_node
                            end
                        end
                    end.compact
                    if fields.empty? && base_classes.all? { |type, _| type.empty? }
                        return ignore(xmlnode, "ignoring the empty struct/class #{name}")
                    end

                    field_defs = fields.map do |field|
                        if field['access'] != 'public'
                            return ignore(xmlnode, "ignoring #{name} since its field #{field['name']} is private")
                        elsif field_type_name = resolve_type_id(field['type'])
                            [field['name'], field_type_name, Integer(field['offset']) / 8, field['line']]
                        else
                            return ignore(xmlnode, "ignoring #{name} since its field #{field['name']} is of the ignored type #{type_names[field['type']]}")
                        end
                    end
                    type = registry.create_compound(name, Integer(xmlnode['size']) / 8) do |c|
                        base_classes.each do |base_type, base_offset|
                            base_type.each_field do |name, type|
                                offset = base_type.offset_of(name)
                                c.add(name, type, base_offset + offset)
                            end
                        end

                        field_defs.each do |field_name, field_type, field_offset, field_line|
                            c.add(field_name, field_type, field_offset)
                        end
                    end
                    set_source_file(type, xmlnode)
                    base_classes.each do |base_type, _|
                        type.metadata.add('base_classes', base_type.name)
                        base_type.each_field do |name, _|
                            base_type.field_metadata[name].get('source_file_line').each do |file_line|
                                type.field_metadata[name].add('source_file_line', file_line)
                            end
                        end
                    end
                    if file = source_file_for(xmlnode)
                        field_defs.each do |field_name, _, _, field_line|
                            type.field_metadata[field_name].set('source_file_line', "#{file}:#{field_line}")
                        end
                    end
                end
            elsif kind == "FundamentalType"
                if !registry.include?(name)
                    return ignore(xmlnode)
                end
            elsif kind == "Typedef"
                type_names[id] = name
                id_to_name[id] = name
                if opaque?(name)
                    type = registry.get(name)
                    if type.name == name
                        # Nothing to do ...
                        set_source_file(type, xmlnode)
                    end
                    return name
                end

                if !(pointed_to_type = resolve_type_id(xmlnode['type']))
                    return ignore(xmlnode, "cannot create the #{name} typedef, as it points to #{type_names[xmlnode['type']]} which is ignored")
                end

                if name != registry.get(pointed_to_type).name
                    registry.alias(name, registry.get(pointed_to_type).name)

                    # And always resolve the typedef as the type it is pointing to
                    name = id_to_name[id] = pointed_to_type
                end

            elsif kind == "Enumeration"
                if xmlnode['name'] =~ /^\._(\d+)$/ # this represents anonymous enums
                    return ignore(xmlnode, "ignoring anonymous enumeration, as they can't be represented in Typelib")
                elsif !(namespace = resolve_node_typelib_name(xmlnode['context']))
                    return ignore(xmlnode, "ignoring enumeration #{name} as it is part of #{type_names[xmlnode['context']]} which is ignored")
                end

                full_name =
                    if namespace != "/"
                        "#{namespace}#{name}"
                    else name
                    end

                name = id_to_name[id] = full_name


                type = registry.create_enum full_name do |e|
                    info.enum_values[id].each do |enum_value|
                        e.add(enum_value["name"], Integer(enum_value['init']))
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
        def resolve_opaques
            # First do typedefs. Search for the typedefs that are named like our
            # type, if we find one, alias it
            opaques.dup.each do |opaque_name|
                name, context = resolve_namespace_of(opaque_name)
                next if !context # this opaque does not appear in the loaded headers
                info.name_to_nodes[name].find_all { |n| n.name == "Typedef" }.each do |typedef|
                    next if context && typedef["context"].to_s != context
                    type_node = node_from_id(typedef["type"].to_s)
                    full_name = resolve_node_typelib_name(type_node)

                    opaques << full_name
                    opaque_t = registry.get(opaque_name)
                    set_source_file(opaque_t, typedef)
                    opaque_t.metadata.set('opaque_is_typedef', '1')
                    registry.alias full_name, opaque_name
                end
            end
        end

        def self.parse_cxx_documentation_before(lines, line)
            lines ||= Array.new

            block = []
            # Lines are given 1-based (as all editors work that way), and we
            # want the line before the definition. Remove two
            line = line - 2
            while true
                case l = lines[line]
                when /^\s*$/
                when /^\s*(\*|\/\/|\/\*)/
                    block << l
                else break
                end
                line = line - 1
            end
            block = block.map do |l|
                l.strip.gsub(/^\s*(\*+\/?|\/+\**)/, '')
            end
            while block.first && block.first.strip == ""
                block.shift
            end
            while block.last && block.last.strip == ""
                block.pop
            end
            # Now remove the same amount of spaces in front of each lines
            space_count = block.map do |l|
                l =~ /^(\s*)/
                if $1.size != l.size
                    $1.size
                end
            end.compact.min
            block = block.map do |l|
                l[space_count..-1]
            end
            if last_line = block[0]
                last_line.gsub!(/\*+\//, '')
            end
            if !block.empty?
                block.reverse.join("\n")
            end
        end

        IGNORED_NODES = %w{Method OperatorMethod Destructor Constructor Function OperatorFunction}.to_set

        def load(required_files, xml)
            @registry = Typelib::Registry.new
            Typelib::Registry.add_standard_cxx_types(registry)
            @info = GCCXMLInfo.new(required_files)
            info.parse(xml)

            all_types = Array.new
            all_typedefs = Array.new
            info.required_files.each_value do |file_id|
                all_types.concat(info.types_per_file[file_id])
                all_typedefs.concat(info.typedefs_per_file[file_id])
            end

            if !opaques.empty?
                # We MUST emit the opaque definitions before calling
                # resolve_opaques as resolve_opaques will add the resolved
                # opaque names to +opaques+
                opaques.each do |type_name|
                    registry.create_opaque type_name, 0
                end
                resolve_opaques
            end

            # Resolve structs and classes
            all_types.each do |node|
                resolve_type_definition(node)
            end

            # Look at typedefs
            all_typedefs.each do |node|
                resolve_type_definition(node)
            end

            # Now, parse documentation for every type and field for which we
            # have a source file/line
            registry.each do |type|
                if location = type.metadata.get('source_file_line').first
                    file, line = location.split(':')
                    line = Integer(line)
                    if doc = GCCXMLLoader.parse_cxx_documentation_before(source_file_content(file), line)
                        type.metadata.set('doc', doc)
                    end
                end
                if type.respond_to?(:field_metadata)
                    type.field_metadata.each do |field_name, md|
                        if location = md.get('source_file_line').first
                            file, line = location.split(':')
                            line = Integer(line)
                            if doc = GCCXMLLoader.parse_cxx_documentation_before(source_file_content(file), line)
                                md.set('doc', doc)
                            end
                        end
                    end
                end
            end

            registry
        end

        class << self
            # Set of options that should be passed to the gccxml binary
            #
            # it is usually a set of options required to workaround the
            # limitations of gccxml, as e.g. passing -DEIGEN_DONT_VECTORIZE when
            # importing the Eigen headers
            #
            # @return [Array]
            attr_reader :gccxml_default_options
            attr_reader :castxml_default_options
        end
        @gccxml_default_options = Shellwords.split(ENV['TYPELIB_GCCXML_DEFAULT_OPTIONS'] || '-DEIGEN_DONT_VECTORIZE')
        @castxml_default_options = Shellwords.split(ENV['TYPELIB_CASTXML_DEFAULT_OPTIONS'] || '-DEIGEN_DONT_VECTORIZE')

        #figure out the correct gccxml binary name, debian has changed this name 
        #to gccxml.real
        def self.gcc_binary_name
            if !`which gccxml.real`.empty?
                return "gccxml.real"
            end
            return "gccxml"
        end

        def self.use_castxml
            if !`which castxml`.empty?
                return "castxml"
            end
            return "gccxml"

        end
        # Runs castxml on the provided file and with the given options, and
        # return the Nokogiri::XML object representing the result
        #
        # Raises RuntimeError if casrxml failed to run
        def self.castxml(file, options)
            cmdline = [use_castxml, *castxml_default_options]
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
            
            Tempfile.open('typelib_gccxml') do |io|
                cmdline << "--castxml-gccxml" 
                cmdline << "-o #{io.path}"
            
                STDOUT.puts "2 -----  "
                STDOUT.puts "with call #{cmdline.join(' ')} "
                STDOUT.puts "2 -----  "


                #if !system(cmdline.join(" "))
                if !system(cmdline.join(" "))
                    raise ArgumentError, "gccxml returned an error while parsing #{file} with call #{cmdline.join(' ')} "
                end
                
                FileUtils.copy_file(io.path, "/tmp/debug2.castxml")
                return ""
                
                io.open
                return io.read
            end
        end
        # Runs gccxml on the provided file and with the given options, and
        # return the Nokogiri::XML object representing the result
        #
        # Raises RuntimeError if gccxml failed to run
        def self.gccxml(file, options)
            cmdline = [gcc_binary_name, *gccxml_default_options]
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

            Tempfile.open('typelib_gccxml') do |io|
                cmdline << "-fxml=#{io.path}"
                if !system(*cmdline)
                    raise ArgumentError, "gccxml returned an error while parsing #{file}"
                end
                 
                FileUtils.copy_file(io.path, "/tmp/debug2.gccxml")

                return io.read
            end
        end

        def self.load(registry, file, kind, options)
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

            castxml(file, options)
            xml = gccxml(file, options)
            converter = GCCXMLLoader.new
            converter.opaques = opaques
            if opaques = options[:opaques]
                converter.opaques |= opaques.to_set
            end
            gccxml_registry = converter.load(required_files, xml)
            registry.merge(gccxml_registry)
        end

        def self.preprocess(files, kind, options)
            includes = options[:include].map { |v| "-I#{v}" }
            defines  = options[:define].map { |v| "-D#{v}" }

            Tempfile.open(['orogen_gccxml_input','.hpp']) do |io|
                files.each do |path|
                    io.puts "#include <#{path}>"
                end
                io.flush
                call = [gcc_binary_name, "--preprocess", *includes, *defines, *gccxml_default_options, io.path]
                if true #debug
                    call = [gcc_binary_name, "--preprocess", *includes, *defines, *gccxml_default_options, io.path]
                    File.open("/tmp/debug1.gccxml","w+"){|f| f.write `#{call.join(' ')}`}
                end
                if true 
                    #call = [use_castxml, "--castxml-gccxml", "-E", *includes, *defines, *castxml_default_options, io.path] 
                    call = [use_castxml, "--castxml-gccxml", "-E", *includes, *defines, *castxml_default_options, io.path] 
                    File.open("/tmp/debug1.castxml","w+"){|f| f.write `#{call.join(' ')}`}
                end
                
                call = [gcc_binary_name, "--preprocess", *includes, *defines, *gccxml_default_options, io.path]
                result = IO.popen(call) do |gccxml_io|
                    gccxml_io.read
                end

#                STDOUT.puts "-------------"
#                STDOUT.puts "#{call.join(' ')}"
#                STDOUT.puts "-------------"
#                FileUtils.copy_file(io.path, "/tmp/gcc-debug.hpp")

                if !$?.success?
                    FileUtils.copy_file(io.path, "/tmp/gcc-debug.hpp")
                    raise ArgumentError, "failed to preprocess #{files.join(" ")} \"#{call[0..-1].join(" ")} /tmp/gcc-debug\""
                end
            end
        end
    end

    class Registry
        # @deprecated use {GCCXMLLoader.load} and {CXX.load}
        def self.load_from_gccxml(registry, file, kind, options)
            GCCXMLLoader.load(registry, file, kind, options)
        end

        # Returns true if Registry#import will use GCCXML to load the given
        # file
        def self.uses_gccxml?(path, kind = 'auto')
            (handler_for(path, kind) == method(:load_from_gccxml))
        end
    end
end

