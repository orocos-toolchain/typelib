#include "TypelibBuilder.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/Type.h"
#include <clang/AST/DeclTemplate.h>
#include <iostream>
#include <typelib/registry.hh>
#include <typelib/typemodel.hh>
#include <typelib/typename.hh>
#include <lang/tlb/import.hh>



void TypelibBuilder::registerNamedDecl(const clang::TypeDecl* decl)
{
    //check if we allready know the type
    std::string typeName = "/" + decl->getQualifiedNameAsString();
    
//     std::cout << "New Type with qualified name " << typeName << std::endl;

    typeName = cxxToTyplibName(typeName);
    
//     std::cout << "Typelib name " << typeName << std::endl;
    
    if(typeName.size() <= 1)
        return;
    
    if(registry.has(typeName, false))
        return;


    if(decl->getKind() == clang::Decl::Typedef)
    {
        registerTypeDef(static_cast<const clang::TypedefDecl *>(decl));
        return;
    }
    
    const clang::Type *typeForDecl = decl->getTypeForDecl();
    if(!typeForDecl)
    {
        std::cout << "TypeDecl '" << decl->getNameAsString() << "' has no type " << std::endl;
        return;
    }

    //check for structs that are only defined inside of functions
    if(decl->getParentFunctionOrMethod())
    {
        std::cout << "Ignoring type '" << typeName << "' as it is defined inside a function" << std::endl;
        return;
    }
    
    if(decl->isHidden())
    {
        std::cout << "Ignoring hidden type '" << typeName << "' because it is hidden" << std::endl;
        return;
    }
    
    if(decl->isInAnonymousNamespace())
    {
        std::cout << "Ignoring '" << typeName << "' as it is in an anonymous namespace" << std::endl;
        return;
    }    
    
    registerType(typeName, typeForDecl, decl->getASTContext());
}

bool TypelibBuilder::checkRegisterContainer(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl)
{
    
    const clang::NamedDecl *underlyingType = decl->getUnderlyingDecl();
    
    
    // skip non-template specializations
    if (!underlyingType ||
        (underlyingType->getKind() != clang::Decl::ClassTemplateSpecialization))
        return false;

    // some things of later use
    const clang::ClassTemplateSpecializationDecl *sdecl =
        static_cast<const clang::ClassTemplateSpecializationDecl *>(decl);
    const clang::TemplateArgumentList &argumentList(sdecl->getTemplateArgs());

    std::cout << canonicalTypeName <<  " is possibly a Container " << std::endl;
    
    
    std::cout << "Underlying name " << decl->getUnderlyingDecl()->getQualifiedNameAsString() << std::endl;;

    const Typelib::Container::AvailableContainers& containers = Typelib::Container::availableContainers();

    std::string containerName = cxxToTyplibName(underlyingType->getQualifiedNameAsString());

    Typelib::Container::AvailableContainers::const_iterator it = containers.find(containerName);
    if(it != containers.end())
    {
        std::cout << "Typelib knowns about this container: '" << it->first << "'" << std::endl;
        
        Typelib::Container::ContainerFactory factory = it->second;
        
        std::list<const Typelib::Type *> typelibArgList;
        //create argument list
        for(size_t i = 0; i < argumentList.size(); i++)
        {
            clang::TemplateArgument arg = argumentList.get(i);
            const clang::Type *typePtr = arg.getAsType().getTypePtr();
            if(!typePtr)
            {
                std::cout << "Error, argument has not type" << std::endl;
                return false;
            }
            
            std::string argTypelibName = getTypelibNameForQualType(arg.getAsType().getCanonicalType());
            
            //HACK ignore allocators
#warning HACK, ignoring types named '/std/allocator'
            if(argTypelibName.find("/std/allocator") == 0)
            {
                continue;
            }

#warning HACK, ignoring '/std/char_traits' for std::basic_string support
            if(argTypelibName.find("/std/char_traits") == 0)
            {
                continue;
            }
            
            std::string originalTypeName = getTypelibNameForQualType(arg.getAsType().getCanonicalType());
            
            const Typelib::Type *argType = checkRegisterType(originalTypeName, typePtr, decl->getASTContext());
            if(!argType)
            {
                return false;
            }
            
            if(containerName == "/std/string" && originalTypeName != "/char")
            {
                std::cout << "Ignoring any basic string, that is not of argument type char" << std::endl;
                //wo only support std::basic_string<char>
                return false;
            }
            
            typelibArgList.push_back(argType);
            
            std::cout << "Arg is '" << getTypelibNameForQualType(arg.getAsType()) << "'" << std::endl;
        }
        
        
        const Typelib::Container &newContainer(factory(registry, typelibArgList));
        
        if(newContainer.getName() != canonicalTypeName)
        {
            registry.alias(newContainer.getName(), canonicalTypeName);
        }
        
        std::cout << "Container registerd" << std::endl;
        
        return true;
    }

    return false;
}

void TypelibBuilder::lookupOpaque(const clang::TypeDecl* decl)
{

    std::string opaqueName = cxxToTyplibName(decl->getQualifiedNameAsString());
    std::string canonicalOpaqueName;
    
    if(decl->getKind() == clang::Decl::Typedef)
    {
        const clang::TypedefDecl *typeDefDecl = static_cast<const clang::TypedefDecl *>(decl);
        canonicalOpaqueName = getTypelibNameForQualType(
            typeDefDecl->getUnderlyingType().getCanonicalType());
    }
    else
    {
        if(!decl->getTypeForDecl())
        {
            std::cout
                << "Could not get Type for Opaque Declaration '"
                << decl->getQualifiedNameAsString() << "'" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        canonicalOpaqueName = getTypelibNameForQualType(decl->getTypeForDecl()->getCanonicalTypeInternal());
        
    }

    Typelib::Type *opaqueType = registry.get_(opaqueName);
    setHeaderPathForTypeFromDecl(decl, *opaqueType);

    // typdef-opaques are specially marked in the Typelib metadata
    if(decl->getKind() == clang::Decl::Typedef) {
        opaqueType->getMetaData().add("opaque_is_typedef", "1");
    }

    std::cout << "Resolved Opaque '" << opaqueName << "' to '"
              << canonicalOpaqueName << "'" << std::endl;

    if(opaqueName != canonicalOpaqueName)
    {
        //as we want to resolve opaques by their canonical name
        //we need to register an alias from the canonical name 
        //to the opaque name.
        registry.alias(opaqueName, canonicalOpaqueName);
    }
}


bool TypelibBuilder::registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context)
{
    
    std::string typeName = std::string("/") + builtin->getNameAsCString(clang::PrintingPolicy(clang::LangOptions()));
    
    if(registry.has(typeName, false))
        return true;
    
    Typelib::Numeric *newNumeric = 0;
    size_t typeSize = context.getTypeSize(builtin->desugar());
    if(typeSize % 8 != 0)
    {
        std::cout << "Warning, can not register type which is not Byte Aligned '" << canonicalTypeName << "'" << std::endl;
        return false;
    }
    
    typeSize /= 8;
    
    if(builtin->isFloatingPoint())
    {
        newNumeric = new Typelib::Numeric(typeName, typeSize, Typelib::Numeric::Float);
    }
    
    if(builtin->isInteger())
    {
        if(builtin->isSignedInteger())
        {
            if(typeName == "/char")
            {
                typeName = "/int8_t";
                newNumeric =new Typelib::Numeric(typeName, typeSize, Typelib::Numeric::SInt);
                registry.add(newNumeric);
                registry.alias(newNumeric->getName(), "/char");
                return true;
            }
            else
                newNumeric =new Typelib::Numeric(typeName, typeSize, Typelib::Numeric::SInt);
        }
        else
        {
            if(typeName == "/char")
            {
                typeName = "/uint8_t";
                newNumeric =new Typelib::Numeric(typeName, typeSize, Typelib::Numeric::UInt);
                registry.add(newNumeric);
                registry.alias(newNumeric->getName(), "/char");
                return true;
            }
            else
                newNumeric =new Typelib::Numeric(typeName, typeSize, Typelib::Numeric::UInt);
        }
    }
    
    if(newNumeric)
    {
        registry.add(newNumeric);
        return true;
    }
    
    return false;
}

bool TypelibBuilder::isConstant(const std::string name, size_t pos)
{
    if(name.size() > pos)
    {
        char c = name.at(pos);
        if(std::isalpha(c) || c == '_')
            return false;
        
        return true;
    }
    return false;
}

std::string TypelibBuilder::cxxToTyplibName(const std::string name)
{
    std::string ret(name);
    
    if(!ret.empty() && ret.at(0) != '/')
    {
        ret.insert(0, "/");
    }
    
    std::string from("::");
    std::string to("/");
    
    for (size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos))
    {
        ret.replace(start_pos, from.length(), to);
    }
    
    from = std::string("<");
    to = std::string("</");
    
    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos))
    {
        if(!isConstant(ret, start_pos + 1))
        {
            ret.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        else
        {
            start_pos += from.length();
        }
    }
    
    from = std::string(", ");
    to = std::string(",/");
    std::string toConst(",");
    
    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos))
    {
        if(!isConstant(ret, start_pos + 2))
        {
            ret.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        else
        {
            ret.replace(start_pos, from.length(), toConst);
            start_pos += toConst.length();
        }
    }
    
    from = std::string(" >");
    to = std::string(">");
    
    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }

    from = std::string(" &");
    to = std::string("&");
    
    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }

    from = std::string(" [");
    to = std::string("[");
    
    for(size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }
    
#warning HACK, renaming '/std/basic_string</char>' to '/std/string'
    if(ret == "/std/basic_string")
        return "/std/string";
    
    if(ret == "/std/basic_string</char>")
        return "/std/string";
    
    return ret;
}

std::string TypelibBuilder::typlibtoCxxName(const std::string name)
{
    std::string ret(name);
    
    std::string from("/");
    std::string to("::");
    
    for (size_t start_pos = ret.find(from); start_pos != std::string::npos; start_pos = ret.find(from, start_pos + to.length()))
    {
        ret.replace(start_pos, from.length(), to);
    }
    

    return ret;
}

bool TypelibBuilder::registerType(const std::string& canonicalTypeName, const clang::Type* type, clang::ASTContext& context)
{
    
    if(canonicalTypeName.find("&") != std::string::npos)
    {
        std::cout << "Ignoring type with reference '" << canonicalTypeName << "'" << std::endl;
        return false;
    }
    
    // FIXME: this is bound to break...
    if(canonicalTypeName.find("sizeof") != std::string::npos)
    {
        std::cout << "Ignoring type with weird sizeof '" << canonicalTypeName << "'" << std::endl;
        return false;
    }

    if(canonicalTypeName.find("(") != std::string::npos)
    {
        std::cout << "Ignoring type with function pointer '" << canonicalTypeName << "'" << std::endl;
        return false;
    }
    
    
    switch(type->getTypeClass())
    {
        case clang::Type::Builtin:
        {
            const clang::BuiltinType *builtin = static_cast<const clang::BuiltinType *>(type);
            
            return registerBuildIn(canonicalTypeName, builtin, context);
        }
        case clang::Type::Record:
        {
            return addRecord(canonicalTypeName, type->getAsCXXRecordDecl());
        }
        case clang::Type::Enum:
        {
            const clang::EnumType *enumType = static_cast<const clang::EnumType *>(type);
            const clang::EnumDecl *enumDecl = enumType->getDecl();
            assert(enumDecl);
            return addEnum(canonicalTypeName, enumDecl);
        }
        case clang::Type::ConstantArray:
        {            
            return addArray(canonicalTypeName, type, context);
        }
        case clang::Type::Elaborated:
        {            
            const clang::ElaboratedType* etype = static_cast<const clang::ElaboratedType*>(type);

            //hm, this type is somehow strange, I have no idea, what it actually is...
            return registerType(canonicalTypeName, etype->getNamedType().getTypePtr(), context);
            
        }
        default:
            std::cout << "Cannot register '" << canonicalTypeName << "'"
                      << " with unhandled type '" << type->getTypeClassName()
                      << "'" << std::endl;
    }
    return false;
    
}

const Typelib::Type* TypelibBuilder::checkRegisterType(const std::string& canonicalTypeName, const clang::Type *type, clang::ASTContext& context)
{
    if(!registry.has(canonicalTypeName, false))
    {
        std::cout << "Trying to register unknown Type '" << canonicalTypeName << "'" << std::endl;
        
        if(!registerType(canonicalTypeName, type, context))
        {
            return NULL;
        }
    }
    else
    {
//         std::cout << "registry claims to know " << canonicalTypeName << std::endl;
    }
    
    
    const Typelib::Type *typelibType = registry.get(canonicalTypeName);

    if(!typelibType)
    {
        std::cout << "Internal error : Just registed Type '" << canonicalTypeName << "' was not found in registry" << std::endl;
        exit(EXIT_FAILURE);
    }

    return typelibType;
}


bool TypelibBuilder::addArray(const std::string& canonicalTypeName, const clang::Type *gtype, clang::ASTContext& context)
{
    const clang::ConstantArrayType *type = static_cast<const clang::ConstantArrayType *>(gtype);
    const clang::Type *arrayBaseType = type->getElementType().getTypePtr();
    std::string arrayBaseTypeName = getTypelibNameForQualType(type->getElementType());
    
    const Typelib::Type *typelibArrayBaseType = checkRegisterType(arrayBaseTypeName, arrayBaseType, context);
    if(!typelibArrayBaseType)
    {
        std::cout << "Not registering Array '" << canonicalTypeName
                  << "' as its elementary type '" << arrayBaseTypeName
                  << "' could not be registered " << std::endl;
        return false;
    }
    
    Typelib::Array *array = new Typelib::Array(*typelibArrayBaseType, type->getSize().getZExtValue());

    registry.add(array);
    
    return true;
}


bool TypelibBuilder::addEnum(const std::string& canonicalTypeName, const clang::EnumDecl *decl)
{
    Typelib::Enum *enumVal =new Typelib::Enum(canonicalTypeName);
    setHeaderPathForTypeFromDecl(decl, *enumVal);

    if(!decl->getIdentifier())
    {
        std::cout << "Ignoring type '" << canonicalTypeName
                  << "' without proper identifier" << std::endl;
        return false;
    }
    
    for(clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin(); it != decl->enumerator_end(); it++)
    {
        enumVal->add(it->getDeclName().getAsString(), it->getInitVal().getSExtValue());
//         std::cout << "Enum CONST " << it->getDeclName().getAsString() << " Value " << it->getInitVal().getSExtValue() << std::endl;
    }
    
    registry.add(enumVal);
    
    return true;
}

bool TypelibBuilder::addBaseClassToCompound(Typelib::Compound& compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl)
{
    for(clang::CXXRecordDecl::base_class_const_iterator it = decl->bases_begin(); it != decl->bases_end(); it++)
    {
        const clang::CXXRecordDecl* curDecl = it->getType()->getAsCXXRecordDecl();
        
        addBaseClassToCompound(compound, canonicalTypeName, curDecl);
        
        if(!addFieldsToCompound(compound, canonicalTypeName, curDecl))
        {
            return false;
        }
    }
    
    return true;
}


bool TypelibBuilder::addRecord(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl)
{
    if(!decl)
    {
        std::cout << "Warning, got NULL Type" << std::endl;
        return false;
    }

    if(!decl->getIdentifier())
    {
        std::cout << "Ignoring type '" << canonicalTypeName
                  << "' without proper identifier" << std::endl;
        return false;
    }

    if(!decl->hasDefinition())
    {
        std::cout << "Ignoring type '" << canonicalTypeName << "' as it has no definition " << std::endl;
        return false;
    }
    
    if(decl->isInjectedClassName())
    {
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it is injected" << std::endl;
        return false;
    }
    
    if(decl->isPolymorphic() || decl->isAbstract())
    {
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it is polymorphic" << std::endl;
        return false;
    }
    
    if(decl->isDependentType() || decl->isInvalidDecl())
    {
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it is dependents / Invalid " << std::endl;
        //ignore incomplete / forward declared types
        return false;
    }
    
    const clang::ASTRecordLayout &typeLayout(decl->getASTContext().getASTRecordLayout(decl));

    
    //container are special record, who have a seperate handling.
    if(checkRegisterContainer(canonicalTypeName, decl))
    {
        return true;
    }
    
    
    Typelib::Compound *compound = new Typelib::Compound(canonicalTypeName);

    size_t typeSize = typeLayout.getSize().getQuantity();
    compound->setSize(typeSize);

    setHeaderPathForTypeFromDecl(decl, *compound);
    if(!addBaseClassToCompound(*compound, canonicalTypeName, decl))
    {
        delete compound;
        return false;
    }

    if(!addFieldsToCompound(*compound, canonicalTypeName, decl))
    {
        delete compound;
        return false;
    }

    if(compound->getFields().empty())
    {
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it has no fields " << std::endl;
        return false;
    }
    
//     std::cout << "Registered Compound " << canonicalTypeName << std::endl;

    registry.add(compound);
    
    return true;
}

void TypelibBuilder::setHeaderPathForTypeFromDecl(const clang::Decl* decl, Typelib::Type& type)
{
    std::string sourceLocation =
        decl->getSourceRange().getBegin().printToString(
            decl->getASTContext().getSourceManager());

    // clang knows the "/path/to/file:line:column" while we in typelib only(?)
    // need the column information. we have to detect the last colon, so that
    // we can put just the beginning of the string into the metadata of the
    // type-object
    type.setPathToDefiningHeader(
        sourceLocation.substr(0, sourceLocation.find_last_of(':')));

    type.getMetaData().add("source_file_line", type.getPathToDefiningHeader());
}


std::string TypelibBuilder::getTypelibNameForQualType(const clang::QualType& type)
{
    const clang::QualType qualType = type.getLocalUnqualifiedType().getCanonicalType();
        
    clang::LangOptions o;
    clang::PrintingPolicy p(o);
    p.SuppressTagKeyword = true;

    return cxxToTyplibName(qualType.getAsString(p));
}


bool TypelibBuilder::addFieldsToCompound(Typelib::Compound& compound, const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl)
{
    const clang::ASTRecordLayout &typeLayout(decl->getASTContext().getASTRecordLayout(decl));

    for(clang::RecordDecl::field_iterator fit = decl->field_begin(); fit != decl->field_end(); fit++)
    {
//         TemporaryFieldType fieldType;
        const clang::QualType qualType = fit->getType().getLocalUnqualifiedType().getCanonicalType();

        if (fit->isAnonymousStructOrUnion()) {
            std::cout
                << "Warning, ignoring Record with Anonymous Struct or Union '"
                << canonicalTypeName << "'" << std::endl;
            return false;
        }

        clang::LangOptions o;
        clang::PrintingPolicy p(o);
        p.SuppressTagKeyword = true;
        
        std::string canonicalFieldTypeName = cxxToTyplibName("/" + qualType.getAsString(p));


        const Typelib::Type *typelibFieldType = checkRegisterType(canonicalFieldTypeName, qualType.getTypePtr(), decl->getASTContext());
        if(!typelibFieldType)
        {
            std::cout << "Not registering type '" << canonicalTypeName << "' as as field type '" << canonicalFieldTypeName << "' could not be registerd " << std::endl;
            return false;
        }

        size_t fieldOffset = typeLayout.getFieldOffset(fit->getFieldIndex());
        
        if(fieldOffset % 8 != 0)
        {
            std::cout << "Warning, can not register field were the offset is not Byte Aligned '" << canonicalFieldTypeName << "'" << std::endl;
            return false;
        }
        
        fieldOffset /= 8;

        
        compound.addField(fit->getNameAsString(), *typelibFieldType, fieldOffset);
    }
    
    return true;
}

void TypelibBuilder::registerTypeDef(const clang::TypedefNameDecl* decl)
{
    std::cout << "Found Typedef '" << decl->getQualifiedNameAsString() << "'"
       << " for Canonical Type '"
       << clang::QualType::getAsString(decl->getUnderlyingType().getCanonicalType().split())
       << "'" << std::endl;
    
    std::string typeDefName = cxxToTyplibName(decl->getQualifiedNameAsString());
    std::string forCanonicalType = getTypelibNameForQualType(decl->getUnderlyingType().getCanonicalType());

    if(!Typelib::isValidTypename(typeDefName, true))
    {
        std::cout << "Warning, ignoring typedef for '" << typeDefName << "'" << std::endl;
        return;
    }
    
    if(checkRegisterType(forCanonicalType, decl->getUnderlyingType().getTypePtr(), decl->getASTContext()))
        registry.alias(forCanonicalType, typeDefName);    
}


void TypelibBuilder::registerTypeDef(const clang::TypedefType* type)
{
    registerTypeDef(type->getDecl());
}

bool TypelibBuilder::loadRegistry(const std::string& filename)
{
    TlbImport importer;
    importer.load(filename, utilmm::config_set(), registry);
    return true;
}


