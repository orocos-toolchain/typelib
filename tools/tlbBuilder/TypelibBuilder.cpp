#include "TypelibBuilder.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/Type.h"
#include <iostream>
#include <typelib/registry.hh>
#include <typelib/typemodel.hh>
#include <typelib/typename.hh>
#include <lang/tlb/import.hh>



void TypelibBuilder::registerNamedDecl(const clang::TypeDecl* decl)
{
    //check if we allready know the type
    std::string typeName = "/" + decl->getQualifiedNameAsString();
    
//     std::cerr << "New Type with qualified name " << typeName << std::endl;

    typeName = cxxToTyplibName(typeName);
    
//     std::cerr << "Typelib name " << typeName << std::endl;
    
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
        std::cerr << "TypeDecl " << decl->getNameAsString() << " has no type " << std::endl;
        return;
    }

    //check for structs that are only defined inside of functions
    if(decl->getParentFunctionOrMethod())
    {
        std::cerr << "Ignoring type " << typeName << " as it is defined inside a function" << std::endl;
        return;
    }
    
    if(decl->isHidden())
    {
        std::cerr << "Ignoring hidden type " << typeName << std::endl;
        return;
    }
    
    if(decl->isInAnonymousNamespace())
    {
        std::cerr << "Ignoring " << typeName << " as it is in an anonymous namespace" << std::endl;
        return;
    }    
    
    registerType(typeName, typeForDecl, decl->getASTContext());
}

void TypelibBuilder::lookupOpaque(const clang::TypeDecl* decl)
{
    std::cout << "Qualified Name " << decl->getQualifiedNameAsString() << std::endl;

    std::string opaqueName = cxxToTyplibName(decl->getQualifiedNameAsString());
    std::string canoniclaOpaqueName;
    
    if(decl->getKind() == clang::Decl::Typedef)
    {
        const clang::TypedefDecl *typeDefDecl = static_cast<const clang::TypedefDecl *>(decl);
        canoniclaOpaqueName = getTypelibNameForQualType(typeDefDecl->getUnderlyingType().getCanonicalType());
    }
    else
    {
        if(!decl->getTypeForDecl())
        {
            std::cout << "Error, could not get Type for Opaque Declaration" << decl->getQualifiedNameAsString() << std::endl;
            exit(0);
        }
        
        canoniclaOpaqueName = getTypelibNameForQualType(decl->getTypeForDecl()->getCanonicalTypeInternal());
        
    }
        
    std::cout << "Opaque name is " << opaqueName << " canonicalName is " << canoniclaOpaqueName << std::endl;
    
    if(opaqueName != canoniclaOpaqueName)
    {
        //as we want to resolve opaques by their canonical name
        //we need to register an alias from the canonical name 
        //to the opaque name.
        registry.alias(opaqueName, canoniclaOpaqueName);
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
        std::cout << "Warning, can not register type which is not Byte Aligned " << canonicalTypeName << std::endl;
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
            newNumeric =new Typelib::Numeric(typeName, typeSize, Typelib::Numeric::SInt);
        }
        else
        {
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
    
    if(canonicalTypeName.find("anonymous") != std::string::npos)
    {
        std::cerr << "Ignoring anonymous type " << canonicalTypeName << std::endl;
        return false;
    }
    
    if(canonicalTypeName.find("&") != std::string::npos)
    {
        std::cerr << "Ignoring type with reference " << canonicalTypeName << std::endl;
        return false;
    }
    
    if(canonicalTypeName.find("sizeof") != std::string::npos)
    {
        std::cerr << "Ignoring type with weired sizeof " << canonicalTypeName << std::endl;
        return false;
    }

    if(canonicalTypeName.find("(") != std::string::npos)
    {
        std::cerr << "Ignoring type with function pointer " << canonicalTypeName << std::endl;
        return false;
    }
    
    
    switch(type->getTypeClass())
    {
        case clang::Type::Builtin:
        {
            const clang::BuiltinType *builtin = static_cast<const clang::BuiltinType *>(type);
            
            return registerBuildIn(canonicalTypeName, builtin, context);
        }
        break;
        case clang::Type::Record:
        {
            return addRecord(canonicalTypeName, type->getAsCXXRecordDecl());
        }
        break;
        case clang::Type::Enum:
        {
            const clang::EnumType *enumType = static_cast<const clang::EnumType *>(type);
            const clang::EnumDecl *enumDecl = enumType->getDecl();
            assert(enumDecl);
            return addEnum(canonicalTypeName, enumDecl);
        }
            break;
        case clang::Type::ConstantArray:
        {            
            return addArray(canonicalTypeName, type, context);
        }
        default:
            std::cerr << "Trying to register '" << canonicalTypeName
                      << "' with unhandled type '" << type->getTypeClassName() << "'\n";

    }
    return false;
    
}

const Typelib::Type* TypelibBuilder::checkRegisterType(const std::string& canonicalTypeName, const clang::Type *type, clang::ASTContext& context)
{
    if(!registry.has(canonicalTypeName, false))
    {
        std::cerr << "Trying to register unknown Type " << canonicalTypeName << std::endl;
        
        if(!registerType(canonicalTypeName, type, context))
        {
            return NULL;
        }
    }
    else
    {
//         std::cerr << "registry claims to know " << canonicalTypeName << std::endl;
    }
    
    
    const Typelib::Type *typelibType = registry.get(canonicalTypeName);

    if(!typelibType)
    {
        std::cerr << "Internal error : Just registed Type " + canonicalTypeName +  " was not found in registry" << std::endl;
        exit(0);
//         throw std::runtime_error("Just registed Type " + canonicalTypeName +  " was not found in registry" );
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
        std::cerr << "Not registering Array " << canonicalTypeName << " as its elementary type " << arrayBaseTypeName << " could not be registered " << std::endl;
        return false;
    }
    
    Typelib::Array *array = new Typelib::Array(*typelibArrayBaseType, type->getSize().getZExtValue());

    registry.add(array);
    
    return true;
}


bool TypelibBuilder::addEnum(const std::string& canonicalTypeName, const clang::EnumDecl *decl)
{
    Typelib::Enum *enumVal =new Typelib::Enum(canonicalTypeName);
    
    for(clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin(); it != decl->enumerator_end(); it++)
    {
        enumVal->add(it->getDeclName().getAsString(), it->getInitVal().getSExtValue());
//         std::cerr << "Enum CONST " << it->getDeclName().getAsString() << " Value " << it->getInitVal().getSExtValue() << std::endl;
    }
    
    registry.add(enumVal);
    
    return true;
}

bool TypelibBuilder::addRecord(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl)
{
    if(!decl)
    {
        std::cerr << "Warning, got NULL Type" << std::endl;
        return false;
    }

    if(!decl->hasDefinition())
    {
        std::cerr << "Ignoring type " << canonicalTypeName << " as it has no definition " << std::endl;
        return false;
    }
    
    if(decl->isInjectedClassName())
    {
        std::cerr << "Ignoring Type " << canonicalTypeName << " as it is injected" << std::endl;
        return false;
    }
    
    if(decl->isPolymorphic() || decl->isAbstract())
    {
        std::cerr << "Ignoring Type " << canonicalTypeName << " as it is polymorphic" << std::endl;
        return false;
    }
    
    if(decl->isDependentType() || decl->isInvalidDecl())
    {
        std::cerr << "Ignoring Type " << canonicalTypeName << " as it is dependents / Invalid " << std::endl;
        //ignore incomplete / forward declared types
        return false;
    }
    
    const clang::ASTRecordLayout &typeLayout(decl->getASTContext().getASTRecordLayout(decl));

    
    Typelib::Compound *compound = new Typelib::Compound(canonicalTypeName);
    
    size_t typeSize = typeLayout.getSize().getQuantity();
    compound->setSize(typeSize);

    for(clang::CXXRecordDecl::base_class_const_iterator it = decl->bases_begin(); it != decl->bases_end(); it++)
    {
        const clang::CXXRecordDecl* curDecl = it->getType()->getAsCXXRecordDecl();
        if(!addFieldsToCompound(*compound, canonicalTypeName, curDecl))
        {
            delete compound;
            return false;
        }
        
    }

    if(!addFieldsToCompound(*compound, canonicalTypeName, decl))
    {
        delete compound;
        return false;
    }

    if(compound->getFields().empty())
    {
        std::cerr << "Ignoring Type " << canonicalTypeName << " as it has no fields " << std::endl;
        return false;
    }
    
//     std::cerr << "Registered Compound " << canonicalTypeName << std::endl;

    registry.add(compound);
    
    return true;
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
        
        if(fit->isAnonymousStructOrUnion())
        {
            std::cerr << "Warning, ignoring Record with Anonymous Struct or Union " << canonicalTypeName << std::endl;
            return false;
        }
        
        
        clang::LangOptions o;
        clang::PrintingPolicy p(o);
        p.SuppressTagKeyword = true;
        
        std::string canonicalFieldTypeName = "/" + qualType.getAsString(p);
        
        
//         std::cerr << "Parent of " << canonicalFieldTypeName << " is " << fit->getParent()->getQualifiedNameAsString() << std::endl;

        canonicalFieldTypeName = cxxToTyplibName(canonicalFieldTypeName);

        const Typelib::Type *typelibFieldType = checkRegisterType(canonicalFieldTypeName, qualType.getTypePtr(), decl->getASTContext());
        if(!typelibFieldType)
        {
            std::cerr << "Not regstering type " << canonicalTypeName << " as as field type " << canonicalFieldTypeName << " could not be registerd " << std::endl;
            return false;
        }

        size_t fieldOffset = typeLayout.getFieldOffset(fit->getFieldIndex());
        
        if(fieldOffset % 8 != 0)
        {
            std::cout << "Warning, can not register field were the offset is not Byte Aligned " << canonicalFieldTypeName << std::endl;
            return false;
        }
        
        fieldOffset /= 8;

        
        compound.addField(fit->getNameAsString(), *typelibFieldType, fieldOffset);
    }
    
    return true;
}

void TypelibBuilder::registerTypeDef(const clang::TypedefNameDecl* decl)
{
    std::cerr << "Found Typedef " << decl->getQualifiedNameAsString() << " for Canonical Type "
    << clang::QualType::getAsString(decl->getUnderlyingType().getCanonicalType().split()) << std::endl << std::endl;
    
    std::string typeDefName = cxxToTyplibName(decl->getQualifiedNameAsString());
    std::string forCanonicalType = getTypelibNameForQualType(decl->getUnderlyingType().getCanonicalType());

    if(!Typelib::isValidTypename(typeDefName, true))
    {
        std::cerr << "Warning, ignoring typedef for " << typeDefName << std::endl;
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


