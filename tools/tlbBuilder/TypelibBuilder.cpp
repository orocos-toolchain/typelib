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
    
    std::cout << "New Type with qualified name " << typeName << std::endl;

    typeName = cxxToTyplibName(typeName);
    
    std::cout << "Typelib name " << typeName << std::endl;
    
    if(typeName.size() <= 1)
        return;
    
    if(registry.has(typeName))
        return;


    
    const clang::Type *typeForDecl = decl->getTypeForDecl();
    if(!typeForDecl)
    {
        std::cout << "TypeDecl " << decl->getNameAsString() << " has no type " << std::endl;
        return;
    }

    //check for structs that are only defined inside of functions
    if(decl->getParentFunctionOrMethod())
    {
        std::cout << "Ignoring type " << typeName << " as it is defined inside a function" << std::endl;
        return;
    }
    
    if(decl->isHidden())
    {
        std::cout << "Ignoring hidden type " << typeName << std::endl;
        return;
    }
    
    
//     if(typeForDecl->isCanonicalUnqualified())
//     {
//         std::cout << "Ignoring " << typeName << " as it is not canonical identifiable" << std::endl;
//         return;
//     }
    
    if(decl->isInAnonymousNamespace())
    {
        std::cout << "Ignoring " << typeName << " as it is in an anonymous namespace" << std::endl;
        return;
    }
    
//     std::cout << "TypeClassName " << typeForDecl->getTypeClassName() << std::endl;;
//     std::cout << "DeclName " << decl->getNameAsString() << std::endl;;
    

    
    registerType(typeName, typeForDecl, decl->getASTContext());
}

void TypelibBuilder::registerBuildIn(const std::string& canonicalTypeName, const clang::BuiltinType* builtin, clang::ASTContext& context)
{
    
    std::string typeName = std::string("/") + builtin->getNameAsCString(clang::PrintingPolicy(clang::LangOptions()));
    
    if(registry.has(typeName))
        return;
    
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
        std::cout << "Start pos " << start_pos << " char at + 1 " << ret.at(start_pos + 1) << std::endl;
        
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
        std::cout << "Start pos " << start_pos << " char at + 2 " << ret.at(start_pos + 2) << std::endl;
        
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
        std::cout << "Ignoring anonymous type " << canonicalTypeName << std::endl;
        return false;
    }
    
    if(canonicalTypeName.find("&") != std::string::npos)
    {
        std::cout << "Ignoring type with reference " << canonicalTypeName << std::endl;
        return false;
    }
    
    if(canonicalTypeName.find("sizeof") != std::string::npos)
    {
        std::cout << "Ignoring type with weired sizeof " << canonicalTypeName << std::endl;
        return false;
    }

    if(canonicalTypeName.find("(") != std::string::npos)
    {
        std::cout << "Ignoring type with function pointer " << canonicalTypeName << std::endl;
        return false;
    }
    switch(type->getTypeClass())
    {
        case clang::Type::Builtin:
        {
            const clang::BuiltinType *builtin = static_cast<const clang::BuiltinType *>(type);
            
            registerBuildIn(canonicalTypeName, builtin, context);
        }
        break;
        case clang::Type::Record:
        {
            const clang::CXXRecordDecl *recordDecl = type->getAsCXXRecordDecl();
            if(!recordDecl)
                return false;

            if(!recordDecl->hasDefinition())
            {
                std::cout << "Ignoring type " << canonicalTypeName << " as it has no definition " << std::endl;
                return false;
            }
            
            std::string typeName = "/" + recordDecl->getCanonicalDecl()->getQualifiedNameAsString();
            typeName = cxxToTyplibName(typeName);
            
            //we need to add one check here, as the type that was given for registration
            //could have been an injected type. In this case the canonicalFieldTypeName name 
            //could be e.g. Pose::Pose, while the typename is Pose
            if(registry.has(typeName))
            {
                return true;
            }
            
            if(recordDecl->isInjectedClassName())
            {
                std::cout << "Is injected class name " << std::endl;
                return false;
            }
            
            if(recordDecl->isDependentType() || recordDecl->isInvalidDecl())
                return false;
            
            if(recordDecl->isAnonymousStructOrUnion())
            {
                return false;
            }
            
            addRecord(typeName, recordDecl);
        }
        break;
        case clang::Type::Enum:
        {
            const clang::EnumType *enumType = static_cast<const clang::EnumType *>(type);
            const clang::EnumDecl *enumDecl = enumType->getDecl();
            assert(enumDecl);
            addEnum(canonicalTypeName, enumDecl);
        }
            break;
        default:
            std::cout << "Found unknown type " << canonicalTypeName << std::endl;
            break;
    }
    return true;
    
}

void TypelibBuilder::addEnum(const std::string& canonicalTypeName, const clang::EnumDecl *decl)
{
    Typelib::Enum *enumVal =new Typelib::Enum(canonicalTypeName);
    
    for(clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin(); it != decl->enumerator_end(); it++)
    {
        enumVal->add(it->getDeclName().getAsString(), it->getInitVal().getSExtValue());
        std::cout << "Enum CONST " << it->getDeclName().getAsString() << " Value " << it->getInitVal().getSExtValue() << std::endl;
    }
    
    registry.add(enumVal);
}

void TypelibBuilder::addRecord(const std::string &canonicalTypeName, const clang::CXXRecordDecl *decl)
{
    if(!decl)
    {
        std::cerr << "Warning, got NULL Type" << std::endl;
        return;
    }

    if(decl->isPolymorphic() || decl->isAbstract())
    {
        std::cout << "Ignoring Type " << canonicalTypeName << " as it is polymorphic" << std::endl;
        return;
    }
    
    if(decl->isDependentType() || decl->isInvalidDecl())
    {
        //ignore incomplete / forward declared types
        return;
    }
    
    if(!decl->getDefinition())
    {
        //ignore incomplete / forward declared types
        return;
    }
    
    const clang::ASTRecordLayout &typeLayout(decl->getASTContext().getASTRecordLayout(decl));

    
    Typelib::Compound *compound = new Typelib::Compound(canonicalTypeName);
    compound->setSize(typeLayout.getSize().getQuantity());
    
//     TemporaryType tempType;
//     tempType.typeName = canonicalTypeName;
//     tempType.sizeInBytes = typeLayout.getSize().getQuantity();

    for(clang::RecordDecl::field_iterator fit = decl->field_begin(); fit != decl->field_end(); fit++)
    {
//         TemporaryFieldType fieldType;
        const clang::QualType qualType = fit->getType().getLocalUnqualifiedType().getCanonicalType();
        clang::SplitQualType T_split = qualType.split();
        
        
        
        if(fit->isAnonymousStructOrUnion())
        {
            std::cout << "Warning, ignoring Record with Anonymous Struct or Union " << canonicalTypeName << std::endl;
            delete compound;
            return;
        }
        
        
        clang::LangOptions o;
        clang::PrintingPolicy p(o);
        p.SuppressTagKeyword = true;
        
        std::string canonicalFieldTypeName = "/" + qualType.getAsString(p);
        
        
//         std::cout << "Parent of " << canonicalFieldTypeName << " is " << fit->getParent()->getQualifiedNameAsString() << std::endl;

        canonicalFieldTypeName = cxxToTyplibName(canonicalFieldTypeName);

        if(!registry.has(canonicalFieldTypeName))
        {
//             fit->getDeclContext()->getDeclKind();
            std::cout << "Found field with non registered Type " << canonicalFieldTypeName << " registering it" << std::endl;
            
            const clang::Type *type = qualType.getTypePtr();
            registerType(canonicalFieldTypeName, type, decl->getASTContext());
        }
        
        const Typelib::Type *typelibFieldType = registry.get(canonicalFieldTypeName);
        if(!typelibFieldType)
        {
            std::cout << "Error, type of field is not known " << canonicalFieldTypeName << std::endl;
            return;
        }
            
        compound->addField(fit->getNameAsString(), *typelibFieldType, typeLayout.getFieldOffset(fit->getFieldIndex()) / 8);
        
//         fieldType.fieldName = fit->getNameAsString();
//         fieldType.offsetInBits = ;

//         std::cout << std::endl << "Qual Type :" <<std::endl;
//         qualType.dump();
//         std::cout << std::endl << "Type : " << std::endl<< std::endl;
//         const clang::Type *type = qualType.getTypePtr();
//         
//         std::cout << type->getTypeClassName();
//         
//         if(type->getTypeClass() == clang::Type::Builtin)
//         {
//             
//             const clang::BuiltinType *builtin = reinterpret_cast<const clang::BuiltinType *>(type);
//             
//             registerBuildIn(builtin, decl->getASTContext());
// 
// //             switch(builtin->getKind())
// //             {
// //                 case clang::BuiltinType::Int:
// //                 {
// //                     std::cout << "Got a Int" <<std::endl;
// //                     std::cout << " Int has a size of " << decl->getASTContext().getTypeSize(qualType) << std::endl;
// //                     decl->getASTContext().getInt128Decl();
// //                 }
// //                 break;
// //             }
//             
//         }
//         
//         
//         tempType.fields.push_back(fieldType);
    }

/*    //put type into map
    typeMap[canonicalTypeName] = tempType;
    
    
    std::cout << "Type " << tempType.typeName <<std::endl;
    std::cout << "  Size" << tempType.sizeInBytes << std::endl;

    if(!tempType.fields.empty())
    {
        std::cout << "  Members :" << std::endl;
    
        for(std::vector<TemporaryFieldType>::const_iterator it = tempType.fields.begin(); it != tempType.fields.end(); it++)
        {
            std::cout << "    FieldName " << it->fieldName << std::endl;
            std::cout << "    TypeName  " << it->typeName << std::endl;
            std::cout << "    Offset    " << it->offsetInBits << std::endl;
            std::cout << std::endl;
        }
    } */   

    registry.add(compound);
}


void TypelibBuilder::registerTypeDef(const clang::TypedefType* type)
{
    std::cout << "Found Typedef" << type->getDecl()->getQualifiedNameAsString() << " for Canonical Type " 
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

