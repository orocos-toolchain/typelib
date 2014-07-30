#include "TypelibBuilder.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclLookups.h"
#include "clang/AST/Type.h"
#include <iostream>
#include <typelib/registry.hh>
#include <typelib/typemodel.hh>



void TypelibBuilder::registerNamedDecl(const clang::TypeDecl* decl)
{
    //check if we allready know the type
    std::string typeName = "/" + decl->getQualifiedNameAsString();
    
    std::cerr << "New Type with qualified name " << typeName << std::endl;

    typeName = cxxToTyplibName(typeName);
    
    std::cerr << "Typelib name " << typeName << std::endl;
    
    if(typeName.size() <= 1)
        return;
    
    if(registry.has(typeName))
        return;


    
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
    
    
//     if(typeForDecl->isCanonicalUnqualified())
//     {
//         std::cerr << "Ignoring " << typeName << " as it is not canonical identifiable" << std::endl;
//         return;
//     }
    
    if(decl->isInAnonymousNamespace())
    {
        std::cerr << "Ignoring " << typeName << " as it is in an anonymous namespace" << std::endl;
        return;
    }
    
//     std::cerr << "TypeClassName " << typeForDecl->getTypeClassName() << std::endl;;
//     std::cerr << "DeclName " << decl->getNameAsString() << std::endl;;
    

    
    registerType(typeName, typeForDecl, decl->getASTContext());
}

bool TypelibBuilder::registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context)
{
    
    std::string typeName = std::string("/") + builtin->getNameAsCString(clang::PrintingPolicy(clang::LangOptions()));
    
    if(registry.has(typeName))
        return true;
    
    Typelib::Numeric *newNumeric = 0;
    
    if(builtin->isFloatingPoint())
    {
        newNumeric = new Typelib::Numeric(typeName, context.getTypeSize(builtin->desugar()), Typelib::Numeric::Float);
    }
    
    if(builtin->isInteger())
    {
        if(builtin->isSignedInteger())
        {
            newNumeric =new Typelib::Numeric(typeName, context.getTypeSize(builtin->desugar()), Typelib::Numeric::SInt);
        }
        else
        {
            newNumeric =new Typelib::Numeric(typeName, context.getTypeSize(builtin->desugar()), Typelib::Numeric::UInt);
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
//         default:
//             std::cerr << "Found unknown type " << canonicalTypeName << std::endl;
//             break;
    }
    return true;
const Typelib::Type* TypelibBuilder::checkRegisterType(const std::string& canonicalTypeName, const clang::Type *type, clang::ASTContext& context)
{
    if(!registry.has(canonicalTypeName))
    {
        std::cerr << "Trying to register unknown Type " << canonicalTypeName << std::endl;
        
        if(!registerType(canonicalTypeName, type, context))
        {
            return NULL;
        }
    }
    
    const Typelib::Type *typelibType = registry.get(canonicalTypeName);

    if(!typelibType)
    {
        std::cerr << "Internal error : Just registed Type " + canonicalTypeName +  " was not found in registry" << std::endl;
//         throw std::runtime_error("Just registed Type " + canonicalTypeName +  " was not found in registry" );
    }

    return typelibType;
}
    
}

bool TypelibBuilder::addEnum(const std::string& canonicalTypeName, const clang::EnumDecl *decl)
{
    Typelib::Enum *enumVal =new Typelib::Enum(canonicalTypeName);
    
    for(clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin(); it != decl->enumerator_end(); it++)
    {
        enumVal->add(it->getDeclName().getAsString(), it->getInitVal().getSExtValue());
        std::cerr << "Enum CONST " << it->getDeclName().getAsString() << " Value " << it->getInitVal().getSExtValue() << std::endl;
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
    compound->setSize(typeLayout.getSize().getQuantity());

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
    
    std::cerr << "Registered Compound " << canonicalTypeName << std::endl;

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
            
        compound.addField(fit->getNameAsString(), *typelibFieldType, typeLayout.getFieldOffset(fit->getFieldIndex()));
    }
    
    return true;
}


void TypelibBuilder::registerTypeDef(const clang::TypedefType* type)
{
    std::cerr << "Found Typedef " << type->getDecl()->getQualifiedNameAsString() << " for Canonical Type "
    << clang::QualType::getAsString(type->getDecl()->getUnderlyingType().getCanonicalType().split()) << std::endl << std::endl;
    
    std::string typeDefName = type->getDecl()->getQualifiedNameAsString();
    std::string forCanonicalType = clang::QualType::getAsString(type->getDecl()->getUnderlyingType().getCanonicalType().split());
    
    ;
    
//     registry.alias(forCanonicalType, typeDefName);
    
}

void TypelibBuilder::buildRegistry()
{
    registry.dump(std::cout);
    
    for(TypeMap::const_iterator it = typeMap.begin(); it != typeMap.end(); it++)
    {
        const std::string &curTypeName(it->first);
        
        if(!registry.has(curTypeName))
        {
            //add the type
            
            
            
        }
        
        
    }
    
}

