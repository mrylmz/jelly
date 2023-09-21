#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/Diagnostic.h"

// TODO: Add specification for name mangling and refactor ASTMangling to match the specification
// TODO: Allocate mangled names inside of AST as uniqued identifiers to avoid memory leaks and reduce memory footprint

static inline void _MangleEnumerationDeclarationName(AllocatorRef allocator, ASTEnumerationDeclarationRef declaration);
static inline void _MangleEnumerationElementName(AllocatorRef allocator, ASTEnumerationDeclarationRef declaration,
                                                 ASTValueDeclarationRef element);
static inline void _MangleFunctionDeclarationName(AllocatorRef allocator, ASTFunctionDeclarationRef declaration);
static inline void _MangleStructureDeclarationName(AllocatorRef allocator, ASTStructureDeclarationRef declaration);
static inline void _MangleValueDeclarationName(AllocatorRef allocator, ASTValueDeclarationRef declaration);
static inline void _MangleInitializerDeclarationName(AllocatorRef allocator, ASTStructureDeclarationRef structure,
                                                     ASTInitializerDeclarationRef initializer);

static inline void _StringAppendMangledIdentifier(StringRef string, StringRef identifier);
static inline void _StringAppendMangledTypeName(StringRef string, ASTTypeRef type);

void PerformNameMangling(ASTContextRef context, ASTModuleDeclarationRef module) {
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            PerformNameManglingForDeclaration(context, (ASTDeclarationRef)child);
        }
    }
}

void PerformNameManglingForDeclaration(ASTContextRef context, ASTDeclarationRef declaration) {
    AllocatorRef tempAllocator = ASTContextGetTempAllocator(context);

    if (declaration->base.tag == ASTTagEnumerationDeclaration) {
        _MangleEnumerationDeclarationName(tempAllocator, (ASTEnumerationDeclarationRef)declaration);
        ASTArrayIteratorRef iterator = ASTArrayGetIterator(((ASTEnumerationDeclarationRef)declaration)->elements);
        while (iterator) {
            ASTValueDeclarationRef element = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(iterator);
            _MangleEnumerationElementName(tempAllocator, (ASTEnumerationDeclarationRef)declaration, element);
            iterator = ASTArrayIteratorNext(iterator);
        }
    }

    if (declaration->base.tag == ASTTagFunctionDeclaration || declaration->base.tag == ASTTagIntrinsicFunctionDeclaration ||
        declaration->base.tag == ASTTagForeignFunctionDeclaration) {
        _MangleFunctionDeclarationName(tempAllocator, (ASTFunctionDeclarationRef)declaration);
    }

    if (declaration->base.tag == ASTTagStructureDeclaration) {
        _MangleStructureDeclarationName(tempAllocator, (ASTStructureDeclarationRef)declaration);

        ASTArrayIteratorRef iterator = ASTArrayGetIterator(((ASTStructureDeclarationRef)declaration)->initializers);
        while (iterator) {
            ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)ASTArrayIteratorGetElement(iterator);
            _MangleInitializerDeclarationName(tempAllocator, (ASTStructureDeclarationRef)declaration, initializer);
            iterator = ASTArrayIteratorNext(iterator);
        }
    }

    if (declaration->base.tag == ASTTagValueDeclaration) {
        _MangleValueDeclarationName(tempAllocator, (ASTValueDeclarationRef)declaration);
    }

    if (declaration->base.tag == ASTTagInitializerDeclaration) {
        ASTInitializerDeclarationRef initializer = (ASTInitializerDeclarationRef)declaration;
        assert(initializer->structure);
        _MangleInitializerDeclarationName(tempAllocator, initializer->structure, (ASTInitializerDeclarationRef)declaration);
    }
}

static inline void _MangleEnumerationDeclarationName(AllocatorRef allocator, ASTEnumerationDeclarationRef declaration) {
    if (declaration->base.mangledName) {
        return;
    }

    StringRef mangledName = StringCreate(allocator, "$E");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleEnumerationElementName(AllocatorRef allocator, ASTEnumerationDeclarationRef declaration,
                                                 ASTValueDeclarationRef element) {
    if (element->base.mangledName) {
        return;
    }

    assert(declaration->base.mangledName);

    StringRef mangledName = StringCreate(allocator, StringGetCharacters(declaration->base.mangledName));
    StringAppend(mangledName, "_M");
    StringAppendFormat(mangledName, "%zu", StringGetLength(element->base.name));
    StringAppendString(mangledName, element->base.name);
    element->base.mangledName = mangledName;
}

static inline void _MangleFunctionDeclarationName(AllocatorRef allocator, ASTFunctionDeclarationRef declaration) {
    if (declaration->base.mangledName) {
        return;
    }

    StringRef mangledName = StringCreate(allocator, "$F");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);

    StringAppendFormat(mangledName, "%zu", ASTArrayGetElementCount(declaration->parameters));
    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        _StringAppendMangledTypeName(mangledName, value->base.base.type);
    }

    _StringAppendMangledTypeName(mangledName, declaration->returnType);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleStructureDeclarationName(AllocatorRef allocator, ASTStructureDeclarationRef declaration) {
    if (declaration->base.mangledName) {
        return;
    }

    StringRef mangledName = StringCreate(allocator, "$S");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleValueDeclarationName(AllocatorRef allocator, ASTValueDeclarationRef declaration) {
    if (declaration->base.mangledName) {
        return;
    }

    StringRef mangledName = StringCreate(allocator, "$V");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleInitializerDeclarationName(AllocatorRef allocator, ASTStructureDeclarationRef structure,
                                                     ASTInitializerDeclarationRef initializer) {
    if (initializer->base.mangledName) {
        return;
    }

    StringRef mangledName = StringCreate(allocator, "$I");
    _StringAppendMangledIdentifier(mangledName, structure->base.name);
    StringAppend(mangledName, "4init");

    StringAppendFormat(mangledName, "%zu", ASTArrayGetElementCount(initializer->parameters));
    for (Index index = 0; index < ASTArrayGetElementCount(initializer->parameters); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(initializer->parameters, index);
        _StringAppendMangledTypeName(mangledName, value->base.base.type);
    }

    initializer->base.mangledName = mangledName;
}

static inline void _StringAppendMangledIdentifier(StringRef string, StringRef identifier) {
    assert(string && identifier);
    StringAppendFormat(string, "%zu", StringGetLength(identifier));
    StringAppendString(string, identifier);
}

static inline void _StringAppendMangledTypeName(StringRef string, ASTTypeRef type) {
    assert(type && type->tag != ASTTagOpaqueType);

    switch (type->tag) {
    case ASTTagPointerType: {
        ASTPointerTypeRef pointerType = (ASTPointerTypeRef)type;
        StringAppend(string, "$p");
        _StringAppendMangledTypeName(string, pointerType->pointeeType);
        break;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef arrayType = (ASTArrayTypeRef)type;
        StringAppend(string, "$a");
        if (arrayType->size) {
            assert(arrayType->size->base.tag == ASTTagConstantExpression);
            ASTConstantExpressionRef size = (ASTConstantExpressionRef)arrayType->size;
            assert(size->kind == ASTConstantKindInt);
            StringAppendFormat(string, "%llu", size->intValue);
        } else {
            StringAppend(string, "?");
        }
        _StringAppendMangledTypeName(string, arrayType->elementType);
        break;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef builtinType = (ASTBuiltinTypeRef)type;
        StringAppend(string, "$b");
        switch (builtinType->kind) {
        case ASTBuiltinTypeKindVoid:
            StringAppend(string, "4Void");
            break;

        case ASTBuiltinTypeKindBool:
            StringAppend(string, "4Bool");
            break;

        case ASTBuiltinTypeKindInt8:
            StringAppend(string, "4Int8");
            break;

        case ASTBuiltinTypeKindInt16:
            StringAppend(string, "5Int16");
            break;

        case ASTBuiltinTypeKindInt32:
            StringAppend(string, "5Int32");
            break;

        case ASTBuiltinTypeKindInt64:
            StringAppend(string, "5Int64");
            break;

        case ASTBuiltinTypeKindInt:
            StringAppend(string, "3Int");
            break;

        case ASTBuiltinTypeKindUInt8:
            StringAppend(string, "5UInt8");
            break;

        case ASTBuiltinTypeKindUInt16:
            StringAppend(string, "6UInt16");
            break;

        case ASTBuiltinTypeKindUInt32:
            StringAppend(string, "6UInt32");
            break;

        case ASTBuiltinTypeKindUInt64:
            StringAppend(string, "6UInt64");
            break;

        case ASTBuiltinTypeKindUInt:
            StringAppend(string, "4UInt");
            break;

        case ASTBuiltinTypeKindFloat32:
            StringAppend(string, "7Float32");
            break;

        case ASTBuiltinTypeKindFloat64:
            StringAppend(string, "7Float64");
            break;

        case ASTBuiltinTypeKindFloat:
            StringAppend(string, "5Float");
            break;

        default:
            JELLY_UNREACHABLE("Invalid kind given for ASTBuiltinType!");
            break;
        }

        break;
    }

    case ASTTagEnumerationType: {
        ASTEnumerationTypeRef enumerationType = (ASTEnumerationTypeRef)type;
        assert(enumerationType->declaration);
        StringAppend(string, "$e");
        _StringAppendMangledIdentifier(string, enumerationType->declaration->base.name);
        break;
    }

    case ASTTagFunctionType: {
        ASTFunctionTypeRef functionType = (ASTFunctionTypeRef)type;
        StringAppend(string, "$f");
        StringAppendFormat(string, "%zu", ASTArrayGetElementCount(functionType->parameterTypes));
        for (Index index = 0; index < ASTArrayGetElementCount(functionType->parameterTypes); index++) {
            ASTTypeRef parameterType = (ASTTypeRef)ASTArrayGetElementAtIndex(functionType->parameterTypes, index);
            _StringAppendMangledTypeName(string, parameterType);
        }

        _StringAppendMangledTypeName(string, functionType->resultType);
        break;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structureType = (ASTStructureTypeRef)type;
        assert(structureType->declaration);
        StringAppend(string, "$s");
        _StringAppendMangledIdentifier(string, structureType->declaration->base.name);
        break;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTType");
        break;
    }
}
