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
    // this removes the bogus double-match of "nested_records::S2::S2"
    // for "struct nested_records::S2". it it not needed by us.
    if (const clang::CXXRecordDecl *rDecl =
            llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
        if (rDecl->isInjectedClassName())
            return;
    }

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

Typelib::Type const*
TypelibBuilder::checkRegisterContainer(const std::string &canonicalTypeName,
                                       const clang::CXXRecordDecl *decl) {

    // skip non-template specializations... because... the two current
    // "Container" implementation are both based on template specializations...
    // and why not hardcode an assumption for all times which is valid today?
    // what can possibly go wrong?
    //
    // this would have to be moved to the end. missing cases: typedefs
    if (!decl ||
        (decl->getKind() != clang::Decl::ClassTemplateSpecialization)) {
        return NULL;
    }

    // the name of the Container -- basically we need a translation of the
    // "Namespace::Class" part of the record-name. If the decl would point to a
    // template, the cxxToTyplibName() whould convert and append the template
    // as well.
    if (decl->getDescribedClassTemplate()) {
        std::cout << "\n\n";
        decl->getDescribedClassTemplate()->dump();
        std::cout << "\n\n";
    }
    // this is problematic...
    std::string containerName = cxxToTyplibName(decl->getQualifiedNameAsString());

    // needed to search for a specific container in all known ones.
    const Typelib::Container::AvailableContainers &containers =
        Typelib::Container::availableContainers();

    Typelib::Container::AvailableContainers::const_iterator contIt = containers.find(containerName);
    if(contIt == containers.end()) {
        std::cout << "No Container named '" << containerName << "' for '"
                  << canonicalTypeName << "' known to typelib\n";
        return NULL;
    }

    // hurray!
    std::cout << "Found Container '" << containerName
              << "' for type '" << canonicalTypeName << "', trying to add Args to registry:\n";

    // some shortcuts for later use
    const clang::ClassTemplateSpecializationDecl *sdecl =
        llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(decl);
    const clang::TemplateArgumentList &argumentList(sdecl->getTemplateArgs());

    // create argument list
    std::list<Typelib::Type const*> typelibArgList;
    for(size_t i = 0; i < argumentList.size(); i++)
    {
        const clang::TemplateArgument& arg = argumentList.get(i);
        const clang::Type *typePtr = arg.getAsType().getTypePtr();
        if(!typePtr)
        {
            std::cout << "Error, argument has not type" << std::endl;
            return NULL;
        }
        
        std::string argTypelibName = cxxToTyplibName(arg.getAsType().getCanonicalType());
        
        //HACK ignore allocators
#warning HACK: ignoring templateArgument whose name begins '/std/allocator' to support Typelib::Container
        if(argTypelibName.find("/std/allocator") == 0) {
            std::cout << "Container '" << containerName << "' has TemplateArgument '"
                      << argTypelibName << "' which is ignored.\n";
            continue;
        }

#warning HACK: ignoring templateArgument whose name begins '/std/char_traits' to support Typelib::Container
        if(argTypelibName.find("/std/char_traits") == 0){
            std::cout << "Container '" << containerName << "' has TemplateArgument '"
                      << argTypelibName << "' which is ignored.\n";
            continue;
        }
        
        Typelib::Type const* argType = registerType(argTypelibName, typePtr, decl->getASTContext());

        if(!argType) {
            std::cout << "Creating new Container '" << containerName
                      << "' failed because arg '" << argTypelibName
                      << "' could not be registered\n";
            return NULL;
        }
        
        // on parole... very very special handling
        if(containerName == "/std/string" && argTypelibName != "/char")
        {
            std::cout << "Warning: Ignoring string that is not of "
                         "character-type char but '" << argTypelibName << "'\n";
            delete argType;
            return NULL;
        }
        
        std::cout << "Container '" << canonicalTypeName << "' has arg '"
                  << argTypelibName << "'" << std::endl;

        typelibArgList.push_back(argType);
    }
    
    
    Typelib::Container::ContainerFactory factory = contIt->second;
    Typelib::Container const& newContainer = factory(registry, typelibArgList);
    
    // the new container _can_ have a different name as the canonicalTypeName,
    // if "/std/allocator" for example is ignored. but do we need an alias for
    // this case? no typelib-code should ever be able to "know" the correct
    // cxx-name... anyhow...
    if(newContainer.getName() != canonicalTypeName) {

        std::cerr << "Name of Typelib::Container '" << newContainer.getName()
                  << "' is different from canonicalTypeName '"
                  << canonicalTypeName << "', adding alias\n";
        registry.alias(newContainer.getName(), canonicalTypeName);
    }

    std::cout << "Type '" << canonicalTypeName
              << "' successfully registered as Container for '" << containerName
              << "'\n";

    return &newContainer;
}

void TypelibBuilder::registerOpaque(const clang::TypeDecl* decl)
{

    // get a typelib-name for the given opaque-decl
    std::string opaqueName = cxxToTyplibName(decl);

    // registry can be pre-filled with all the opaque-type names loaded from a
    // tlb-databse given to the tool.
    //
    // need to use the "get_()" variant to receive a non-const version for
    // changing of meta-data later...
    Typelib::Type *opaqueType = registry.get_(opaqueName);

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

        clang::LangOptions o;
        clang::PrintingPolicy suppressTagKeyword(o);
        suppressTagKeyword.SuppressTagKeyword = true;

        std::cout << "Opaque '" << decl->getQualifiedNameAsString()
                  << "' is a typedef of '"
                  << actualType.getAsString(suppressTagKeyword)
                  << "', alias created to '" << canonicalOpaqueName << "'\n";
    }
}

Typelib::Type const *
TypelibBuilder::registerBuiltIn(const std::string &canonicalTypeName,
                                const clang::BuiltinType *builtin,
                                clang::ASTContext &context) {

    // check if the to-be-registered type is already present
    if (Typelib::Type const* type = registry.get(canonicalTypeName)) {
        return type;
    }
    
    // typelib expects sizes in bytes, while clang can return them in bits.
    // should be of no concern for us, but adding a check anyways for extra
    // portability.
    uint64_t typeSizeInBits = context.getTypeSize(builtin);
    if (typeSizeInBits % 8 != 0) {
        std::cout << "Error, cannot register builtin '" << canonicalTypeName
                  << "' whose size " << typeSizeInBits
                  << " is not byte-aligned\n";
        return NULL;
    }
    size_t typeSizeInBytes = typeSizeInBits / 8;
    
    // pointer to hold the new TypeLib::Type
    Typelib::Numeric *newNumeric = NULL;

    if (builtin->isFloatingPoint()) {

        newNumeric = new Typelib::Numeric(canonicalTypeName, typeSizeInBytes,
                                          Typelib::Numeric::Float);

    } else if (builtin->isSignedInteger()) {

        // attention folks, here we rename "char" to "int8" automatically and
        // create the appropriate...
        if (canonicalTypeName == "/char") {
            newNumeric = new Typelib::Numeric("/int8_t", typeSizeInBytes,
                                              Typelib::Numeric::SInt);
            registry.add(newNumeric);
            registry.alias(newNumeric->getName(), canonicalTypeName);
            return registry.get(canonicalTypeName);
        } else
            newNumeric = new Typelib::Numeric(
                canonicalTypeName, typeSizeInBytes, Typelib::Numeric::SInt);

    } else if (builtin->isUnsignedInteger()) {

        // attention folks, here we rename "char" to "uint8" automatically and
        // create the appropriate...
        if (canonicalTypeName == "/char") {
            newNumeric = new Typelib::Numeric("/uint8_t", typeSizeInBytes,
                                              Typelib::Numeric::UInt);
            registry.add(newNumeric);
            registry.alias(newNumeric->getName(), canonicalTypeName);
            return registry.get(canonicalTypeName);
        } else
            newNumeric = new Typelib::Numeric(
                canonicalTypeName, typeSizeInBytes, Typelib::Numeric::UInt);

    }

    // only if a numeric was detected and an object created, we can add smth to
    // the database
    if (newNumeric) {
        registry.add(newNumeric);
        return newNumeric;
    }

    // otherwise smth went wrong...
    return NULL;
}

Typelib::Type const *
TypelibBuilder::registerType(const std::string &canonicalTypeName,
                             const clang::Type *type,
                             clang::ASTContext &context) {

    // shortcut: if the requested name is already present in the database, we
    // don't have to do anything.
    if (Typelib::Type const* type = registry.get(canonicalTypeName)) {
        return type;
    }

    if(type->isReferenceType())
    {
        std::cout << "Ignoring type with reference '" << canonicalTypeName << "'\n";
        return NULL;
    }
    
    if(type->isAnyPointerType())
    {
        std::cout << "Ignoring pointer type '" << canonicalTypeName << "'\n";
        return NULL;
    }

    // FIXME: handling of container-detection should be done here?

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
            return addEnum(canonicalTypeName, llvm::dyn_cast<clang::EnumType>(type)->getDecl());
        }
        case clang::Type::ConstantArray:
        {            
            return addArray(canonicalTypeName, llvm::dyn_cast<clang::ConstantArrayType>(type), context);
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
            return NULL;
    }
}

Typelib::Type const *
TypelibBuilder::addArray(const std::string &canonicalTypeName,
                         const clang::ConstantArrayType *arrayType,
                         clang::ASTContext &context) {

    const clang::QualType arrayElementType = arrayType->getElementType();
    // before we can add the array in question we first have to check wether
    // the element type of the array is addable. so at first get the
    // typelib-name of the array elements...
    std::string arrayElementTypeName = cxxToTyplibName(arrayElementType);
    // ...then try to add it to the database
    Typelib::Type const *typelibArrayElementType = registerType(
        arrayElementTypeName, arrayElementType.getTypePtr(), context);

    if (!typelibArrayElementType) {
        std::cout << "Not registering Array '" << canonicalTypeName
                  << "' as its elementary type '" << arrayElementTypeName
                  << "' could not be registered.\n";
        return NULL;
    }

    // giving the number of elements as second argument. does "zero extension"
    // of the internal number.
    Typelib::Array *array = new Typelib::Array(
        *typelibArrayElementType, arrayType->getSize().getZExtValue());

    registry.add(array);

    return array;
}

Typelib::Type const *
TypelibBuilder::addEnum(const std::string &canonicalTypeName,
                        const clang::EnumDecl *decl) {

    Typelib::Enum *enumVal = new Typelib::Enum(canonicalTypeName);
    setMetaDataSourceFileLine(decl, enumVal);

    if(!decl->getIdentifier())
    {
        std::cout << "Ignoring enum '" << canonicalTypeName
                  << "' without proper identifier" << std::endl;
        return NULL;
    }
    
    for(clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin(); it != decl->enumerator_end(); it++)
    {
        enumVal->add(it->getDeclName().getAsString(), it->getInitVal().getSExtValue());
    }
    
    registry.add(enumVal);
    
    return enumVal;
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

Typelib::Type const *
TypelibBuilder::addRecord(const std::string &canonicalTypeName,
                          const clang::CXXRecordDecl *decl) {

    // check if smth named like this is already there
    if (Typelib::Type const* type = registry.get(canonicalTypeName)) {
        return type;
    }

    if(!decl)
    {
        std::cout << "Warning, got NULL Type" << std::endl;
        return NULL;
    }

    if(!decl->getIdentifier())
    {
        std::cout << "Ignoring type '" << canonicalTypeName
                  << "' without proper identifier" << std::endl;
        return NULL;
    }

    if(!decl->hasDefinition())
    {
        std::cout << "Ignoring type '" << canonicalTypeName << "' as it has no definition " << std::endl;
        return NULL;
    }
    
    if(decl->isInjectedClassName())
    {
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it is injected" << std::endl;
        return NULL;
    }
    
    // this check is on parole, because there might be "Typelib::Container" who
    // can be added despite these features...?
    if(decl->isPolymorphic() || decl->isAbstract())
    {
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it is polymorphic" << std::endl;
        return NULL;
    }
    
    if(decl->isDependentType() || decl->isInvalidDecl())
    {
        //ignore incomplete / forward declared types
        std::cout << "Ignoring Type '" << canonicalTypeName << "' as it is dependents / Invalid " << std::endl;
        return NULL;
    }

    if(Typelib::Type const* type = checkRegisterContainer(canonicalTypeName, decl)) {
        return type;
    }
    
    Typelib::Compound *compound = new Typelib::Compound(canonicalTypeName);

    const clang::ASTRecordLayout &typeLayout(decl->getASTContext().getASTRecordLayout(decl));
    size_t typeSizeInBytes = typeLayout.getSize().getQuantity();
    compound->setSize(typeSizeInBytes);

    setMetaDataSourceFileLine(decl, compound);
    if(!addBaseClassToCompound(*compound, canonicalTypeName, decl))
    {
        delete compound;
        return NULL;
    }

    if(!addFieldsToCompound(*compound, canonicalTypeName, decl))
    {
        delete compound;
        return NULL;
    }

    if(compound->getFields().empty())
    {
        std::cout << "Ignoring Compound '" << canonicalTypeName << "' as it has no fields " << std::endl;
        return NULL;
    }
    
    registry.add(compound);
    
    return compound;
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
                << "Warning, cannot add field '" << cxxToTyplibName(qualType)
                << "' to record '" << canonicalTypeName
                << "' because the field is an Anonymous Struct or Union\n";
            return false;
        }

        std::string canonicalFieldTypeName = cxxToTyplibName(qualType);

        const Typelib::Type *typelibFieldType =
            registerType(canonicalFieldTypeName, qualType.getTypePtr(),
                         decl->getASTContext());
        if(!typelibFieldType)
        {
            std::cout << "Not registering type '" << canonicalTypeName
                      << "' as as field type '" << canonicalFieldTypeName
                      << "' could not be registered " << std::endl;
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
    clang::LangOptions o;
    clang::PrintingPolicy suppressTagKeyword(o);
    suppressTagKeyword.SuppressTagKeyword = true;

    std::cout << "Found Typedef '" << decl->getQualifiedNameAsString() << "'"
              << " of '"
              << decl->getUnderlyingType().getCanonicalType().getAsString(
                     suppressTagKeyword) << "'\n";

    std::string typeDefName = cxxToTyplibName(decl);
    std::string forCanonicalType = cxxToTyplibName(decl->getUnderlyingType().getCanonicalType());

    if(!Typelib::isValidTypename(typeDefName, true))
    {
        std::cout << "Warning, ignoring typedef for '" << typeDefName << "'" << std::endl;
        return;
    }
    
    if(registerType(forCanonicalType, decl->getUnderlyingType().getTypePtr(), decl->getASTContext()))
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

    // if there is no comment the resulting task is simple...
    if (!comment)
        return;

    std::ostringstream stream;

    clang::ArrayRef<clang::comments::BlockContentComment *>::const_iterator i;
    for (i = comment->getBlocks().begin(); i != comment->getBlocks().end();
         i++) {

        switch ((*i)->getCommentKind()) {
        case clang::comments::Comment::ParagraphCommentKind: {
            const clang::comments::ParagraphComment *p =
                llvm::dyn_cast<clang::comments::ParagraphComment>((*i));

            clang::comments::ParagraphComment::child_iterator c;

            for (c = p->child_begin(); c != p->child_end(); c++) {
                if (const clang::comments::TextComment *TC =
                        llvm::dyn_cast<clang::comments::TextComment>(*c)) {
                    stream << TC->getText().str() << "\n";
                }
            }
            break;
        }
        case clang::comments::Comment::VerbatimLineCommentKind: {
            stream << llvm::dyn_cast<clang::comments::VerbatimLineComment>((*i))
                          ->getText()
                          .str();
            break;
        }
        default: {
            std::cout << "Non-handled comment type '"
                      << (*i)->getCommentKindName() << "' for Type '"
                      << type->getName() << "' located at '"
                      << (*i)->getLocation().printToString(
                             decl->getASTContext().getSourceManager()) << "'\n";
            return;
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

