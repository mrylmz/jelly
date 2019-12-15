#include "JellyCore/ClangImporter.h"
#include "JellyCore/Diagnostic.h"

#include <clang-c/Index.h>

struct _ClangImporter {
    AllocatorRef allocator;
    ASTContextRef context;
    ASTModuleDeclarationRef module;
    ASTSourceUnitRef sourceUnit;
    ScopeID currentScope;
    Bool hasLocalErrorReports;
};

static inline void _ClangImporterReportError(ClangImporterRef importer, CXCursor cursor, const Char *message);

static inline ASTTypeRef _ClangImporterParseType(ClangImporterRef importer, CXType type);

enum CXChildVisitResult _ClangImporterParseCursorVisitor(CXCursor cursor, CXCursor parent, CXClientData userdata);
ASTNodeRef _ClangImporterParseCursor(ClangImporterRef importer, StringRef implicitName, CXCursor cursor);

static enum CXChildVisitResult _ClangImporterGetCursorChildrenVisitor(CXCursor cursor, CXCursor parent, CXClientData userdata);
ArrayRef _ClangImporterGetCursorChildren(ClangImporterRef importer, CXCursor cursor);

ScopeID _ClangImporterPushScope(ClangImporterRef importer, SourceRange location, ScopeKind kind);
void _ClangImporterPopScope(ClangImporterRef importer);
void _ClangImporterSetScopeNode(ClangImporterRef importer, ScopeID scope, ASTNodeRef node);

ClangImporterRef ClangImporterCreate(AllocatorRef allocator, ASTContextRef context) {
    ClangImporterRef importer      = AllocatorAllocate(allocator, sizeof(struct _ClangImporter));
    importer->allocator            = allocator;
    importer->context              = context;
    importer->module               = NULL;
    importer->sourceUnit           = NULL;
    importer->currentScope         = kScopeGlobal;
    importer->hasLocalErrorReports = false;
    return importer;
}

void ClangImporterDestroy(ClangImporterRef importer) {
    AllocatorDeallocate(importer->allocator, importer);
}

ASTModuleDeclarationRef ClangImporterImport(ClangImporterRef importer, StringRef filePath) {
    StringRef moduleName = StringCreateCopyOfBasename(importer->allocator, filePath);
    // TODO: Perform a correct string escaping, replacing whitespaces only is insufficient...
    StringReplaceOccurenciesOf(moduleName, ' ', '_');

    importer->module     = ASTContextCreateModuleDeclaration(importer->context, SourceRangeNull(), kScopeNull, ASTModuleKindInterface,
                                                         moduleName, NULL, NULL);
    importer->sourceUnit = ASTContextCreateSourceUnit(importer->context, SourceRangeNull(), importer->currentScope, filePath, NULL);
    ASTArrayAppendElement(importer->module->sourceUnits, importer->sourceUnit);

    const Char *arguments[] = {
        "-Xclang",
        "-ast-dump",
        "-fsyntax-only",
    };

    CXIndex index          = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(index, StringGetCharacters(filePath), arguments, 3, NULL, 0,
                                                        CXTranslationUnit_None);
    if (!unit) {
        ReportErrorFormat("Parsing of header '%s' failed", StringGetCharacters(filePath));
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(cursor, &_ClangImporterParseCursorVisitor, importer);

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    StringDestroy(moduleName);
    return importer->module;
}

static inline void _ClangImporterReportError(ClangImporterRef importer, CXCursor cursor, const Char *message) {
    CXString source = clang_getCursorPrettyPrinted(cursor, NULL);
    ReportErrorFormat("%s\n\n%s\n", message, clang_getCString(source));
    clang_disposeString(source);

    importer->hasLocalErrorReports = true;
}

static inline ASTTypeRef _ClangImporterParseType(ClangImporterRef importer, CXType type) {
    switch (type.kind) {
    case CXType_Invalid:
    case CXType_Unexposed:
        return NULL;

    case CXType_Void:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindVoid);

    case CXType_Bool:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindBool);

    case CXType_Char_U:
    case CXType_Char_S:
    case CXType_SChar:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindInt8);

    case CXType_UChar:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindUInt8);

    case CXType_Short:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindInt16);

    case CXType_Char16:
    case CXType_UShort:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindUInt16);

    case CXType_WChar:
    case CXType_Int:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindInt32);

    case CXType_Char32:
    case CXType_UInt:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindUInt32);

    case CXType_Long:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindInt);

    case CXType_ULong:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindUInt);

    case CXType_LongLong:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindInt64);

    case CXType_ULongLong:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindUInt64);

    case CXType_Float:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindFloat32);

    case CXType_Double:
        return (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindFloat64);

    case CXType_Elaborated: {
        CXType namedType  = clang_Type_getNamedType(type);
        CXString spelling = clang_getTypeSpelling(namedType);
        StringRef name    = StringCreate(importer->allocator, clang_getCString(spelling));
        StringRemovePrefix(name, "struct ");
        StringRemovePrefix(name, "union ");
        StringRemovePrefix(name, "enum ");

        if (StringGetLength(name) < 1) {
            return NULL;
        }

        ASTTypeRef opaque = (ASTTypeRef)ASTContextCreateOpaqueType(importer->context, SourceRangeNull(), importer->currentScope, name);
        StringDestroy(name);
        clang_disposeString(spelling);
        return opaque;
    }

    case CXType_Enum:
    case CXType_Record: {
        CXString cxTypeSpelling = clang_getTypeSpelling(type);
        StringRef typeSpelling  = StringCreate(importer->allocator, clang_getCString(cxTypeSpelling));
        StringRemovePrefix(typeSpelling, "struct ");
        StringRemovePrefix(typeSpelling, "union ");
        StringRemovePrefix(typeSpelling, "enum ");

        ASTTypeRef type = NULL;
        if (StringGetLength(typeSpelling) > 0) {
            type = (ASTTypeRef)ASTContextCreateOpaqueType(importer->context, SourceRangeNull(), importer->currentScope, typeSpelling);
        }

        StringDestroy(typeSpelling);
        clang_disposeString(cxTypeSpelling);
        return type;
    }

    case CXType_FunctionProto:
    case CXType_FunctionNoProto: {
        Index parameterCount    = clang_getNumArgTypes(type);
        ArrayRef parameterTypes = ArrayCreateEmpty(importer->allocator, sizeof(ASTTypeRef), parameterCount);
        for (Index index = 0; index < parameterCount; index++) {
            CXType argumentType      = clang_getArgType(type, index);
            ASTTypeRef parameterType = _ClangImporterParseType(importer, argumentType);
            if (!parameterType) {
                ArrayDestroy(parameterTypes);
                return NULL;
            }

            ArrayAppendElement(parameterTypes, &parameterType);
        }

        CXType cxResultType   = clang_getResultType(type);
        ASTTypeRef resultType = _ClangImporterParseType(importer, cxResultType);
        if (!resultType) {
            ArrayDestroy(parameterTypes);
            return NULL;
        }

        ASTTypeRef result = NULL;
        if (parameterCount == ArrayGetElementCount(parameterTypes)) {
            result = (ASTTypeRef)ASTContextCreateFunctionType(importer->context, SourceRangeNull(), importer->currentScope, parameterTypes,
                                                              resultType);
        }

        ArrayDestroy(parameterTypes);

        return result;
    }

    case CXType_Typedef: {
        CXString spelling = clang_getTypedefName(type);
        StringRef name    = StringCreate(importer->allocator, clang_getCString(spelling));
        clang_disposeString(spelling);

        if (StringGetLength(name) < 1) {
            StringDestroy(name);
            CXCursor cursor = clang_getTypeDeclaration(type);
            return _ClangImporterParseType(importer, clang_getTypedefDeclUnderlyingType(cursor));
        }

        ASTTypeRef opaque = (ASTTypeRef)ASTContextCreateOpaqueType(importer->context, SourceRangeNull(), importer->currentScope, name);
        StringDestroy(name);
        return opaque;
    }

    case CXType_BlockPointer:
    case CXType_Pointer: {
        CXType cxPointeeType   = clang_getPointeeType(type);
        ASTTypeRef pointeeType = _ClangImporterParseType(importer, cxPointeeType);
        if (!pointeeType) {
            return NULL;
        }

        if (pointeeType->tag == ASTTagFunctionType) {
            return pointeeType;
        }

        return (ASTTypeRef)ASTContextCreatePointerType(importer->context, SourceRangeNull(), importer->currentScope, pointeeType);
    }

    case CXType_ConstantArray: {
        ASTTypeRef elementType = _ClangImporterParseType(importer, clang_getElementType(type));
        if (!elementType) {
            return NULL;
        }

        ASTExpressionRef size = (ASTExpressionRef)ASTContextCreateConstantIntExpression(importer->context, SourceRangeNull(),
                                                                                        importer->currentScope, clang_getArraySize(type));
        return (ASTTypeRef)ASTContextCreateArrayType(importer->context, SourceRangeNull(), importer->currentScope, elementType, size);
    }

    case CXType_IncompleteArray: {
        ASTTypeRef elementType = _ClangImporterParseType(importer, clang_getElementType(type));
        if (!elementType) {
            return NULL;
        }

        return (ASTTypeRef)ASTContextCreateArrayType(importer->context, SourceRangeNull(), importer->currentScope, elementType, NULL);
    }

    default:
        return NULL;
    }
}

enum CXChildVisitResult _ClangImporterParseCursorVisitor(CXCursor cursor, CXCursor parent, CXClientData userdata) {
    ClangImporterRef importer = (ClangImporterRef)userdata;
    _ClangImporterParseCursor(importer, NULL, cursor);
    return CXChildVisit_Continue;
}

ASTNodeRef _ClangImporterParseCursor(ClangImporterRef importer, StringRef implicitName, CXCursor cursor) {
    enum CXLanguageKind languageKind = clang_getCursorLanguage(cursor);
    if (languageKind != CXLanguage_C) {
        return NULL;
    }

    enum CXCursorKind cursorKind = clang_getCursorKind(cursor);
    if (clang_isInvalid(cursorKind)) {
        return NULL;
    }

    // TODO: Verify if only external interfaces should be parsed...
    //    enum CXLinkageKind linkageKind = clang_getCursorLinkage(cursor);
    //    if (linkageKind != CXLinkage_UniqueExternal && linkageKind != CXLinkage_External) {
    //        return NULL;
    //    }

    if (cursorKind == CXCursor_StructDecl) {
        importer->hasLocalErrorReports = false;

        ScopeID scope     = _ClangImporterPushScope(importer, SourceRangeNull(), ScopeKindStructure);
        CXString spelling = clang_getCursorSpelling(cursor);
        StringRef name    = StringCreate(importer->allocator, clang_getCString(spelling));
        if (StringGetLength(name) < 1 && implicitName) {
            StringDestroy(name);
            name = StringCreateCopy(importer->allocator, implicitName);
        }

        if (StringGetLength(name) > 0) {
            ArrayRef values   = ArrayCreateEmpty(importer->allocator, sizeof(ASTValueDeclarationRef), 8);
            ArrayRef children = _ClangImporterGetCursorChildren(importer, cursor);
            for (Index index = 0; index < ArrayGetElementCount(children); index++) {
                CXCursor childCursor        = *(CXCursor *)ArrayGetElementAtIndex(children, index);
                enum CXCursorKind childKind = clang_getCursorKind(childCursor);
                if (childKind == CXCursor_FieldDecl) {
                    CXString childSpelling = clang_getCursorSpelling(childCursor);
                    CXType childType       = clang_getCursorType(childCursor); // clang_getCanonicalType(clang_getCursorType(childCursor));

                    StringRef name = StringCreate(importer->allocator, clang_getCString(childSpelling));
                    if (StringGetLength(name) < 1) {
                        _ClangImporterReportError(importer, childCursor, "Structure field name is empty");
                    }

                    ASTTypeRef type = _ClangImporterParseType(importer, childType);
                    if (!type) {
                        _ClangImporterReportError(importer, childCursor, "Structure field type couldn't be parsed");
                    }

                    if (StringGetLength(name) > 0 && type) {
                        ASTNodeRef value = (ASTNodeRef)ASTContextCreateValueDeclaration(
                            importer->context, SourceRangeNull(), importer->currentScope, ASTValueKindVariable, name, type, NULL);
                        ArrayAppendElement(values, &value);
                    }

                    StringDestroy(name);
                    clang_disposeString(childSpelling);
                } else if (childKind == CXCursor_PackedAttr) {
                    _ClangImporterReportError(importer, cursor, "Packed structure layout is currently not supported");
                } else if (childKind == CXCursor_StructDecl) {
                    _ClangImporterReportError(importer, cursor, "Structure declarations as members are currently not supported");
                } else if (childKind == CXCursor_UnionDecl) {
                    _ClangImporterReportError(importer, cursor, "Unsupported union declaration found in structure declaration");
                } else {
                    _ClangImporterReportError(importer, cursor, "Unsupported declaration found in structure declaration");
                }
            }

            _ClangImporterPopScope(importer);

            if (!importer->hasLocalErrorReports) {
                ASTStructureDeclarationRef node = ASTContextCreateStructureDeclaration(importer->context, SourceRangeNull(),
                                                                                       importer->currentScope, name, values, NULL);
                node->innerScope                = scope;
                _ClangImporterSetScopeNode(importer, scope, (ASTNodeRef)node);
                ASTArrayAppendElement(importer->sourceUnit->declarations, (ASTNodeRef)node);
            }

            ArrayDestroy(children);
            ArrayDestroy(values);
        }

        StringDestroy(name);
        clang_disposeString(spelling);
    }

    if (cursorKind == CXCursor_UnionDecl) {
        _ClangImporterReportError(importer, cursor, "Union declarations are not supported");
        return NULL;
    }

    if (cursorKind == CXCursor_EnumDecl) {
        if (!clang_isCursorDefinition(cursor)) {
            return NULL;
        }

        importer->hasLocalErrorReports = false;

        ScopeID scope     = _ClangImporterPushScope(importer, SourceRangeNull(), ScopeKindEnumeration);
        CXString spelling = clang_getCursorSpelling(cursor);
        StringRef name    = StringCreate(importer->allocator, clang_getCString(spelling));
        if (StringGetLength(name) < 1 && implicitName) {
            StringDestroy(name);
            name = StringCreateCopy(importer->allocator, implicitName);
        }

        if (StringGetLength(name) > 0) {
            ArrayRef elements = ArrayCreateEmpty(importer->allocator, sizeof(ASTValueDeclarationRef), 8);
            ArrayRef children = _ClangImporterGetCursorChildren(importer, cursor);
            for (Index index = 0; index < ArrayGetElementCount(children); index++) {
                CXCursor childCursor              = *(CXCursor *)ArrayGetElementAtIndex(children, index);
                enum CXCursorKind childCursorKind = clang_getCursorKind(childCursor);
                if (childCursorKind == CXCursor_EnumConstantDecl) {
                    CXString childSpelling = clang_getCursorSpelling(childCursor);
                    StringRef name         = StringCreate(importer->allocator, clang_getCString(childSpelling));
                    if (StringGetLength(name) < 1) {
                        _ClangImporterReportError(importer, childCursor, "Enumeration element name is empty");
                    }

                    Int64 value          = clang_getEnumConstantDeclValue(childCursor);
                    Bool isValueNegative = false;
                    if (value < 0) {
                        value           = -value;
                        isValueNegative = true;
                    }

                    if (!importer->hasLocalErrorReports) {
                        ASTExpressionRef constant = (ASTExpressionRef)ASTContextCreateConstantIntExpression(
                            importer->context, SourceRangeNull(), importer->currentScope, (UInt64)value);
                        if (isValueNegative) {
                            ASTExpressionRef arguments[] = {constant};
                            constant                     = (ASTExpressionRef)ASTContextCreateUnaryExpression(
                                importer->context, SourceRangeNull(), importer->currentScope, ASTUnaryOperatorUnaryMinus, arguments);
                        }

                        // TODO: Add strong typing for enumeration elements and base typing for enumeration based on clang result!
                        ASTTypeRef type = (ASTTypeRef)ASTContextGetBuiltinType(importer->context, ASTBuiltinTypeKindInt);
                        ASTNodeRef node = (ASTNodeRef)ASTContextCreateValueDeclaration(
                            importer->context, SourceRangeNull(), importer->currentScope, ASTValueKindEnumerationElement, name, type,
                            constant);
                        ArrayAppendElement(elements, &node);
                    }

                } else {
                    _ClangImporterReportError(importer, childCursor, "Unsupported declaration found in enumeration declaration");
                }
            }

            _ClangImporterPopScope(importer);

            if (!importer->hasLocalErrorReports) {
                ASTEnumerationDeclarationRef node = ASTContextCreateEnumerationDeclaration(importer->context, SourceRangeNull(),
                                                                                           importer->currentScope, name, elements);
                node->innerScope                  = scope;
                _ClangImporterSetScopeNode(importer, scope, (ASTNodeRef)node);
                ASTArrayAppendElement(importer->sourceUnit->declarations, (ASTNodeRef)node);
            }

            ArrayDestroy(children);
            ArrayDestroy(elements);
        }

        StringDestroy(name);
        clang_disposeString(spelling);
    }

    // TODO: Including the OpenGL/gl.h interface from macOS the OPENGL_DEPRECATED macro is interpreted
    //       as the name of the functions but that is not correct!
    //       There seems to be a problem with the availability attributes of the macOS platform
    if (cursorKind == CXCursor_FunctionDecl) {
        if (clang_isCursorDefinition(cursor)) {
            return NULL;
        }

        if (clang_Cursor_isFunctionInlined(cursor)) {
            _ClangImporterReportError(importer, cursor, "Inline functions are not supported");
            return NULL;
        }

        if (clang_Cursor_isVariadic(cursor)) {
            _ClangImporterReportError(importer, cursor, "Variadic functions are not supported");
            return NULL;
        }

        importer->hasLocalErrorReports = false;

        ScopeID scope     = _ClangImporterPushScope(importer, SourceRangeNull(), ScopeKindFunction);
        CXString spelling = clang_getCursorSpelling(cursor);
        StringRef name    = StringCreate(importer->allocator, clang_getCString(spelling));
        if (StringGetLength(name) < 1) {
            _ClangImporterReportError(importer, cursor, "Name of function is empty");
        }

        CXString mangling     = clang_Cursor_getMangling(cursor);
        StringRef mangledName = StringCreate(importer->allocator, clang_getCString(mangling));
        if (StringGetLength(mangledName) < 1) {
            _ClangImporterReportError(importer, cursor, "Mangled name of function is empty");
        }

        Index argumentCount = clang_Cursor_getNumArguments(cursor);
        ArrayRef arguments  = ArrayCreateEmpty(importer->allocator, sizeof(ASTValueDeclarationRef), argumentCount);
        for (Index index = 0; index < argumentCount; index++) {
            CXCursor argumentCursor   = clang_Cursor_getArgument(cursor, index);
            CXString argumentSpelling = clang_getCursorSpelling(argumentCursor);
            CXType argumentType       = clang_getCursorType(argumentCursor);

            StringRef name = StringCreate(importer->allocator, clang_getCString(argumentSpelling));
            if (StringGetLength(name) < 1) {
                StringAppendFormat(name, "arg%zu", index);
            }

            ASTTypeRef type = _ClangImporterParseType(importer, argumentType);
            if (!type) {
                _ClangImporterReportError(importer, cursor, "Parameter type couldn't be parsed");
            }

            if (StringGetLength(name) > 0 && type) {
                ASTNodeRef argument = (ASTNodeRef)ASTContextCreateValueDeclaration(
                    importer->context, SourceRangeNull(), importer->currentScope, ASTValueKindParameter, name, type, NULL);
                ArrayAppendElement(arguments, &argument);
            }

            StringDestroy(name);
            clang_disposeString(argumentSpelling);
        }

        CXType cursorResultType = clang_getCursorResultType(cursor);
        ASTTypeRef resultType   = _ClangImporterParseType(importer, cursorResultType);
        if (!resultType) {
            _ClangImporterReportError(importer, cursor, "Result type of function couldn't be parsed");
        }

        _ClangImporterPopScope(importer);

        ASTFunctionDeclarationRef node = NULL;
        if (!importer->hasLocalErrorReports) {
            node = ASTContextCreateForeignFunctionDeclaration(importer->context, SourceRangeNull(), importer->currentScope, ASTFixityNone,
                                                              name, arguments, resultType, mangledName);
            node->innerScope = scope;
            _ClangImporterSetScopeNode(importer, scope, (ASTNodeRef)node);
            ASTArrayAppendElement(importer->sourceUnit->declarations, (ASTNodeRef)node);
        }

        ArrayDestroy(arguments);
        StringDestroy(mangledName);
        clang_disposeString(mangling);
        StringDestroy(name);
        clang_disposeString(spelling);

        return (ASTNodeRef)node;
    }

    if (cursorKind == CXCursor_VarDecl) {
        importer->hasLocalErrorReports = false;

        CXString cursorSpelling = clang_getCursorSpelling(cursor);
        CXType cursorType       = clang_getCanonicalType(clang_getCursorType(cursor));

        StringRef name = StringCreate(importer->allocator, clang_getCString(cursorSpelling));
        if (StringGetLength(name) < 1) {
            _ClangImporterReportError(importer, cursor, "Global variable name is empty");
        }

        ASTTypeRef type = _ClangImporterParseType(importer, cursorType);
        if (!type) {
            _ClangImporterReportError(importer, cursor, "Global variable type couldn't be parsed");
        }

        // TODO: Check if assignment expression of global has to be parsed!
        if (!importer->hasLocalErrorReports) {
            ASTNodeRef node = (ASTNodeRef)ASTContextCreateValueDeclaration(importer->context, SourceRangeNull(), importer->currentScope,
                                                                           ASTValueKindVariable, name, type, NULL);
            ASTArrayAppendElement(importer->sourceUnit->declarations, node);
        }

        StringDestroy(name);
        clang_disposeString(cursorSpelling);
    }

    if (cursorKind == CXCursor_TypedefDecl) {
        importer->hasLocalErrorReports = false;

        CXString cursorSpelling = clang_getCursorSpelling(cursor);
        StringRef name          = StringCreate(importer->allocator, clang_getCString(cursorSpelling));
        if (StringGetLength(name) < 1) {
            _ClangImporterReportError(importer, cursor, "Typealias name is empty");
        }

        CXType underlyingType                   = clang_getTypedefDeclUnderlyingType(cursor);
        CXType canonicalType                    = clang_getCanonicalType(underlyingType);
        CXCursor declarationCursor              = clang_getTypeDeclaration(canonicalType);
        enum CXCursorKind declarationCursorKind = clang_getCursorKind(declarationCursor);
        CXString declarationSpelling            = clang_getCursorSpelling(declarationCursor);
        StringRef declarationName               = StringCreate(importer->allocator, clang_getCString(declarationSpelling));
        if (StringGetLength(declarationName) < 1) {
            if (declarationCursorKind == CXCursor_StructDecl || declarationCursorKind == CXCursor_EnumDecl) {
                _ClangImporterParseCursor(importer, name, declarationCursor);
            }
        } else {
            if (!StringIsEqual(name, declarationName)) {
                ASTTypeRef type = _ClangImporterParseType(importer, canonicalType);
                if (!type) {
                    _ClangImporterReportError(importer, cursor, "Typealias type couldn't be parsed");
                }

                if (!importer->hasLocalErrorReports &&
                    (declarationCursorKind != CXCursor_NoDeclFound ||
                     (declarationCursorKind == CXCursor_NoDeclFound && type->tag == ASTTagFunctionType))) {
                    ASTNodeRef node = (ASTNodeRef)ASTContextCreateTypeAliasDeclaration(importer->context, SourceRangeNull(),
                                                                                       importer->currentScope, name, type);
                    ASTArrayAppendElement(importer->sourceUnit->declarations, node);
                }
            }
        }

        StringDestroy(declarationName);
        clang_disposeString(declarationSpelling);
        StringDestroy(name);
        clang_disposeString(cursorSpelling);
    }

    return NULL;
}

static enum CXChildVisitResult _ClangImporterGetCursorChildrenVisitor(CXCursor cursor, CXCursor parent, CXClientData userdata) {
    ArrayRef children = (ArrayRef)userdata;
    ArrayAppendElement(children, &cursor);
    return CXChildVisit_Continue;
}

ArrayRef _ClangImporterGetCursorChildren(ClangImporterRef importer, CXCursor cursor) {
    ArrayRef children = ArrayCreateEmpty(importer->allocator, sizeof(CXCursor), 8);
    clang_visitChildren(cursor, &_ClangImporterGetCursorChildrenVisitor, children);
    return children;
}

ScopeID _ClangImporterPushScope(ClangImporterRef importer, SourceRange location, ScopeKind kind) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(importer->context);
    ScopeID scope              = SymbolTableInsertScope(symbolTable, kind, importer->currentScope, location.start);
    importer->currentScope     = scope;
    return scope;
}

void _ClangImporterPopScope(ClangImporterRef importer) {
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(importer->context);
    ScopeID parent             = SymbolTableGetScopeParent(symbolTable, importer->currentScope);
    assert(parent != kScopeNull);
    importer->currentScope = parent;
}

void _ClangImporterSetScopeNode(ClangImporterRef importer, ScopeID scope, ASTNodeRef node) {
    SymbolTableSetScopeUserdata(ASTContextGetSymbolTable(importer->context), scope, node);
}
