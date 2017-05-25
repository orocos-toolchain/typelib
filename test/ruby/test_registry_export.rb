require 'typelib/test'

describe Typelib::RegistryExport do
    attr_reader :reg, :root
    before do
        @reg = Typelib::Registry.new
        @root = Typelib::RegistryExport::Namespace.new
        root.reset_registry_export(reg, nil, '/')
    end

    describe "access from constants" do
        before do
            @original_logger_level = Typelib.logger.level
            Typelib.logger.level = Logger::FATAL
        end
        after do
            Typelib.logger.level = @original_logger_level
        end

        it "gives access to toplevel objects" do
            type = reg.create_compound '/CustomType'
            assert_same type, root::CustomType
        end
        it "raises NameError if the type does not exist" do
            error = assert_raises(Typelib::RegistryExport::NotFound) { root::DoesNotExist }
            assert((error.message =~ /DoesNotExist/), "expected the NotFound message to include the name of the type (DoesNotExist), but the message was #{error.message}")
        end
        it "handles types mixed case if needed" do
            type = reg.create_compound '/CustomTypeWith_camelCase'
            assert_same type, root::Custom_typeWith_camelCase
        end
        it "handles namespaces mixed case if needed" do
            type = reg.create_compound '/CustomTypeWith_camelCase/Child'
            assert_same type, root::Custom_typeWith_camelCase::Child
        end
        it "gives access to types defined inside other types" do
            parent_type = reg.create_compound '/Parent'
            child_type = reg.create_compound '/Parent/Child'
            assert_same parent_type, root::Parent
            assert_same child_type, root::Parent::Child
        end
        it "gives access to types defined inside namespaces" do
            type = reg.create_compound '/NS/Child'
            assert_same type, root::NS::Child
        end
    end

    describe "access from methods" do
        it "raises TypeNotFound if the type does not exist" do
            assert_raises(Typelib::RegistryExport::NotFound) { root.DoesNotExist }
        end
        it "gives access to toplevel objects" do
            type = reg.create_compound '/CustomType'
            assert_same type, root.CustomType
        end
        it "gives access to types defined inside other types" do
            parent_type = reg.create_compound '/Parent'
            child_type = reg.create_compound '/Parent/Child'
            assert_same parent_type, root.Parent
            assert_same child_type, root.Parent.Child
        end
        it "gives access to types defined inside namespaces" do
            type = reg.create_compound '/NS/Child'
            assert_same type, root.NS.Child
        end
        it "gives access to template types" do
            parent_type = reg.create_compound '/NS/Template<-1,/TEST>'
            child_type = reg.create_compound '/NS/Template<-1,/TEST>/Child'
            assert_equal parent_type, root.NS.Template(-1,'/TEST')
            assert_equal child_type, root.NS.Template(-1,'/TEST').Child
        end
        it "gives access to template namespaces" do
            type = reg.create_compound '/NS/Template<-1,/TEST>/Child'
            assert_equal type, root.NS.Template(-1,'/TEST').Child
        end
        it "returns the type as converted to ruby" do
            type = reg.create_compound '/CustomType'
            type.convert_to_ruby(Array) { Array.new }
            assert_equal Array.new, root.CustomType.new
        end
    end

    describe "the filter block" do
        attr_reader :type
        before do
            @type = reg.create_compound '/CustomType'
            type.convert_to_ruby(Array) { Array.new }
        end

        it "gets passed the type as well as the type as converted to ruby" do
            recorder = flexmock
            recorder.should_receive(:call).with(type, Array).once
            root.reset_registry_export(reg, ->(type, ruby_type) { recorder.call(type, ruby_type); type })
            root.CustomType
        end
        it "returns the type as returned by the block" do
            root.reset_registry_export(reg, ->(type, ruby_type) { Hash })
            assert_kind_of Hash, root.CustomType.new
        end
        it "performs the conversion-to-ruby for the returned type" do
            target_t = reg.create_compound '/WrapperType'
            target_t.convert_to_ruby(Hash) { Hash.new }
            root.reset_registry_export(reg, ->(type, ruby_type) { ruby_type })
            assert_kind_of Hash, root.WrapperType.new
        end
        it "will consider that types for which the block returned nil do not exist" do
            root.reset_registry_export(reg, ->(type, ruby_type) { })
            assert_raises(Typelib::RegistryExport::NotFound) do
                root.CustomType
            end
        end
    end
end

