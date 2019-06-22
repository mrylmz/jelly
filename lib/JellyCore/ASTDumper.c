#include "JellyCore/ASTDumper.h"

// TODO: Add runtime informations like memory address and also other members like types
//       for now this implementation will be helpful to run tests for the Parser...

struct _ASTDumper {
    AllocatorRef allocator;
    FILE *target;
    Index indentation;
};

static inline void _ASTDumperPrintIndentation(ASTDumperRef dumper);
static inline void _ASTDumperPrintCString(ASTDumperRef dumper, const Char *string);
static inline void _ASTDumperPrintProperty(ASTDumperRef dumper, const Char *key, const Char *value);
static inline void _ASTDumperPrintTag(ASTDumperRef dumper, ASTNodeRef node);
static inline void _ASTDumperPrintBuiltinTypeKind(ASTDumperRef dumper, ASTBuiltinTypeKind kind);
static inline void _ASTDumperPrintPrefixOperator(ASTDumperRef dumper, ASTUnaryOperator op);
static inline void _ASTDumperPrintInfixOperator(ASTDumperRef dumper, ASTBinaryOperator op);
static inline void _ASTDumperDumpChild(ASTDumperRef dumper, ASTNodeRef child);
static inline void _ASTDumperDumpChildren(ASTDumperRef dumper, ASTLinkedListRef list);

ASTDumperRef ASTDumperCreate(AllocatorRef allocator, FILE *target) {
    ASTDumperRef dumper = (ASTDumperRef)AllocatorAllocate(allocator, sizeof(struct _ASTDumper));
    dumper->allocator   = allocator;
    dumper->target      = target;
    dumper->indentation = 0;
    return dumper;
}

void ASTDumperDestroy(ASTDumperRef dumper) {
    AllocatorDeallocate(dumper->allocator, dumper);
}

void ASTDumperDump(ASTDumperRef dumper, ASTNodeRef node) {
    _ASTDumperPrintIndentation(dumper);
    _ASTDumperPrintTag(dumper, node);
    _ASTDumperPrintCString(dumper, "\n");

    switch (node->tag) {
    case ASTTagSourceUnit: {
        ASTSourceUnitRef unit = (ASTSourceUnitRef)node;
        _ASTDumperPrintProperty(dumper, "filePath", StringGetCharacters(unit->filePath));
        _ASTDumperDumpChildren(dumper, unit->declarations);
        return;
    }

    case ASTTagLoadDirective: {
        ASTLoadDirectiveRef load = (ASTLoadDirectiveRef)node;
        _ASTDumperPrintProperty(dumper, "sourceFilePath", StringGetCharacters(load->filePath->stringValue));
        return;
    }

    case ASTTagBlock: {
        ASTBlockRef block = (ASTBlockRef)node;
        _ASTDumperDumpChildren(dumper, block->statements);
        return;
    }

    case ASTTagIfStatement: {
        ASTIfStatementRef statement = (ASTIfStatementRef)node;
        _ASTDumperDumpChild(dumper, statement->condition);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->thenBlock);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->elseBlock);
        return;
    }

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        _ASTDumperDumpChild(dumper, (ASTNodeRef)loop->condition);
        _ASTDumperDumpChild(dumper, (ASTNodeRef)loop->loopBlock);
        return;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            _ASTDumperDumpChild(dumper, statement->condition);
        }

        _ASTDumperDumpChild(dumper, (ASTNodeRef)statement->body);
        return;
    }

    case ASTTagSwitchStatement: {
        ASTSwitchStatementRef statement = (ASTSwitchStatementRef)node;
        _ASTDumperDumpChild(dumper, statement->argument);
        _ASTDumperDumpChildren(dumper, statement->cases);
        return;
    }

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindReturn && control->result) {
            _ASTDumperDumpChild(dumper, control->result);
        }
        return;
    }

    case ASTTagUnaryExpression: {
        ASTUnaryExpressionRef unary = (ASTUnaryExpressionRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@operator = '");
        _ASTDumperPrintPrefixOperator(dumper, unary->op);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;

        _ASTDumperDumpChild(dumper, unary->arguments[0]);
        return;
    }

    case ASTTagBinaryExpression: {
        ASTBinaryExpressionRef binary = (ASTBinaryExpressionRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@operator = '");
        _ASTDumperPrintInfixOperator(dumper, binary->op);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;

        _ASTDumperDumpChild(dumper, binary->arguments[0]);
        _ASTDumperDumpChild(dumper, binary->arguments[1]);
        return;
    }

    case ASTTagIdentifierExpression: {
        ASTIdentifierExpressionRef expression = (ASTIdentifierExpressionRef)node;
        _ASTDumperPrintProperty(dumper, "identifier", StringGetCharacters(expression->name));
        return;
    }

    case ASTTagMemberAccessExpression: {
        ASTMemberAccessExpressionRef expression = (ASTMemberAccessExpressionRef)node;
        _ASTDumperPrintProperty(dumper, "memberName", StringGetCharacters(expression->memberName));
        _ASTDumperDumpChild(dumper, expression->argument);
        return;
    }

    case ASTTagCallExpression: {
        ASTCallExpressionRef call = (ASTCallExpressionRef)node;
        _ASTDumperDumpChild(dumper, call->callee);
        _ASTDumperDumpChildren(dumper, call->arguments);
        return;
    }

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        if (constant->kind == ASTConstantKindNil) {
        } else if (constant->kind == ASTConstantKindBool) {
            if (constant->boolValue) {
                _ASTDumperPrintProperty(dumper, "value", "true");
            } else {
                _ASTDumperPrintProperty(dumper, "value", "false");
            }
        } else if (constant->kind == ASTConstantKindInt) {
            dumper->indentation += 1;
            _ASTDumperPrintIndentation(dumper);
            _ASTDumperPrintCString(dumper, "@value = '");
            Char buffer[20];
            snprintf(&buffer[0], 20, "%lld", constant->intValue);
            _ASTDumperPrintCString(dumper, &buffer[0]);
            _ASTDumperPrintCString(dumper, "'\n");
            dumper->indentation -= 1;
        } else if (constant->kind == ASTConstantKindFloat) {
            dumper->indentation += 1;
            _ASTDumperPrintIndentation(dumper);
            _ASTDumperPrintCString(dumper, "@value = '");
            Char buffer[1024];
            snprintf(&buffer[0], 1024, "%f", constant->floatValue);
            _ASTDumperPrintCString(dumper, &buffer[0]);
            _ASTDumperPrintCString(dumper, "'\n");
            dumper->indentation -= 1;
        } else if (constant->kind == ASTConstantKindString) {
            _ASTDumperPrintProperty(dumper, "value", StringGetCharacters(constant->stringValue));
        }
        return;
    }

    case ASTTagModuleDeclaration: {
        ASTModuleDeclarationRef module = (ASTModuleDeclarationRef)node;
        _ASTDumperDumpChildren(dumper, module->importedModules);
        _ASTDumperDumpChildren(dumper, module->sourceUnits);
        return;
    }

    case ASTTagEnumerationDeclaration: {
        ASTEnumerationDeclarationRef enumeration = (ASTEnumerationDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(enumeration->name));
        _ASTDumperDumpChildren(dumper, enumeration->elements);
        return;
    }

    case ASTTagFunctionDeclaration: {
        ASTFunctionDeclarationRef func = (ASTFunctionDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(func->name));
        _ASTDumperDumpChildren(dumper, func->parameters);
        _ASTDumperDumpChild(dumper, func->returnType);

        if (func->body) {
            _ASTDumperDumpChild(dumper, (ASTNodeRef)func->body);
        }
        return;
    }

    case ASTTagStructureDeclaration: {
        ASTStructureDeclarationRef structure = (ASTStructureDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(structure->name));
        _ASTDumperDumpChildren(dumper, structure->values);
        return;
    }

    case ASTTagOpaqueDeclaration: {
        ASTOpaqueDeclarationRef opaque = (ASTOpaqueDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(opaque->name));
        return;
    }

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(value->name));
        _ASTDumperDumpChild(dumper, value->type);

        if (value->initializer) {
            _ASTDumperDumpChild(dumper, value->initializer);
        }

        return;
    }

    case ASTTagOpaqueType: {
        ASTOpaqueTypeRef type = (ASTOpaqueTypeRef)node;
        _ASTDumperPrintProperty(dumper, "name", StringGetCharacters(type->name));
        return;
    }

    case ASTTagPointerType: {
        ASTPointerTypeRef type = (ASTPointerTypeRef)node;
        _ASTDumperDumpChild(dumper, type->pointeeType);
        return;
    }

    case ASTTagArrayType: {
        ASTArrayTypeRef type = (ASTArrayTypeRef)node;
        _ASTDumperDumpChild(dumper, type->elementType);

        if (type->size) {
            _ASTDumperDumpChild(dumper, type->size);
        }
        return;
    }

    case ASTTagBuiltinType: {
        ASTBuiltinTypeRef type = (ASTBuiltinTypeRef)node;

        dumper->indentation += 1;
        _ASTDumperPrintIndentation(dumper);
        _ASTDumperPrintCString(dumper, "@name = '");
        _ASTDumperPrintBuiltinTypeKind(dumper, type->kind);
        _ASTDumperPrintCString(dumper, "'\n");
        dumper->indentation -= 1;
        return;
    }

    default:
        JELLY_UNREACHABLE("Invalid tag given for ASTNode in ASTDumper!");
        return;
    }
}

static inline void _ASTDumperPrintIndentation(ASTDumperRef dumper) {
    for (Index i = 0; i < dumper->indentation; i++) {
        fprintf(dumper->target, "%s", "  ");
    }
}

static inline void _ASTDumperPrintCString(ASTDumperRef dumper, const Char *string) {
    fprintf(dumper->target, "%s", string);
}

static inline void _ASTDumperPrintProperty(ASTDumperRef dumper, const Char *key, const Char *value) {
    dumper->indentation += 1;
    _ASTDumperPrintIndentation(dumper);
    _ASTDumperPrintCString(dumper, "@");
    _ASTDumperPrintCString(dumper, key);
    _ASTDumperPrintCString(dumper, " = '");
    _ASTDumperPrintCString(dumper, value);
    _ASTDumperPrintCString(dumper, "'\n");
    dumper->indentation -= 1;
}

static inline void _ASTDumperPrintTag(ASTDumperRef dumper, ASTNodeRef node) {
    switch (node->tag) {
    case ASTTagSourceUnit:
        return _ASTDumperPrintCString(dumper, "SourceUnit");

    case ASTTagLoadDirective:
        return _ASTDumperPrintCString(dumper, "LoadDirective");

    case ASTTagBlock:
        return _ASTDumperPrintCString(dumper, "Block");

    case ASTTagIfStatement:
        return _ASTDumperPrintCString(dumper, "IfStatement");

    case ASTTagLoopStatement: {
        ASTLoopStatementRef loop = (ASTLoopStatementRef)node;
        if (loop->kind == ASTLoopKindDo) {
            return _ASTDumperPrintCString(dumper, "DoStatement");
        } else if (loop->kind == ASTLoopKindWhile) {
            return _ASTDumperPrintCString(dumper, "WhileStatement");
        }
        break;
    }

    case ASTTagCaseStatement: {
        ASTCaseStatementRef statement = (ASTCaseStatementRef)node;
        if (statement->kind == ASTCaseKindConditional) {
            return _ASTDumperPrintCString(dumper, "ConditionalCaseStatement");
        } else if (statement->kind == ASTCaseKindElse) {
            return _ASTDumperPrintCString(dumper, "ElseCaseStatement");
        }
        break;
    }

    case ASTTagSwitchStatement:
        return _ASTDumperPrintCString(dumper, "SwitchStatement");

    case ASTTagControlStatement: {
        ASTControlStatementRef control = (ASTControlStatementRef)node;
        if (control->kind == ASTControlKindBreak) {
            return _ASTDumperPrintCString(dumper, "BreakStatement");
        } else if (control->kind == ASTControlKindReturn) {
            return _ASTDumperPrintCString(dumper, "ReturnStatement");
        } else if (control->kind == ASTControlKindContinue) {
            return _ASTDumperPrintCString(dumper, "ContinueStatement");
        } else if (control->kind == ASTControlKindFallthrough) {
            return _ASTDumperPrintCString(dumper, "FallthroughStatement");
        }
        break;
    }

    case ASTTagUnaryExpression:
        return _ASTDumperPrintCString(dumper, "UnaryExpression");

    case ASTTagBinaryExpression:
        return _ASTDumperPrintCString(dumper, "BinaryExpression");

    case ASTTagIdentifierExpression:
        return _ASTDumperPrintCString(dumper, "IdentifierExpression");

    case ASTTagMemberAccessExpression:
        return _ASTDumperPrintCString(dumper, "MemberAccessExpression");

    case ASTTagCallExpression:
        return _ASTDumperPrintCString(dumper, "CallExpression");

    case ASTTagConstantExpression: {
        ASTConstantExpressionRef constant = (ASTConstantExpressionRef)node;
        if (constant->kind == ASTConstantKindNil) {
            return _ASTDumperPrintCString(dumper, "NilLiteral");
        } else if (constant->kind == ASTConstantKindBool) {
            return _ASTDumperPrintCString(dumper, "BoolLiteral");
        } else if (constant->kind == ASTConstantKindInt) {
            return _ASTDumperPrintCString(dumper, "IntLiteral");
        } else if (constant->kind == ASTConstantKindFloat) {
            return _ASTDumperPrintCString(dumper, "FloatLiteral");
        } else if (constant->kind == ASTConstantKindString) {
            return _ASTDumperPrintCString(dumper, "StringLiteral");
        }
        break;
    }

    case ASTTagModuleDeclaration:
        return _ASTDumperPrintCString(dumper, "ModuleDeclaration");

    case ASTTagEnumerationDeclaration:
        return _ASTDumperPrintCString(dumper, "EnumerationDeclaration");

    case ASTTagFunctionDeclaration:
        return _ASTDumperPrintCString(dumper, "FunctionDeclaration");

    case ASTTagStructureDeclaration:
        return _ASTDumperPrintCString(dumper, "StructureDeclaration");

    case ASTTagOpaqueDeclaration:
        return _ASTDumperPrintCString(dumper, "OpaqueDeclaration");

    case ASTTagValueDeclaration: {
        ASTValueDeclarationRef value = (ASTValueDeclarationRef)node;
        if (value->kind == ASTValueKindVariable) {
            return _ASTDumperPrintCString(dumper, "VariableDeclaration");
        } else if (value->kind == ASTValueKindParameter) {
            return _ASTDumperPrintCString(dumper, "ParameterDeclaration");
        } else if (value->kind == ASTValueKindEnumerationElement) {
            return _ASTDumperPrintCString(dumper, "EnumerationElementDeclaration");
        }
        break;
    }

    case ASTTagOpaqueType:
        return _ASTDumperPrintCString(dumper, "OpaqueType");

    case ASTTagPointerType:
        return _ASTDumperPrintCString(dumper, "PointerType");

    case ASTTagArrayType:
        return _ASTDumperPrintCString(dumper, "ArrayType");

    case ASTTagBuiltinType:
        return _ASTDumperPrintCString(dumper, "BuiltinType");

    default:
        break;
    }

    JELLY_UNREACHABLE("Invalid tag given for ASTNode in ASTDumper!");
}

static inline void _ASTDumperPrintBuiltinTypeKind(ASTDumperRef dumper, ASTBuiltinTypeKind kind) {
    switch (kind) {
    case ASTBuiltinTypeKindError:
        return _ASTDumperPrintCString(dumper, "<error>");
    case ASTBuiltinTypeKindVoid:
        return _ASTDumperPrintCString(dumper, "Void");
    case ASTBuiltinTypeKindBool:
        return _ASTDumperPrintCString(dumper, "Bool");
    case ASTBuiltinTypeKindInt8:
        return _ASTDumperPrintCString(dumper, "Int8");
    case ASTBuiltinTypeKindInt16:
        return _ASTDumperPrintCString(dumper, "Int16");
    case ASTBuiltinTypeKindInt32:
        return _ASTDumperPrintCString(dumper, "Int32");
    case ASTBuiltinTypeKindInt64:
        return _ASTDumperPrintCString(dumper, "Int64");
    case ASTBuiltinTypeKindInt128:
        return _ASTDumperPrintCString(dumper, "Int128");
    case ASTBuiltinTypeKindInt:
        return _ASTDumperPrintCString(dumper, "Int");
    case ASTBuiltinTypeKindUInt8:
        return _ASTDumperPrintCString(dumper, "UInt8");
    case ASTBuiltinTypeKindUInt16:
        return _ASTDumperPrintCString(dumper, "UInt16");
    case ASTBuiltinTypeKindUInt32:
        return _ASTDumperPrintCString(dumper, "UInt32");
    case ASTBuiltinTypeKindUInt64:
        return _ASTDumperPrintCString(dumper, "UInt64");
    case ASTBuiltinTypeKindUInt128:
        return _ASTDumperPrintCString(dumper, "UInt128");
    case ASTBuiltinTypeKindUInt:
        return _ASTDumperPrintCString(dumper, "UInt");
    case ASTBuiltinTypeKindFloat16:
        return _ASTDumperPrintCString(dumper, "Float16");
    case ASTBuiltinTypeKindFloat32:
        return _ASTDumperPrintCString(dumper, "Float32");
    case ASTBuiltinTypeKindFloat64:
        return _ASTDumperPrintCString(dumper, "Float64");
    case ASTBuiltinTypeKindFloat80:
        return _ASTDumperPrintCString(dumper, "Float80");
    case ASTBuiltinTypeKindFloat128:
        return _ASTDumperPrintCString(dumper, "Float128");
    case ASTBuiltinTypeKindFloat:
        return _ASTDumperPrintCString(dumper, "Float");

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown kind given for ASTBuiltinTypeKind in ASTDumper!");
}

static inline void _ASTDumperPrintPrefixOperator(ASTDumperRef dumper, ASTUnaryOperator op) {
    switch (op) {
    case ASTUnaryOperatorUnknown:
        return _ASTDumperPrintCString(dumper, "<unknown>");
    case ASTUnaryOperatorLogicalNot:
        return _ASTDumperPrintCString(dumper, "!");
    case ASTUnaryOperatorBitwiseNot:
        return _ASTDumperPrintCString(dumper, "~");
    case ASTUnaryOperatorUnaryPlus:
        return _ASTDumperPrintCString(dumper, "+");
    case ASTUnaryOperatorUnaryMinus:
        return _ASTDumperPrintCString(dumper, "-");

    default:
        break;
    }

    JELLY_UNREACHABLE("Unknown value given for ASTUnaryOperator in ASTDumper!");
}

static inline void _ASTDumperPrintInfixOperator(ASTDumperRef dumper, ASTBinaryOperator op) {
    switch (op) {
    case ASTBinaryOperatorUnknown:
        return _ASTDumperPrintCString(dumper, "<unknown>");
    case ASTBinaryOperatorBitwiseLeftShift:
        return _ASTDumperPrintCString(dumper, "<<");
    case ASTBinaryOperatorBitwiseRightShift:
        return _ASTDumperPrintCString(dumper, ">>");
    case ASTBinaryOperatorMultiply:
        return _ASTDumperPrintCString(dumper, "*");
    case ASTBinaryOperatorDivide:
        return _ASTDumperPrintCString(dumper, "/");
    case ASTBinaryOperatorReminder:
        return _ASTDumperPrintCString(dumper, "%");
    case ASTBinaryOperatorBitwiseAnd:
        return _ASTDumperPrintCString(dumper, "&");
    case ASTBinaryOperatorAdd:
        return _ASTDumperPrintCString(dumper, "+");
    case ASTBinaryOperatorSubtract:
        return _ASTDumperPrintCString(dumper, "-");
    case ASTBinaryOperatorBitwiseOr:
        return _ASTDumperPrintCString(dumper, "|");
    case ASTBinaryOperatorBitwiseXor:
        return _ASTDumperPrintCString(dumper, "^");
    case ASTBinaryOperatorTypeCheck:
        return _ASTDumperPrintCString(dumper, "is");
    case ASTBinaryOperatorTypeCast:
        return _ASTDumperPrintCString(dumper, "as");
    case ASTBinaryOperatorLessThan:
        return _ASTDumperPrintCString(dumper, "<");
    case ASTBinaryOperatorLessThanEqual:
        return _ASTDumperPrintCString(dumper, "<=");
    case ASTBinaryOperatorGreaterThan:
        return _ASTDumperPrintCString(dumper, ">");
    case ASTBinaryOperatorGreaterThanEqual:
        return _ASTDumperPrintCString(dumper, ">=");
    case ASTBinaryOperatorEqual:
        return _ASTDumperPrintCString(dumper, "==");
    case ASTBinaryOperatorNotEqual:
        return _ASTDumperPrintCString(dumper, "!=");
    case ASTBinaryOperatorLogicalAnd:
        return _ASTDumperPrintCString(dumper, "&&");
    case ASTBinaryOperatorLogicalOr:
        return _ASTDumperPrintCString(dumper, "||");
    case ASTBinaryOperatorAssign:
        return _ASTDumperPrintCString(dumper, "=");
    case ASTBinaryOperatorMultiplyAssign:
        return _ASTDumperPrintCString(dumper, "*=");
    case ASTBinaryOperatorDivideAssign:
        return _ASTDumperPrintCString(dumper, "/=");
    case ASTBinaryOperatorReminderAssign:
        return _ASTDumperPrintCString(dumper, "%=");
    case ASTBinaryOperatorAddAssign:
        return _ASTDumperPrintCString(dumper, "+=");
    case ASTBinaryOperatorSubtractAssign:
        return _ASTDumperPrintCString(dumper, "-=");
    case ASTBinaryOperatorBitwiseLeftShiftAssign:
        return _ASTDumperPrintCString(dumper, "<<=");
    case ASTBinaryOperatorBitwiseRightShiftAssign:
        return _ASTDumperPrintCString(dumper, ">>=");
    case ASTBinaryOperatorBitwiseAndAssign:
        return _ASTDumperPrintCString(dumper, "&=");
    case ASTBinaryOperatorBitwiseOrAssign:
        return _ASTDumperPrintCString(dumper, "|=");
    case ASTBinaryOperatorBitwiseXorAssign:
        return _ASTDumperPrintCString(dumper, "^=");

    default:
        break;
    }
}

static inline void _ASTDumperDumpChild(ASTDumperRef dumper, ASTNodeRef child) {
    dumper->indentation += 1;
    ASTDumperDump(dumper, child);
    dumper->indentation -= 1;
}

static inline void _ASTDumperDumpChildren(ASTDumperRef dumper, ASTLinkedListRef list) {
    ASTLinkedListRef next = list;
    while (next) {
        _ASTDumperDumpChild(dumper, next->node);
        next = next->next;
    }
}
