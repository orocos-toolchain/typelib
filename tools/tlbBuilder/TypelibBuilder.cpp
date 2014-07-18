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
    std::string typeName = decl->getQualifiedNameAsString();
    
    std::cout << "New Type with qualified name " << typeName << std::endl;
    
    if(typeMap.find(typeName) != typeMap.end())
    {
        //type allready known return;
        return;
    }

    
    const clang::Type *typeForDecl = decl->getTypeForDecl();
    if(!typeForDecl)
    {
        std::cout << "TypeDecl " << decl->getNameAsString() << " has no type " << std::endl;
        return;
    }
    
    std::cout << "TypeClassName " << typeForDecl->getTypeClassName() << std::endl;;
    std::cout << "DeclName " << decl->getNameAsString() << std::endl;;
    
    switch(typeForDecl->getTypeClass())
    {
        case clang::Type::Record:
        {
            const clang::RecordDecl *recordDecl = dynamic_cast<const clang::RecordDecl *>(decl);
            if(recordDecl->isInjectedClassName())
                return;
            if(recordDecl->isDependentType() || recordDecl->isInvalidDecl())
                return;
            
            const clang::CXXRecordDecl* cxxdecl = dynamic_cast<const clang::CXXRecordDecl *>(decl);
            if(cxxdecl)
            {
                addRecord(typeName, cxxdecl);
            }
        }
            break;
        case clang::Type::Enum:
        {
            const clang::EnumDecl *enumDecl = dynamic_cast<const clang::EnumDecl *>(decl);
            assert(enumDecl);
            addEnum(typeName, enumDecl);
        }
            break;
        default:
            std::cout << "Found unknown type " << typeName << std::endl;
            break;
    }    
}

void TypelibBuilder::registerBuildIn(const clang::BuiltinType* builtin, clang::ASTContext& context)
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
    
}


void TypelibBuilder::registerType(const clang::Type* type, clang::ASTContext& context)
{
    if(type->getTypeClass() == clang::Type::Builtin)
    {
        
        const clang::BuiltinType *builtin = static_cast<const clang::BuiltinType *>(type);
        
        registerBuildIn(builtin, context);
    }

}

void TypelibBuilder::addEnum(const std::string& canonicalTypeName, const clang::EnumDecl *decl)
{
    Typelib::Enum *enumVal =new Typelib::Enum("/" + decl->getQualifiedNameAsString());
    
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
        std::cout << "Ignoring Type " << decl->getQualifiedNameAsString() << " as it is polymorphic" << std::endl;
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
        const clang::QualType qualType = fit->getType().getCanonicalType();
        clang::SplitQualType T_split = qualType.split();
        
        std::string canonicalFieldTypeName = clang::QualType::getAsString(T_split);

        if(!registry.has(canonicalTypeName))
        {
            fit->getDeclContext()->getDeclKind();
            
            const clang::Type *type = qualType.getTypePtr();
            
            //TODO make it work
            registerType(type, decl->getASTContext());
        }
        
        const Typelib::Type *typelibFieldType = registry.get(canonicalFieldTypeName);
        compound->addField(fit->getNameAsString(), *typelibFieldType, typeLayout.getFieldOffset(fit->getFieldIndex()));
        
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

