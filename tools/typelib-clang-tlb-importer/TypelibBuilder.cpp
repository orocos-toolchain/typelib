#include "TypelibBuilder.hpp"
#include "NamingConversions.hpp"
#include "IgnoredOrRenamedTypes.hpp"
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

Typelib::Type const *
TypelibBuilder::registerTypeDecl(const clang::TypeDecl *decl) {

    // this removes the bogus double-match of "nested_records::S2::S2"
    // for "struct nested_records::S2". it it not needed by us.
    if (const clang::CXXRecordDecl *rDecl =
            llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {
        if (rDecl->isInjectedClassName())
            return NULL;
    }

    if(decl->getKind() == clang::Decl::Typedef)
    {
        return registerTypedefNameDecl(llvm::dyn_cast<clang::TypedefDecl>(decl));
    }
    
    const clang::Type *typeForDecl = decl->getTypeForDecl();
    if(!typeForDecl)
    {
        std::cout << "TypeDecl '" << decl->getQualifiedNameAsString() << "' has no type " << std::endl;
        return NULL;
    }

    //check for structs that are only defined inside of functions
    if(decl->getParentFunctionOrMethod())
    {
        std::cout << "Ignoring type '" << decl->getQualifiedNameAsString() << "' as it is defined inside a function" << std::endl;
        return NULL;
    }
    
    if(decl->isHidden())
    {
        std::cout << "Ignoring hidden type '" << decl->getQualifiedNameAsString() << "' because it is hidden" << std::endl;
        return NULL;
    }
    
    if(decl->isInAnonymousNamespace())
    {
        std::cout << "Ignoring '" << decl->getQualifiedNameAsString() << "' as it is in an anonymous namespace" << std::endl;
        return NULL;
    }

    return registerType(cxxToTypelibName(decl), typeForDecl,
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
    // template, the cxxToTypelibName() whould convert and append the template
    // as well.
    if (decl->getDescribedClassTemplate()) {
        std::cout << "\n\n";
        decl->getDescribedClassTemplate()->dump();
        std::cout << "\n\n";
    }
    // this is problematic...
    std::string containerName = cxxToTypelibName(decl->getQualifiedNameAsString());

    // the "std::string" is actually a typedef to "std::basic_string", but
    // mapped to "/std/string" in typelib
    std::pair<bool, std::string> retval = IgnoredOrRenamedType::isTypeRenamed(containerName);
    if (retval.first) {
        /* std::cout << "Renaming '" << containerName << "' to '" << retval.second << "'\n"; */
        containerName = retval.second;
    }

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
        
        std::string argTypelibName = cxxToTypelibName(arg.getAsType().getCanonicalType());
        
        // ignore certain types
        if(IgnoredOrRenamedType::isTemplateArgumentIgnored(argTypelibName)) {
            std::cout << "Container '" << containerName << "' has TemplateArgument '"
                      << argTypelibName << "' which is ignored.\n";
            continue;
        }

        // very very special handling... I cannot believe this is needed...
        if ((containerName.find("/std/string") == 0) &&
            argTypelibName != "/char") {
            std::cout
                << "Warning: Ignoring '" << containerName
                << "' that is not of "
                   "character-type '/char' but of '" << argTypelibName
                << "' and cannot be handled by the typelib-string-container.\n";
            return NULL;
        }

        // carefull: is this "ASTContext" correct?
        Typelib::Type const* argType = registerType(argTypelibName, typePtr, decl->getASTContext());

        if(!argType) {
            std::cout << "Creating new Container '" << containerName
                      << "' failed because arg '" << argTypelibName
                      << "' could not be registered\n";
            return NULL;
        }

        std::cout << "Container '" << containerName
                  << "' has template-argument '" << argTypelibName
                  << "' which was successfully registered in database\n";

        typelibArgList.push_back(argType);
    }
    
    
    Typelib::Container::ContainerFactory factory = contIt->second;
    Typelib::Container const& newContainer = factory(registry, typelibArgList);
    
    // the new container _can_ have a different name as the canonicalTypeName,
    // if "/std/allocator" for example is ignored. but do we need an alias for
    // this case? no typelib-code should ever be able to "know" the correct
    // cxx-name... anyhow...
    if(newContainer.getName() != canonicalTypeName) {

        std::cerr << "Name of Container '" << newContainer.getName()
                  << "' is different from its canonicalTypeName '"
                  << canonicalTypeName << "', adding alias\n";
        registry.alias(newContainer.getName(), canonicalTypeName);
    }

    std::cout << "Container '" << containerName
              << "' successfully registered for Type '" << canonicalTypeName
              << "'\n";

    // FIXME: and, as usual: all the metadata should not be missing? but we
    // only get a const-reference from the container-factory... so ask the
    // registry to get the type-pointer of the actual type... meh...
    setMetaDataDoc(decl, registry.get_(canonicalTypeName));
    setMetaDataSourceFileLine(decl, registry.get_(canonicalTypeName));

    return &newContainer;
}

void TypelibBuilder::registerOpaque(const clang::TypeDecl* decl)
{

    // get a typelib-name for the given opaque-decl
    //
    // NOTE: there is special trickery in place inside cxxToTypelibName():
    // given a template specialization, it will automagically add the
    // template parameters from the decl to the returned typelib-name.
    std::string opaqueName = cxxToTypelibName(decl);

    // registry can be pre-filled with all the opaque-type names loaded from a
    // tlb-databse given to the tool.
    //
    // need to use the "get_()" variant to receive a non-const version for
    // changing of meta-data later...
    Typelib::Type *opaqueType = registry.get_(opaqueName);

    if (!opaqueType) {
        std::cout << "Opaque '" << decl->getQualifiedNameAsString()
                  << "' of kind '" << decl->getDeclKindName()
                  << "' with presumed opaque-name '" << opaqueName
                  << "' is not found in registry\n";
        return;
    }

    std::cout << "Opaque '" << decl->getQualifiedNameAsString()
              << "' of kind '" << decl->getDeclKindName()
              << "' found in registry as '" << opaqueName << "'\n";

    // add the bases for inherited classes...
    setMetaDataBaseClasses(decl, opaqueType);
    // and the file-location for the decl in the metadata
    setMetaDataSourceFileLine(decl, opaqueType);
    setMetaDataDoc(decl, opaqueType);

    // and special care if this is a typedef: we have to note an alias from the
    // given opaque to the actual type.
    if (decl->getKind() == clang::Decl::Typedef) {

        // behold, clang-API magic at work!
        const clang::QualType &actualType =
            llvm::dyn_cast<clang::TypedefNameDecl>(decl)
                ->getUnderlyingType()
                .getCanonicalType();

        // convert the type of the decl into a typelib-string
        std::string canonicalOpaqueName = cxxToTypelibName(actualType);

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
        // attention folks, here we change the given canonicalTypeName (like
        // "char") to a name based on the size and signedness (like "uint8_t")
        // automatically. for sizes like 16 or 64, the automatically generated
        // string should be the same as the canonicalTypeName. later, there is
        // an alias added to the registry

        std::string numericTypelibName =
            "/int" + boost::lexical_cast<std::string>(typeSizeInBytes * 8) +
            "_t";
        newNumeric = new Typelib::Numeric(numericTypelibName, typeSizeInBytes,
                                          Typelib::Numeric::SInt);

    } else if (builtin->isUnsignedInteger()) {

        std::string numericTypelibName =
            "/uint" + boost::lexical_cast<std::string>(typeSizeInBytes * 8) +
            "_t";
        newNumeric = new Typelib::Numeric(numericTypelibName, typeSizeInBytes,
                                          Typelib::Numeric::UInt);

    }

    // only if a numeric was detected and an object created, we can add smth to
    // the database
    if (newNumeric) {
        // if this numeric is not there already
        if (!registry.get(newNumeric->getName())) {
            registry.add(newNumeric);
            // needed if we added a "char" which was automatically added as
            // "uint8_t", to still find the char. does not hurt if not needed.
            registry.alias(newNumeric->getName(), canonicalTypeName);
            return newNumeric;
            // otherwise we at least try to create an alias for the existing
            // numeric using our canonicalTypeName
        } else {
            const Typelib::Type *existing_numeric =
                registry.get(newNumeric->getName());
            registry.alias(existing_numeric->getName(), canonicalTypeName);
            return existing_numeric;
        }
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
        case clang::Type::TemplateSpecialization:
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
    std::string arrayElementTypeName = cxxToTypelibName(arrayElementType);
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

    if(!decl->getIdentifier())
    {
        std::cout << "Ignoring enum '" << canonicalTypeName
                  << "' without proper identifier" << std::endl;
        return NULL;
    }
    
    Typelib::Enum *enumVal = new Typelib::Enum(canonicalTypeName);

    for (clang::EnumDecl::enumerator_iterator it = decl->enumerator_begin();
         it != decl->enumerator_end(); it++) {
        enumVal->add(it->getDeclName().getAsString(),
                     it->getInitVal().getSExtValue());
    }

    setMetaDataSourceFileLine(decl, enumVal);
    setMetaDataDoc(decl, enumVal);

    registry.add(enumVal);
    
    return enumVal;
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
        std::cout << "Ignoring record '" << canonicalTypeName
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

    if(!addMembersOfClassToCompound(*compound, canonicalTypeName, decl))
    {
        delete compound;
        return NULL;
    }

    if(compound->getFields().empty())
    {
        std::cout << "Ignoring Compound '" << canonicalTypeName << "' as it has no fields " << std::endl;
        delete compound;
        return NULL;
    }

    setMetaDataSourceFileLine(decl, compound);
    setMetaDataDoc(decl, compound);
    setMetaDataBaseClasses(decl, compound);
    
    registry.add(compound);
    
    return compound;
}

bool TypelibBuilder::addMembersOfClassToCompound(
    Typelib::Compound &compound, const std::string &canonicalTypeName,
    const clang::CXXRecordDecl *decl) {

    // this will be used for later querying of the byte-offset
    const clang::ASTRecordLayout &typeLayout(
        decl->getASTContext().getASTRecordLayout(decl));

    for (clang::RecordDecl::field_iterator fit = decl->field_begin();
         fit != decl->field_end(); fit++) {

        const clang::QualType qualType =
            fit->getType().getLocalUnqualifiedType().getCanonicalType();

        if (fit->isAnonymousStructOrUnion()) {
            std::cout
                << "Warning, cannot add field '" << cxxToTypelibName(qualType)
                << "' to record '" << canonicalTypeName
                << "' because the field is an Anonymous Struct or Union\n";
            return false;
        }

        // FIXME: what about testing for private-ness of inheritance of member-variable?

        std::string canonicalFieldTypeName = cxxToTypelibName(qualType);

        // creating of getting the Typelib::Type
        const Typelib::Type *typelibFieldType =
            registerType(canonicalFieldTypeName, qualType.getTypePtr(),
                         decl->getASTContext());

        if (!typelibFieldType) {
            std::cout << "Field type '" << canonicalFieldTypeName
                      << "' of compound '" << canonicalTypeName
                      << "' could not be registered in the database.\n";
            return false;
        }

        size_t fieldOffset = typeLayout.getFieldOffset(fit->getFieldIndex());

        if (fieldOffset % 8 != 0) {
            std::cout << "Warning, can not register field '"
                      << canonicalFieldTypeName << "' of record '"
                      << canonicalTypeName
                      << "' because the offset is not Byte Aligned but "
                      << fieldOffset << "\n";
            return false;
        }

        fieldOffset /= 8;

        // meta-data is not added here, but should be added in the
        // "registerType" for this thing
        compound.addField(fit->getNameAsString(), *typelibFieldType,
                          fieldOffset);
    }
    
    // and then call this function here recursively add bases -- so that member
    // variables defined as part of a base-class are added to the compound as well
    for (clang::CXXRecordDecl::base_class_const_iterator it =
             decl->bases_begin();
         it != decl->bases_end(); it++) {
        // this is the decl of one of the base-classes
        const clang::CXXRecordDecl *curDecl =
            it->getType()->getAsCXXRecordDecl();

        if (!addMembersOfClassToCompound(compound, canonicalTypeName, curDecl)) {
            return false;
        }
    }

    // if we reach here everything worked fine and we got ourselfs a ready-made
    // Typelib::Compound for further use
    return true;
}

Typelib::Type const *
TypelibBuilder::registerTypedefNameDecl(const clang::TypedefNameDecl *decl) {

    std::string typeDefName = cxxToTypelibName(decl);
    std::string forCanonicalType = cxxToTypelibName(decl->getUnderlyingType().getCanonicalType());

    if (!Typelib::isValidTypename(typeDefName, true)) {
        std::cout << "Warning, ignoring typedef for '" << typeDefName
                  << "' as it seems to be no valid Typelib name\n";
        return NULL;
    }

    if (Typelib::Type const *type = registerType(
            forCanonicalType, decl->getUnderlyingType().getTypePtr(),
            decl->getASTContext())) {

        std::cout << "Found Typedef '" << typeDefName << "'"
                  << " of '"
                  << forCanonicalType << "', created alias.\n";
    
        registry.alias(forCanonicalType, typeDefName);    

        return type;
    }

    return NULL;
}

void TypelibBuilder::setMetaDataSourceFileLine(const clang::Decl *decl,
                                               Typelib::Type *type) {
    const clang::SourceManager &sm = decl->getASTContext().getSourceManager();
    const clang::SourceLocation &loc = sm.getExpansionLoc(decl->getLocStart());

    // typelib needs the '/path/to/file:column' information
    std::ostringstream stream;
    stream << sm.getFilename(loc).str() << ":" << sm.getExpansionLineNumber(loc);
    type->setPathToDefiningHeader(stream.str());

    type->getMetaData().add("source_file_line",
                            type->getPathToDefiningHeader());
}

void TypelibBuilder::setMetaDataOrogenInclude(const clang::Decl *decl,
                                              Typelib::Type *type) {
    const clang::SourceManager &sm = decl->getASTContext().getSourceManager();
    const clang::SourceLocation &loc = sm.getSpellingLoc(decl->getLocStart());
    // add a header from the include-chain as "orogen_include" entry to the
    // metadata. this leads to possibly adding the main-header itself if the
    // type is defined there, which would be mostly the same as
    // "source_file_line". or it is the first non-main-header if the type is
    // defined somewhere else.
    /* std::cout << "-- the file '" << sm.getFilename(loc).str() << "' is " */
    /*           << (sm.isInSystemHeader(loc) ? "a" : "no") << " system header\n"; */
    clang::SourceLocation incLoc = loc;
    clang::SourceLocation lastValidLoc = loc;
    while (incLoc.isValid() && sm.isInSystemHeader(incLoc)) {
        lastValidLoc = incLoc;
        incLoc = sm.getIncludeLoc(sm.getFileID(incLoc));
        /* std::cout << "-- the file '" << sm.getFilename(loc).str() */
        /*           << "' includes '" << sm.getFilename(incLoc).str() */
        /*           << "' which is " << (sm.isInSystemHeader(incLoc) ? "a" : "no") */
        /*           << " system header\n"; */
    }

    if (lastValidLoc.isValid())
        type->getMetaData().add("orogen_include",
                                sm.getFilename(lastValidLoc).str());
    else
        std::cout << "setMetaDataOrogenInclude() Warning: could not find "
                     "suitable include-file for '" << type->getName() << "'\n";
}

void TypelibBuilder::setMetaDataBaseClasses(const clang::Decl *decl,
                                            Typelib::Type *type) {
    // we are also required to note all base-classes of the decl in the
    // metadata
    if (const clang::CXXRecordDecl *cxxRecord =
            llvm::dyn_cast<clang::CXXRecordDecl>(decl)) {

        // TODO hm, there is something in the clang-api broken. need this
        // additional test because some things don't have a definition, but
        // still think they are able to access internal undefined pointers via
        // "bases_begin()" for example -- which would segfault.
        //
        // maybe templates-declaration which have no definition...?
        //
        // only fails for "/base/geometry/Spline<1>" of base/orogen/types
        // because there is no ostream-operator defined in
        // "base/geometry/Spline.hpp" -- but one for Spline<3>
        if (!cxxRecord->hasDefinition()) {
            return;
        }
        clang::CXXRecordDecl::base_class_const_iterator base;
        for (base = cxxRecord->bases_begin(); base != cxxRecord->bases_end();
             base++) {
            const clang::QualType &qualType = base->getType();

            type->getMetaData().add("base_classes", cxxToTypelibName(qualType));
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

            // save us very empty comment blocks
            if (p->isWhitespace())
                break;

            clang::comments::ParagraphComment::child_iterator c = p->child_begin();;
            while (c != p->child_end()) {
                const clang::comments::TextComment *TC =
                    llvm::dyn_cast<clang::comments::TextComment>(*c);
                if (TC) {

                    stream << TC->getText().str();
                }
                c++;
                // only do a new line if it is not the last TextComment in the
                // block
                if (c != p->child_end() && TC) {
                    stream << "\n";
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
