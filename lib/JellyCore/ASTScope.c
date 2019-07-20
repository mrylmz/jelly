#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/ASTScope.h"
#include "JellyCore/Diagnostic.h"

static inline void _ASTScopeDump(ASTScopeRef scope, FILE *target, Index indentation);
static inline void _PrintIndentation(FILE *target, Index indentation);
static inline void _PrintCString(FILE *target, const Char *string);
static inline void _PrintScopeKind(FILE *target, ASTScopeKind kind);
static inline void _PrintDeclaration(FILE *target, Index indentation, ASTDeclarationRef declaration);
static inline void _PrintType(FILE *target, ASTTypeRef type);
static inline void _PrintBuiltinType(FILE *target, ASTBuiltinTypeKind kind);

void ASTScopeInsertDeclaration(ASTScopeRef scope, ASTDeclarationRef declaration) {
    switch (declaration->base.tag) {
    case ASTTagEnumerationDeclaration: {
        if (scope->kind == ASTScopeKindGlobal) {
            ASTArrayIteratorRef iterator = ASTArrayGetIterator(scope->declarations);
            while (iterator) {
                ASTDeclarationRef child = (ASTDeclarationRef)ASTArrayIteratorGetElement(iterator);
                if (StringIsEqual(child->name, declaration->name)) {
                    ReportError("Invalid redeclaration of identifier");
                    return;
                }

                iterator = ASTArrayIteratorNext(iterator);
            }

            ASTArrayAppendElement(scope->declarations, declaration);
            return;
        } else {
            ReportError("enum can only be declared in global scope");
            return;
        }
    }

    case ASTTagFunctionDeclaration:
    case ASTTagForeignFunctionDeclaration:
    case ASTTagIntrinsicFunctionDeclaration: {
        ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
        if (scope->kind == ASTScopeKindGlobal) {
            ASTArrayIteratorRef iterator = ASTArrayGetIterator(scope->declarations);
            while (iterator) {
                ASTDeclarationRef child = (ASTDeclarationRef)ASTArrayIteratorGetElement(iterator);
                if (StringIsEqual(child->name, declaration->name)) {
                    if (child->base.tag != ASTTagFunctionDeclaration && child->base.tag != ASTTagForeignFunctionDeclaration &&
                        child->base.tag != ASTTagIntrinsicFunctionDeclaration) {
                        ReportError("Invalid redeclaration of identifier");
                        return;
                    }

                    // TODO: @Verify This is maybe dependendent on the resolution of the parameter types and result type
                    //       so the addition of this could depend on partial resolution of the function signature

                    ASTFunctionDeclarationRef childFunction = (ASTFunctionDeclarationRef)child;
                    if (ASTArrayGetElementCount(function->parameters) == ASTArrayGetElementCount(childFunction->parameters)) {
                        Bool hasMatchingParameterTypes  = true;
                        ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(function->parameters);
                        ASTArrayIteratorRef rhsIterator = ASTArrayGetIterator(childFunction->parameters);
                        while (lhsIterator && rhsIterator) {
                            ASTValueDeclarationRef lhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(lhsIterator);
                            ASTValueDeclarationRef rhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(rhsIterator);
                            assert(lhsParameter->base.type);
                            assert(rhsParameter->base.type);

                            if (!ASTTypeIsEqual(lhsParameter->base.type, rhsParameter->base.type)) {
                                hasMatchingParameterTypes = false;
                                break;
                            }

                            lhsIterator = ASTArrayIteratorNext(lhsIterator);
                            rhsIterator = ASTArrayIteratorNext(rhsIterator);
                        }

                        if (hasMatchingParameterTypes && ASTTypeIsEqual(function->returnType, childFunction->returnType)) {
                            ReportError("Invalid redeclaration of identifier");
                            return;
                        }
                    }
                }

                iterator = ASTArrayIteratorNext(iterator);
            }

            ASTArrayAppendElement(scope->declarations, declaration);
            return;
        } else {
            ReportError("func can only be declared in global scope");
            return;
        }
    }

    case ASTTagStructureDeclaration: {
        if (scope->kind == ASTScopeKindGlobal) {
            ASTArrayIteratorRef iterator = ASTArrayGetIterator(scope->declarations);
            while (iterator) {
                ASTDeclarationRef child = (ASTDeclarationRef)ASTArrayIteratorGetElement(iterator);
                if (StringIsEqual(child->name, declaration->name)) {
                    ReportError("Invalid redeclaration of identifier");
                    return;
                }

                iterator = ASTArrayIteratorNext(iterator);
            }

            ASTArrayAppendElement(scope->declarations, declaration);
            return;
        } else {
            ReportError("struct can only be declared in global scope");
            return;
        }
    }

    case ASTTagValueDeclaration: {
        ASTArrayIteratorRef iterator = ASTArrayGetIterator(scope->declarations);
        while (iterator) {
            ASTDeclarationRef child = (ASTDeclarationRef)ASTArrayIteratorGetElement(iterator);
            if (StringIsEqual(child->name, declaration->name)) {
                ReportError("Invalid redeclaration of identifier");
                return;
            }

            iterator = ASTArrayIteratorNext(iterator);
        }

        ASTArrayAppendElement(scope->declarations, declaration);
        return;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for declaration!");
        break;
    }
}

ASTDeclarationRef ASTScopeLookupDeclarationByName(ASTScopeRef scope, StringRef name) {
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(scope->declarations);
    while (iterator) {
        ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayIteratorGetElement(iterator);
        if (StringIsEqual(declaration->name, name)) {
            return declaration;
        }

        iterator = ASTArrayIteratorNext(iterator);
    }

    return NULL;
}

ASTDeclarationRef ASTScopeLookupDeclarationByNameOrMatchingFunctionSignature(ASTScopeRef scope, StringRef name, ASTFixity fixity,
                                                                             ASTArrayRef parameters, ASTTypeRef resultType) {
    ASTArrayIteratorRef iterator = ASTArrayGetIterator(scope->declarations);
    while (iterator) {
        ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayIteratorGetElement(iterator);
        if (!StringIsEqual(declaration->name, name)) {
            iterator = ASTArrayIteratorNext(iterator);
            continue;
        }

        if (declaration->base.tag != ASTTagFunctionDeclaration && declaration->base.tag != ASTTagForeignFunctionDeclaration &&
            declaration->base.tag != ASTTagIntrinsicFunctionDeclaration) {
            return declaration;
        }

        // TODO: Perform checks
        ASTFunctionDeclarationRef function = (ASTFunctionDeclarationRef)declaration;
        if (function->fixity != fixity) {
            iterator = ASTArrayIteratorNext(iterator);
            continue;
        }

        if (ASTArrayGetElementCount(function->parameters) != ASTArrayGetElementCount(parameters)) {
            iterator = ASTArrayIteratorNext(iterator);
            continue;
        }

        if (!ASTTypeIsEqual(function->returnType, resultType)) {
            iterator = ASTArrayIteratorNext(iterator);
            continue;
        }

        Bool hasSameParameters          = true;
        ASTArrayIteratorRef lhsIterator = ASTArrayGetIterator(function->parameters);
        ASTArrayIteratorRef rhsIterator = ASTArrayGetIterator(parameters);
        while (lhsIterator && rhsIterator) {
            ASTValueDeclarationRef lhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(lhsIterator);
            ASTValueDeclarationRef rhsParameter = (ASTValueDeclarationRef)ASTArrayIteratorGetElement(rhsIterator);

            if (!ASTTypeIsEqual(lhsParameter->base.type, rhsParameter->base.type)) {
                hasSameParameters = false;
                break;
            }

            lhsIterator = ASTArrayIteratorNext(lhsIterator);
            rhsIterator = ASTArrayIteratorNext(rhsIterator);
        }

        if (hasSameParameters) {
            return declaration;
        }

        iterator = ASTArrayIteratorNext(iterator);
    }

    return NULL;
}

ASTDeclarationRef ASTScopeLookupDeclarationInHierarchyByName(ASTScopeRef scope, StringRef name) {
    ASTScopeRef currentScope = scope;
    while (currentScope) {
        ASTDeclarationRef declaration = ASTScopeLookupDeclarationByName(currentScope, name);
        if (declaration) {
            return declaration;
        }

        currentScope = ASTScopeGetNextParentForLookup(currentScope);
    }

    return NULL;
}

ASTScopeRef ASTScopeGetNextParentForLookup(ASTScopeRef scope) {
    ASTScopeRef parent = scope->parent;
    while (parent) {
        switch (scope->kind) {
        case ASTScopeKindGlobal:
        case ASTScopeKindBranch:
        case ASTScopeKindLoop:
        case ASTScopeKindCase:
        case ASTScopeKindSwitch:
            return parent;

        case ASTScopeKindEnumeration:
        case ASTScopeKindFunction:
        case ASTScopeKindStructure:
            if (parent->kind == ASTScopeKindGlobal) {
                return parent;
            }
            break;
        }

        parent = parent->parent;
    }

    return parent;
}

void ASTScopeDump(ASTScopeRef scope, FILE *target) {
    _ASTScopeDump(scope, target, 0);
}

static inline void _ASTScopeDump(ASTScopeRef scope, FILE *target, Index indentation) {
    _PrintIndentation(target, indentation);
    _PrintScopeKind(target, scope->kind);
    _PrintCString(target, " {\n");

    indentation += 1;

    for (Index index = 0; index < ASTArrayGetElementCount(scope->declarations); index++) {
        ASTDeclarationRef declaration = (ASTDeclarationRef)ASTArrayGetElementAtIndex(scope->declarations, index);
        _PrintIndentation(target, indentation);
        _PrintDeclaration(target, indentation, declaration);
        _PrintCString(target, "\n");
    }

    if (ASTArrayGetElementCount(scope->declarations) > 0 && ASTArrayGetElementCount(scope->children) > 0) {
        _PrintCString(target, "\n");
    }

    for (Index index = 0; index < ASTArrayGetElementCount(scope->children); index++) {
        ASTScopeRef child = (ASTScopeRef)ASTArrayGetElementAtIndex(scope->children, index);
        _ASTScopeDump(child, target, indentation);

        if (index + 1 < ASTArrayGetElementCount(scope->children)) {
            _PrintCString(target, "\n");
        }
    }

    indentation -= 1;

    _PrintIndentation(target, indentation);
    _PrintCString(target, "}\n");
}

static inline void _PrintIndentation(FILE *target, Index indentation) {
    for (Index i = 0; i < indentation; i++) {
        fprintf(target, "%s", "  ");
    }
}

static inline void _PrintCString(FILE *target, const Char *string) {
    fprintf(target, "%s", string);
}

static inline void _PrintScopeKind(FILE *target, ASTScopeKind kind) {
    switch (kind) {
    case ASTScopeKindGlobal:
        return _PrintCString(target, "[Global]");

    case ASTScopeKindBranch:
        return _PrintCString(target, "[Branch]");

    case ASTScopeKindLoop:
        return _PrintCString(target, "[Loop]");

    case ASTScopeKindCase:
        return _PrintCString(target, "[Case]");

    case ASTScopeKindSwitch:
        return _PrintCString(target, "[Switch]");

    case ASTScopeKindEnumeration:
        return _PrintCString(target, "[Enumeration]");

    case ASTScopeKindFunction:
        return _PrintCString(target, "[Function]");

    case ASTScopeKindStructure:
        return _PrintCString(target, "[Structure]");

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown kind given for ASTScopeKind!");
}

static inline void _PrintDeclaration(FILE *target, Index indentation, ASTDeclarationRef declaration) {
    _PrintCString(target, StringGetCharacters(declaration->name));
    _PrintCString(target, " = ");

    switch (declaration->base.tag) {
    case ASTTagModuleDeclaration: {
        _PrintCString(target, "module");
        break;
    }

    case ASTTagEnumerationDeclaration: {
        _PrintCString(target, "enum ");
        break;
    }

    case ASTTagFunctionDeclaration: {
        _PrintCString(target, "func ");
        break;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)declaration;
        _PrintCString(target, "struct {\n");
        indentation += 1;
        for (Index index = 0; index < ASTArrayGetElementCount(structure->values); index++) {
            ASTNodeRef child = (ASTNodeRef)ASTArrayGetElementAtIndex(structure->values, index);
            assert(child->tag == ASTTagValueDeclaration);

            ASTValueDeclarationRef value = (ASTValueDeclarationRef)child;
            assert(value->kind == ASTValueKindVariable);

            _PrintIndentation(target, indentation);
            _PrintCString(target, StringGetCharacters(value->base.name));
            _PrintCString(target, " = ");
            _PrintType(target, value->base.type);
            _PrintCString(target, "\n");
        }
        indentation -= 1;
        _PrintIndentation(target, indentation);
        _PrintCString(target, "}\n");
        break;
    }

    case ASTTagValueDeclaration: {
        _PrintCString(target, "value ");
        break;
    }

    default:
        JELLY_UNREACHABLE("Unknown tag given for ASTDeclarationRef in ScopeDumper!");
        break;
    }
}

static inline void _PrintType(FILE *target, ASTTypeRef type) {
    switch (type->tag) {
    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef opaque = (ASTOpaqueTypeRef)type;
        _PrintCString(target, StringGetCharacters(opaque->name));
        _PrintCString(target, "?");
        return;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef pointer = (ASTPointerTypeRef)type;
        _PrintType(target, pointer->pointeeType);
        _PrintCString(target, "*");
        return;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef array = (ASTArrayTypeRef)type;
        _PrintType(target, array->elementType);
        _PrintCString(target, "[]");
        return;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef builtin = (ASTBuiltinTypeRef)type;
        _PrintCString(target, "type ");
        _PrintBuiltinType(target, builtin->kind);
        return;
    }

    case ASTTagEnumerationType:
        return _PrintCString(target, "enum");

    case ASTTagFunctionType: {
        ASTFunctionTypeRef func = (ASTFunctionTypeRef)type;
        assert(func->declaration);
        _PrintCString(target, "func (");
        ASTArrayIteratorRef iterator = ASTArrayGetIterator(func->parameterTypes);
        while (iterator) {
            ASTTypeRef parameterType = (ASTTypeRef)ASTArrayIteratorGetElement(iterator);
            _PrintType(target, parameterType);

            iterator = ASTArrayIteratorNext(iterator);

            if (iterator) {
                _PrintCString(target, ", ");
            }
        }

        _PrintCString(target, ") -> ");
        _PrintType(target, func->resultType);
        return;
    }

    case ASTTagStructureType: {
        ASTStructureTypeRef structure = (ASTStructureTypeRef)type;
        assert(structure->declaration);
        _PrintCString(target, "struct ");
        _PrintCString(target, StringGetCharacters(structure->declaration->base.name));
        return;
    }

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown tag given for ASTTypeRef in ScopeDumper!");
}

static inline void _PrintBuiltinType(FILE *target, ASTBuiltinTypeKind kind) {
    switch (kind) {
    case ASTBuiltinTypeKindError:
        return _PrintCString(target, "<error>");
    case ASTBuiltinTypeKindVoid:
        return _PrintCString(target, "Void");
    case ASTBuiltinTypeKindBool:
        return _PrintCString(target, "Bool");
    case ASTBuiltinTypeKindInt8:
        return _PrintCString(target, "Int8");
    case ASTBuiltinTypeKindInt16:
        return _PrintCString(target, "Int16");
    case ASTBuiltinTypeKindInt32:
        return _PrintCString(target, "Int32");
    case ASTBuiltinTypeKindInt64:
        return _PrintCString(target, "Int64");
    case ASTBuiltinTypeKindInt:
        return _PrintCString(target, "Int");
    case ASTBuiltinTypeKindUInt8:
        return _PrintCString(target, "UInt8");
    case ASTBuiltinTypeKindUInt16:
        return _PrintCString(target, "UInt16");
    case ASTBuiltinTypeKindUInt32:
        return _PrintCString(target, "UInt32");
    case ASTBuiltinTypeKindUInt64:
        return _PrintCString(target, "UInt64");
    case ASTBuiltinTypeKindUInt:
        return _PrintCString(target, "UInt");
    case ASTBuiltinTypeKindFloat32:
        return _PrintCString(target, "Float32");
    case ASTBuiltinTypeKindFloat64:
        return _PrintCString(target, "Float64");
    case ASTBuiltinTypeKindFloat:
        return _PrintCString(target, "Float");
    default:
        JELLY_UNREACHABLE("Unknown kind given for ASTBuiltinTypeKind in ScopeDumper!");
        break;
    }
}
