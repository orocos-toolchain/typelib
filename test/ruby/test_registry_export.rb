require 'typelib/test'

describe Typelib::RegistryExport do
    attr_reader :reg, :root
    before do
        @reg = Typelib::Registry.new
        @root = Typelib::RegistryExport::Namespace.new
        root.reset_registry_export(reg, nil)
    end

    describe "access from constants" do
        it "gives access to toplevel objects" do
            type = reg.create_compound '/CustomType'
            assert_same type, root::CustomType
        end
        it "raises NameError if the type does not exist" do
            assert_raises(Typelib::RegistryExport::NotFound) { root::DoesNotExist }
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
    end
end


