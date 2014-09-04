#include "TypelibBuilder.hpp"
#include "NamingConversions.hpp"
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
#include <lang/tlb/export.hh>
#include <clang/AST/Comment.h>
#include <llvm/Support/Casting.h>

void TypelibBuilder::registerTypeDecl(const clang::TypeDecl* decl)
{

    if(decl->getKind() == clang::Decl::Typedef)
    {
        registerTypedefNameDecl(llvm::dyn_cast<clang::TypedefDecl>(decl));
        return;
    }
    
    const clang::Type *typeForDecl = decl->getTypeForDecl();
    if(!typeForDecl)
    {
        std::cout << "TypeDecl '" << decl->getQualifiedNameAsString() << "' has no type " << std::endl;
        return;
    }

    //check for structs that are only defined inside of functions
    if(decl->getParentFunctionOrMethod())
    {
        std::cout << "Ignoring type '" << decl->getQualifiedNameAsString() << "' as it is defined inside a function" << std::endl;
        return;
    }
    
    if(decl->isHidden())
    {
        std::cout << "Ignoring hidden type '" << decl->getQualifiedNameAsString() << "' because it is hidden" << std::endl;
        return;
    }
    
    if(decl->isInAnonymousNamespace())
    {
        std::cout << "Ignoring '" << decl->getQualifiedNameAsString() << "' as it is in an anonymous namespace" << std::endl;
        return;
    }

    registerType(cxxToTyplibName(decl), typeForDecl,
                 decl->getASTContext());
}

bool TypelibBuilder::checkRegisterContainer(const std::string& canonicalTypeName, const clang::CXXRecordDecl* decl)
{
    
    const clang::NamedDecl *underlyingDecl = decl->getUnderlyingDecl();
    
    
    // skip non-template specializations
    if (!underlyingDecl ||
        (underlyingDecl->getKind() != clang::Decl::ClassTemplateSpecialization))
        return false;

    // some things of later use
    const clang::ClassTemplateSpecializationDecl *sdecl =
        llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl);
    const clang::TemplateArgumentList &argumentList(sdecl->getTemplateArgs());

    std::cout << canonicalTypeName <<  " is possibly a Container " << std::endl;
    
    
    std::cout << "Underlying name " << decl->getUnderlyingDecl()->getQualifiedNameAsString() << std::endl;;

    const Typelib::Container::AvailableContainers& containers = Typelib::Container::availableContainers();

    std::string containerName = cxxToTyplibName(underlyingDecl);

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
            
            std::string argTypelibName = cxxToTyplibName(arg.getAsType().getCanonicalType());
            
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
            
            std::string originalTypeName = cxxToTyplibName(arg.getAsType().getCanonicalType());
            
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
            
            std::cout << "Arg is '" << cxxToTyplibName(arg.getAsType()) << "'" << std::endl;
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

void TypelibBuilder::registerOpaque(const clang::TypeDecl* decl)
{

    // get a typelib-name for the given opaque-decl
    std::string opaqueName = cxxToTyplibName(decl);

    // registry can be pre-filled with all the opaque-type names loaded from a
    // tlb-databse given to the tool
    Typelib::Type * opaqueType = registry.get_(opaqueName);

    if (!opaqueType) {
        std::cout << "Opaque '" << decl->getQualifiedNameAsString()
                  << "' of kind '" << decl->getDeclKindName()
                  << "' not found in registry\n";
        return;
    }

    std::cout << "Opaque '" << decl->getQualifiedNameAsString()
              << "' of kind '" << decl->getDeclKindName()
              << "' is in database as '" << opaqueName << "'\n";

    // note the bases for inherited classes...
    setMetaDataBaseClasses(decl, opaqueType);

    // also note the filename for the decl in the metadata
    setMetaDataSourceFileLine(decl, opaqueType);

    // and special care if this is a typedef: we have to note an alias from the
    // given opaque to the actual type.
    if (decl->getKind() == clang::Decl::Typedef) {

        // behold, clang-API magic at work!
        const clang::QualType &actualType =
            llvm::dyn_cast<clang::TypedefNameDecl>(decl)
                ->getUnderlyingType()
                .getCanonicalType();

        // convert the type of the decl into a typelib-string
        std::string canonicalOpaqueName = cxxToTyplibName(actualType);

        // typedef-opaques are specially marked in the Typelib metadata
        opaqueType->getMetaData().add("opaque_is_typedef", "1");

        registry.alias(opaqueName, canonicalOpaqueName);

        std::cout << "Opaque '" << decl->getQualifiedNameAsString()
                  << "' is a typedef of '" << actualType.getAsString()
                  << "', alias created to '" << canonicalOpaqueName << "'\n";
    }
}

bool TypelibBuilder::registerBuiltIn(const std::string &canonicalTypeName,
                                     const clang::BuiltinType *builtin,
                                     clang::ASTContext &context) {

    // check if the to-be-registered type is already present
    if(registry.has(canonicalTypeName, false))
        return true;
    
    // typelib expects sizes in bytes, while clang can return them in bits.
    // should be of no concern for us, but adding a check anyways for extra
    // portability.
    uint64_t typeSizeInBits = context.getTypeSize(builtin);
    if (typeSizeInBits % 8 != 0) {
        std::cout << "Error, cannot register builtin '" << canonicalTypeName
                  << "' whose size " << typeSizeInBits
                  << " is not byte-aligned\n";
        return false;
    }
    size_t typeSizeInBytes = typeSizeInBits / 8;
    
    // pointer to hold the new TypeLib::Type
    Typelib::Numeric *newNumeric = NULL;

    if (builtin->isFloatingPoint()) {

        newNumeric = new Typelib::Numeric(canonicalTypeName, typeSizeInBytes,
                                          Typelib::Numeric::Float);

    } else if (builtin->isSignedInteger()) {

        // attention fold, here we rename "char" to "int8" automatically and
        // create the appropriate...
        if (canonicalTypeName == "/char") {
            newNumeric = new Typelib::Numeric("/int8_t", typeSizeInBytes,
                                              Typelib::Numeric::SInt);
            registry.add(newNumeric);
            registry.alias(newNumeric->getName(), canonicalTypeName);
            return true;
        } else
            newNumeric = new Typelib::Numeric(
                canonicalTypeName, typeSizeInBytes, Typelib::Numeric::SInt);

    } else if (builtin->isUnsignedInteger()) {

        // attention fold, here we rename "char" to "uint8" automatically and
        // create the appropriate...
        if (canonicalTypeName == "/char") {
            newNumeric = new Typelib::Numeric("/uint8_t", typeSizeInBytes,
                                              Typelib::Numeric::UInt);
            registry.add(newNumeric);
            registry.alias(newNumeric->getName(), canonicalTypeName);
            return true;
        } else
            newNumeric = new Typelib::Numeric(
                canonicalTypeName, typeSizeInBytes, Typelib::Numeric::UInt);

    }

    // only if a numeric was detected and an object created, we can add smth to
    // the database
    if (newNumeric) {
        registry.add(newNumeric);
        return true;
    }

    // otherwise smth went wrong...
    return false;
}

bool TypelibBuilder::registerType(const std::string &canonicalTypeName,
                                  const clang::Type *type,
                                  clang::ASTContext &context) {

    if(type->isReferenceType())
    {
        std::cout << "Ignoring type with reference '" << canonicalTypeName << "'\n";
        return false;
    }
    
    if(type->isAnyPointerType())
    {
        std::cout << "Ignoring pointer type '" << canonicalTypeName << "'\n";
        return false;
    }

    switch(type->getTypeClass())
    {
        case clang::Type::Builtin:
        {
            return registerBuiltIn(canonicalTypeName, llvm::dyn_cast<clang::BuiltinType>(type), context);
        }
        case clang::Type::Record:
        {
            return addRecord(canonicalTypeName, type->getAsCXXRecordDecl());
        }
        case clang::Type::Enum:
        {
            const clang::EnumType *enumType = llvm::dyn_cast<clang::EnumType>(type);
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
            const clang::ElaboratedType* etype = llvm::dyn_cast<clang::ElaboratedType>(type);

            // hm, this type is somehow strange, I have no idea, what it
            // actually is...
            //
            // quote from the clang doku: "The type itself is always "sugar",
            // used to express what was written in the source code but
            // containing no additional semantic information"
            return registerType(canonicalTypeName, etype->getNamedType().getTypePtr(), context);
        }
        default:
            std::cout << "Cannot register '" << canonicalTypeName << "'"
                      << " with unhandled type '" << type->getTypeClassName()
                      << "'" << std::endl;
            return false;
    }
}

const Typelib::Type *
TypelibBuilder::checkRegisterType(const std::string &canonicalTypeName,
                                  const clang::Type *type,
                                  clang::ASTContext &context) {
    if(!registry.has(canonicalTypeName, false))
    {
        std::cout << "Trying to register Type '" << canonicalTypeName
                  << "' which is unknown to the database" << std::endl;

        // what is this? why return NULL? makes it more complicated
        // downstream...
        if(!registerType(canonicalTypeName, type, context)) {
            return NULL;
        }
    }

    const Typelib::Type *typelibType = registry.get(canonicalTypeName);

    if(!typelibType)
    {
        std::cout << "Internal error : Just registed Type '"
                  << canonicalTypeName << "' was not found in registry"
                  << std::endl;
        exit(EXIT_FAILURE);
    }

    return typelibType;
}


bool TypelibBuilder::addArray(const std::string& canonicalTypeName, const clang::Type *gtype, clang::ASTContext& context)
{
    const clang::ConstantArrayType *type = llvm::dyn_cast<clang::ConstantArrayType>(gtype);
    const clang::Type *arrayBaseType = type->getElementType().getTypePtr();
    std::string arrayBaseTypeName = cxxToTyplibName(type->getElementType());

    const Typelib::Type *typelibArrayBaseType =
        checkRegisterType(arrayBaseTypeName, arrayBaseType, context);
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
    setMetaDataSourceFileLine(decl, enumVal);

    if(!decl->getIdentifier())
    {
        std::cout << "Ignoring type '" << canonicalTypeName
                  << "' without proper identifier" << std::endl;
        return false;
    }
    
    for(clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin(); it != decl->enumerator_end(); it++)
    {
        enumVal->add(it->getDeclName().getAsString(), it->getInitVal().getSExtValue());
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

    setMetaDataSourceFileLine(decl, compound);
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
        std::cout << "Ignoring Compound '" << canonicalTypeName << "' as it has no fields " << std::endl;
        return false;
    }
    
    if (registry.get(compound->getName()))
        return false;

    registry.add(compound);
    
    return true;
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

        std::string canonicalFieldTypeName = cxxToTyplibName(qualType);

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

void TypelibBuilder::registerTypedefNameDecl(const clang::TypedefNameDecl* decl)
{
    std::cout << "Found Typedef '" << decl->getQualifiedNameAsString() << "'"
       << " of '"
       << decl->getUnderlyingType().getCanonicalType().getAsString()
       << "'\n";
    
    std::string typeDefName = cxxToTyplibName(decl);
    std::string forCanonicalType = cxxToTyplibName(decl->getUnderlyingType().getCanonicalType());

    if(!Typelib::isValidTypename(typeDefName, true))
    {
        std::cout << "Warning, ignoring typedef for '" << typeDefName << "'" << std::endl;
        return;
    }
    
    if(checkRegisterType(forCanonicalType, decl->getUnderlyingType().getTypePtr(), decl->getASTContext()))
        registry.alias(forCanonicalType, typeDefName);    
}

void TypelibBuilder::setMetaDataSourceFileLine(const clang::Decl *decl,
                                               Typelib::Type *type) {
    const clang::SourceManager &sm = decl->getASTContext().getSourceManager();
    const clang::SourceLocation &loc =
        sm.getSpellingLoc(decl->getSourceRange().getBegin());

    // typelib needs the '/path/to/file:column' information
    std::ostringstream stream;
    stream << sm.getFilename(loc).str() << ":" << sm.getSpellingLineNumber(loc);
    type->setPathToDefiningHeader(stream.str());

    type->getMetaData().add("source_file_line",
                            type->getPathToDefiningHeader());
}

void TypelibBuilder::setMetaDataBaseClasses(const clang::Decl *decl,
                                            Typelib::Type *type) {
    // we are also required to note all base-classes of the decl in the
    // metadata
    if (const clang::CXXRecordDecl *cxxRecord =
            llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
        clang::CXXRecordDecl::base_class_const_iterator base;
        for (base = cxxRecord->bases_begin(); base != cxxRecord->bases_end();
             base++) {
            const clang::QualType &qualType = base->getType();

            type->getMetaData().add("base_classes", cxxToTyplibName(qualType));
        }
    }
}

void TypelibBuilder::setMetaDataDoc(const clang::Decl *decl,
                                    Typelib::Type *type) {

    clang::comments::FullComment *comment =
        decl->getASTContext().getCommentForDecl(decl, NULL);

    if (!comment)
        return;

    std::ostringstream stream;

    clang::ArrayRef<clang::comments::BlockContentComment *>::const_iterator i;
    for (i = comment->getBlocks().begin(); i != comment->getBlocks().end();
         i++) {

        const clang::comments::ParagraphComment *p =
            llvm::dyn_cast<clang::comments::ParagraphComment>((*i));

        clang::comments::ParagraphComment::child_iterator c;

        for (c = p->child_begin(); c != p->child_end(); c++) {
            if (const clang::comments::TextComment *TC =
                    llvm::dyn_cast<clang::comments::TextComment>(*c)) {
                stream << TC->getText().str() << "\n";
            }
        }
    }

    type->getMetaData().add("doc", stream.str());
}

void TypelibBuilder::loadRegistry(const std::string& filename)
{
    TlbImport importer;
    importer.load(filename, utilmm::config_set(), registry);
}

void TypelibBuilder::saveRegistry(const std::string& filename)
{
    TlbExport exporter;
    exporter.save(filename, utilmm::config_set(), registry);
}

