#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTMangling.h"
#include "JellyCore/Diagnostic.h"

// TODO: Add specification for name mangling and refactor ASTMangling to match the specification
// TODO: Allocate mangled names inside of AST as uniqued identifiers to avoid memory leaks and reduce memory footprint

static inline void _MangleEnumerationDeclarationName(ASTEnumerationDeclarationRef declaration);
static inline void _MangleFunctionDeclarationName(ASTFunctionDeclarationRef declaration);
static inline void _MangleStructureDeclarationName(ASTStructureDeclarationRef declaration);
static inline void _MangleValueDeclarationName(ASTValueDeclarationRef declaration);

static inline void _StringAppendMangledIdentifier(StringRef string, StringRef identifier);
static inline void _StringAppendMangledTypeName(StringRef string, ASTTypeRef type);

void PerformNameMangling(ASTContextRef context, ASTModuleDeclarationRef module) {
    for (Index sourceUnitIndex = 0; sourceUnitIndex < ASTArrayGetElementCount(module->sourceUnits); sourceUnitIndex++) {
        ASTSourceUnitRef sourceUnit = (ASTSourceUnitRef)ASTArrayGetElementAtIndex(module->sourceUnits, sourceUnitIndex);
        for (Index index = 0; index < ASTArrayGetElementCount(sourceUnit->declarations); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(sourceUnit->declarations, index);
            if (child->tag == ASTTagEnumerationDeclaration) {
                _MangleEnumerationDeclarationName((ASTEnumerationDeclarationRef)child);
            }

            if (child->tag == ASTTagFunctionDeclaration) {
                _MangleFunctionDeclarationName((ASTFunctionDeclarationRef)child);
            }

            if (child->tag == ASTTagStructureDeclaration) {
                _MangleStructureDeclarationName((ASTStructureDeclarationRef)child);
            }

            if (child->tag == ASTTagValueDeclaration) {
                _MangleValueDeclarationName((ASTValueDeclarationRef)child);
            }
        }
    }
}

static inline void _MangleEnumerationDeclarationName(ASTEnumerationDeclarationRef declaration) {
    assert(!declaration->base.mangledName);

    StringRef mangledName = StringCreate(AllocatorGetSystemDefault(), "$E");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleFunctionDeclarationName(ASTFunctionDeclarationRef declaration) {
    assert(!declaration->base.mangledName);

    StringRef mangledName = StringCreate(AllocatorGetSystemDefault(), "$F");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);

    StringAppendFormat(mangledName, "%zu", ASTArrayGetElementCount(declaration->parameters));
    for (Index index = 0; index < ASTArrayGetElementCount(declaration->parameters); index++) {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)ASTArrayGetElementAtIndex(declaration->parameters, index);
        _StringAppendMangledTypeName(mangledName, value->base.type);
    }

    _StringAppendMangledTypeName(mangledName, declaration->returnType);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleStructureDeclarationName(ASTStructureDeclarationRef declaration) {
    assert(!declaration->base.mangledName);

    StringRef mangledName = StringCreate(AllocatorGetSystemDefault(), "$S");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);
    declaration->base.mangledName = mangledName;
}

static inline void _MangleValueDeclarationName(ASTValueDeclarationRef declaration) {
    assert(!declaration->base.mangledName);

    StringRef mangledName = StringCreate(AllocatorGetSystemDefault(), "$V");
    _StringAppendMangledIdentifier(mangledName, declaration->base.name);
    declaration->base.mangledName = mangledName;
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
