#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/BumpAllocator.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Lexer.h"
#include "JellyCore/Parser.h"
#include "JellyCore/SourceRange.h"

// TODO: Add missing scopes and add all declarations to scopes!
// TODO: Write tests for correct scope creation and population!

struct _Parser {
    AllocatorRef allocator;
    AllocatorRef tempAllocator;
    ASTContextRef context;
    LexerRef lexer;
    Token token;
};

static inline Bool _StringIsValidFilePath(StringRef string);
static inline Bool _SourceRangeContainsLineBreakCharacter(SourceRange range);

static inline Bool _ParserIsToken(ParserRef parser, TokenKind kind);
static inline Bool _ParserConsumeToken(ParserRef parser, TokenKind kind);

static inline StringRef _ParserConsumeIdentifier(ParserRef parser);
static inline ASTUnaryOperator _ParserConsumeUnaryOperator(ParserRef parser);
static inline ASTBinaryOperator _ParserConsumeBinaryOperator(ParserRef parser);
static inline ASTPostfixOperator _ParserConsumePostfixOperatorHead(ParserRef parser);
static inline Bool _ParserConsumePostfixOperatorTail(ParserRef parser, ASTPostfixOperator postfix);
static inline ASTLoadDirectiveRef _ParserParseDirective(ParserRef parser);
static inline ASTBlockRef _ParserParseBlock(ParserRef parser);
static inline ASTIfStatementRef _ParserParseIfStatement(ParserRef parser);
static inline ASTLoopStatementRef _ParserParseLoopStatement(ParserRef parser);
static inline ASTCaseStatementRef _ParserParseCaseStatement(ParserRef parser);
static inline ASTSwitchStatementRef _ParserParseSwitchStatement(ParserRef parser);
static inline ASTControlStatementRef _ParserParseControlStatement(ParserRef parser);
static inline ASTNodeRef _ParserParseStatement(ParserRef parser);
static inline ASTExpressionRef _ParserParsePrimaryExpression(ParserRef parser);
static inline ASTUnaryExpressionRef _ParserParseUnaryExpression(ParserRef parser, ASTUnaryOperator unary, SourceRange location);
static inline ASTExpressionRef _ParserParseExpression(ParserRef parser, ASTOperatorPrecedence minPrecedence, Bool silentDiagnostics);
static inline ASTIdentifierExpressionRef _ParserParseIdentifierExpression(ParserRef parser);
static inline ASTCallExpressionRef _ParserParseCallExpression(ParserRef parser, ASTExpressionRef callee);
static inline ASTConstantExpressionRef _ParserParseConstantExpression(ParserRef parser);
static inline ASTEnumerationDeclarationRef _ParserParseEnumerationDeclaration(ParserRef parser);
static inline ASTFunctionDeclarationRef _ParserParseFunctionDeclaration(ParserRef parser);
static inline ASTStructureDeclarationRef _ParserParseStructureDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseVariableDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseEnumerationElementDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseParameterDeclaration(ParserRef parser);
static inline ASTTypeRef _ParserParseType(ParserRef parser);
static inline ASTExpressionRef _ParserParseConditionList(ParserRef parser);
static inline ASTNodeRef _ParserParseTopLevelNode(ParserRef parser);

ParserRef ParserCreate(AllocatorRef allocator, ASTContextRef context) {
    ParserRef parser  = AllocatorAllocate(allocator, sizeof(struct _Parser));
    parser->allocator = allocator;
    // TODO: Replace tempAllocator with a pool allocator which resets after finishing a top level parse action.
    parser->tempAllocator = BumpAllocatorCreate(allocator);
    parser->context       = context;
    parser->lexer         = NULL;
    return parser;
}

void ParserDestroy(ParserRef parser) {
    if (parser->lexer) {
        LexerDestroy(parser->lexer);
    }

    AllocatorDestroy(parser->tempAllocator);
    AllocatorDeallocate(parser->allocator, parser);
}

ASTSourceUnitRef ParserParseSourceUnit(ParserRef parser, StringRef filePath, StringRef source) {
    ASTModuleDeclarationRef module = ASTContextGetModule(parser->context);

    parser->lexer = LexerCreate(parser->allocator, source);
    LexerNextToken(parser->lexer, &parser->token);

    SourceRange location  = parser->token.location;
    ArrayRef declarations = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (true) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef declaration    = _ParserParseTopLevelNode(parser);
        if (!declaration) {
            break;
        }

        if (ArrayGetElementCount(declarations) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(declarations, &declaration);
    }

    location.end                = parser->token.location.start;
    ASTSourceUnitRef sourceUnit = ASTContextCreateSourceUnit(parser->context, location, filePath, declarations);
    ASTModuleAddSourceUnit(parser->context, module, sourceUnit);
    return sourceUnit;
}

static inline Bool _StringIsValidFilePath(StringRef string) {
    // TODO: Implement check for file path validation
    return true;
}

static inline Bool _SourceRangeContainsLineBreakCharacter(SourceRange range) {
    for (const Char *cursor = range.start; cursor < range.end; cursor++) {
        switch (*cursor) {
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            return true;

        default:
            break;
        }
    }

    return false;
}

static inline Bool _ParserIsToken(ParserRef parser, TokenKind kind) {
    return parser->token.kind == kind;
}

static inline Bool _ParserConsumeToken(ParserRef parser, TokenKind kind) {
    if (parser->token.kind == kind) {
        LexerNextToken(parser->lexer, &parser->token);
        return true;
    }

    return false;
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static inline StringRef _ParserConsumeIdentifier(ParserRef parser) {
    if (parser->token.kind == TokenKindIdentifier) {
        StringRef result = StringCreateRange(parser->tempAllocator, parser->token.location.start, parser->token.location.end);
        LexerNextToken(parser->lexer, &parser->token);
        return result;
    }

    return NULL;
}

static inline ASTUnaryOperator _ParserConsumeUnaryOperator(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindExclamationMark)) {
        return ASTUnaryOperatorLogicalNot;
    } else if (_ParserConsumeToken(parser, TokenKindTilde)) {
        return ASTUnaryOperatorBitwiseNot;
    } else if (_ParserConsumeToken(parser, TokenKindPlusSign)) {
        return ASTUnaryOperatorUnaryPlus;
    } else if (_ParserConsumeToken(parser, TokenKindMinusSign)) {
        return ASTUnaryOperatorUnaryMinus;
    } else {
        return ASTUnaryOperatorUnknown;
    }
}

static inline ASTBinaryOperator _ParserConsumeBinaryOperator(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindLessThanLessThan)) {
        return ASTBinaryOperatorBitwiseLeftShift;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThanGreaterThan)) {
        return ASTBinaryOperatorBitwiseRightShift;
    } else if (_ParserConsumeToken(parser, TokenKindAsterisk)) {
        return ASTBinaryOperatorMultiply;
    } else if (_ParserConsumeToken(parser, TokenKindSlash)) {
        return ASTBinaryOperatorDivide;
    } else if (_ParserConsumeToken(parser, TokenKindPercentSign)) {
        return ASTBinaryOperatorReminder;
    } else if (_ParserConsumeToken(parser, TokenKindAmpersand)) {
        return ASTBinaryOperatorBitwiseAnd;
    } else if (_ParserConsumeToken(parser, TokenKindPlusSign)) {
        return ASTBinaryOperatorAdd;
    } else if (_ParserConsumeToken(parser, TokenKindMinusSign)) {
        return ASTBinaryOperatorSubtract;
    } else if (_ParserConsumeToken(parser, TokenKindPipe)) {
        return ASTBinaryOperatorBitwiseOr;
    } else if (_ParserConsumeToken(parser, TokenKindCircumflex)) {
        return ASTBinaryOperatorBitwiseXor;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordIs)) {
        return ASTBinaryOperatorTypeCheck;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordAs)) {
        return ASTBinaryOperatorTypeCast;
    } else if (_ParserConsumeToken(parser, TokenKindLessThan)) {
        return ASTBinaryOperatorLessThan;
    } else if (_ParserConsumeToken(parser, TokenKindLessThanEqualsSign)) {
        return ASTBinaryOperatorLessThanEqual;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThan)) {
        return ASTBinaryOperatorGreaterThan;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThanEqualsSign)) {
        return ASTBinaryOperatorGreaterThanEqual;
    } else if (_ParserConsumeToken(parser, TokenKindEqualsEqualsSign)) {
        return ASTBinaryOperatorEqual;
    } else if (_ParserConsumeToken(parser, TokenKindExclamationMarkEqualsSign)) {
        return ASTBinaryOperatorNotEqual;
    } else if (_ParserConsumeToken(parser, TokenKindAmpersandAmpersand)) {
        return ASTBinaryOperatorLogicalAnd;
    } else if (_ParserConsumeToken(parser, TokenKindPipePipe)) {
        return ASTBinaryOperatorLogicalOr;
    } else if (_ParserConsumeToken(parser, TokenKindEqualsSign)) {
        return ASTBinaryOperatorAssign;
    } else if (_ParserConsumeToken(parser, TokenKindAsteriskEquals)) {
        return ASTBinaryOperatorMultiplyAssign;
    } else if (_ParserConsumeToken(parser, TokenKindSlashEquals)) {
        return ASTBinaryOperatorDivideAssign;
    } else if (_ParserConsumeToken(parser, TokenKindPercentEquals)) {
        return ASTBinaryOperatorReminderAssign;
    } else if (_ParserConsumeToken(parser, TokenKindPlusEquals)) {
        return ASTBinaryOperatorAddAssign;
    } else if (_ParserConsumeToken(parser, TokenKindMinusEqualsSign)) {
        return ASTBinaryOperatorSubtractAssign;
    } else if (_ParserConsumeToken(parser, TokenKindLessThanLessThanEquals)) {
        return ASTBinaryOperatorBitwiseLeftShiftAssign;
    } else if (_ParserConsumeToken(parser, TokenKindGreaterThanGreaterThanEquals)) {
        return ASTBinaryOperatorBitwiseRightShiftAssign;
    } else if (_ParserConsumeToken(parser, TokenKindAmpersandEquals)) {
        return ASTBinaryOperatorBitwiseAndAssign;
    } else if (_ParserConsumeToken(parser, TokenKindPipeEquals)) {
        return ASTBinaryOperatorBitwiseOrAssign;
    } else if (_ParserConsumeToken(parser, TokenKindCircumflexEquals)) {
        return ASTBinaryOperatorBitwiseXorAssign;
    } else {
        return ASTBinaryOperatorUnknown;
    }
}

static inline ASTPostfixOperator _ParserConsumePostfixOperatorHead(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindDot)) {
        return ASTPostfixOperatorSelector;
    } else if (_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        return ASTPostfixOperatorCall;
    } else {
        return ASTPostfixOperatorUnknown;
    }
}

static inline Bool _ParserConsumePostfixOperatorTail(ParserRef parser, ASTPostfixOperator postfix) {
    switch (postfix) {
    case ASTPostfixOperatorCall:
        return _ParserConsumeToken(parser, TokenKindRightParenthesis);

    case ASTPostfixOperatorUnknown:
    case ASTPostfixOperatorSelector:
        return true;

    default:
        return false;
    }
}

/// grammar: directive      := load-directive
/// grammar: load-directive := "#load" string-literal
static inline ASTLoadDirectiveRef _ParserParseDirective(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindDirectiveLoad)) {
        ASTConstantExpressionRef filePath = _ParserParseConstantExpression(parser);
        if (!filePath || filePath->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#load` directive");
            return NULL;
        }

        assert(filePath->kind == ASTConstantKindString);

        if (!_StringIsValidFilePath(filePath->stringValue)) {
            ReportError("Expected valid file path after `#load` directive");
            return NULL;
        }

        location.end = parser->token.location.start;
        return ASTContextCreateLoadDirective(parser->context, location, filePath);
    }

    ReportError("Unknown compiler directive");
    return NULL;
}

/// grammar: block := '{' { statement } '}'
static inline ASTBlockRef _ParserParseBlock(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' at start of `block-statement`");
        return NULL;
    }

    ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef statement      = _ParserParseStatement(parser);
        if (!statement) {
            return NULL;
        }

        if (ArrayGetElementCount(statements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(statements, &statement);
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateBlock(parser->context, location, statements);
}

/// grammar: if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]
static inline ASTIfStatementRef _ParserParseIfStatement(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordIf)) {
        return NULL;
    }

    ASTExpressionRef condition = _ParserParseConditionList(parser);
    if (!condition) {
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SymbolTablePushScope(symbolTable, ScopeKindBranch);

    ASTBlockRef thenBlock = _ParserParseBlock(parser);
    if (!thenBlock) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    ASTBlockRef elseBlock = NULL;
    if (_ParserConsumeToken(parser, TokenKindKeywordElse)) {
        SymbolTablePushScope(symbolTable, ScopeKindBranch);

        location.end = parser->token.location.start;
        if (_ParserIsToken(parser, TokenKindKeywordIf)) {
            SourceRange location          = parser->token.location;
            ASTIfStatementRef ifStatement = _ParserParseIfStatement(parser);
            if (!ifStatement) {
                return NULL;
            }

            ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 1);
            ArrayAppendElement(statements, &ifStatement);

            location.end = parser->token.location.start;
            elseBlock    = ASTContextCreateBlock(parser->context, location, statements);
        } else {
            elseBlock = _ParserParseBlock(parser);
            if (!elseBlock) {
                return NULL;
            }
        }

        SymbolTablePopScope(symbolTable);
    } else {
        elseBlock = ASTContextCreateBlock(parser->context, location, NULL);
    }

    location.end = parser->token.location.start;
    return ASTContextCreateIfStatement(parser->context, location, condition, thenBlock, elseBlock);
}

/// grammar: loop-statement  := while-statement | do-statement
/// grammar: while-statement := "while" expression { "," expression } block
/// grammar: do-statement    := "do" block "while" expression
static inline ASTLoopStatementRef _ParserParseLoopStatement(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindKeywordWhile)) {
        ASTExpressionRef condition = _ParserParseConditionList(parser);
        if (!condition) {
            return NULL;
        }

        SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
        SymbolTablePushScope(symbolTable, ScopeKindLoop);

        ASTBlockRef loopBlock = _ParserParseBlock(parser);
        if (!loopBlock) {

            return NULL;
        }

        SymbolTablePopScope(symbolTable);

        location.end = parser->token.location.start;
        return ASTContextCreateLoopStatement(parser->context, location, ASTLoopKindWhile, condition, loopBlock);
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordDo)) {
        SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
        SymbolTablePushScope(symbolTable, ScopeKindLoop);

        ASTBlockRef loopBlock = _ParserParseBlock(parser);
        if (!loopBlock) {
            return NULL;
        }

        if (!_ParserConsumeToken(parser, TokenKindKeywordWhile)) {
            ReportError("Expected 'while' after block of 'do' statement");
            return NULL;
        }

        ASTExpressionRef condition = _ParserParseConditionList(parser);
        if (!condition) {
            return NULL;
        }

        SymbolTablePopScope(symbolTable);

        location.end = parser->token.location.start;
        return ASTContextCreateLoopStatement(parser->context, location, ASTLoopKindDo, condition, loopBlock);
    }

    ReportError("Expected 'while' or 'do' at start of loop-statement!");
    return NULL;
}

/// grammar: case-statement             := conditional-case-statement | else-case-statement
/// grammar: conditional-case-statement := "case" expression ":" statement { line-break statement }
/// grammar: else-case-statement        := "else" ":" statement { line-break statement }
static inline ASTCaseStatementRef _ParserParseCaseStatement(ParserRef parser) {
    SourceRange location = parser->token.location;

    ASTCaseKind kind           = ASTCaseKindElse;
    ASTExpressionRef condition = NULL;
    if (_ParserConsumeToken(parser, TokenKindKeywordCase)) {
        kind      = ASTCaseKindConditional;
        condition = _ParserParseExpression(parser, 0, false);
        if (!condition) {
            return NULL;
        }
    } else if (!_ParserConsumeToken(parser, TokenKindKeywordElse)) {
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindColon)) {
        if (kind == ASTCaseKindConditional) {
            ReportError("Expected ':' after 'case'");
        } else {
            ReportError("Expected ':' after 'else'");
        }
        return NULL;
    }

    SourceRange blockLocation  = parser->token.location;
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SymbolTablePushScope(symbolTable, ScopeKindCase);

    ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindKeywordCase) && !_ParserIsToken(parser, TokenKindKeywordElse) &&
           !_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia = parser->token.leadingTrivia;
        ASTNodeRef statement      = _ParserParseStatement(parser);
        if (!statement) {
            return NULL;
        }

        if (ArrayGetElementCount(statements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(statements, &statement);
    }

    if (ArrayGetElementSize(statements) < 1) {
        ReportError("case statement in a switch should contain at least one statement");
    }

    SymbolTablePopScope(symbolTable);

    blockLocation.end = parser->token.location.start;
    ASTBlockRef body  = ASTContextCreateBlock(parser->context, blockLocation, statements);

    location.end = parser->token.location.start;
    return ASTContextCreateCaseStatement(parser->context, location, kind, condition, body);
}

/// grammar: switch-statement := "switch" expression "{" [ case-statement { line-break case-statement } ] "}"
static inline ASTSwitchStatementRef _ParserParseSwitchStatement(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordSwitch)) {
        return NULL;
    }

    ASTExpressionRef argument = _ParserParseExpression(parser, 0, true);
    if (!argument) {
        ReportError("Expected expression in 'switch' statement");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' at start of 'switch' statement");
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SymbolTablePushScope(symbolTable, ScopeKindSwitch);

    ArrayRef statements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        ReportError("'switch' statement body must have at least one 'case' or 'else' block");
    }

    while (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        SourceRange leadingTrivia     = parser->token.leadingTrivia;
        ASTCaseStatementRef statement = _ParserParseCaseStatement(parser);
        if (!statement) {
            if (ArrayGetElementCount(statements) > 0) {
                ReportError("All statements inside a switch must be covered by a 'case' or 'else'");
            }

            break;
        }

        if (ArrayGetElementCount(statements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
            ReportError("Consecutive statements on a line are not allowed");
        }

        ArrayAppendElement(statements, &statement);
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end = parser->token.location.start;
    return ASTContextCreateSwitchStatement(parser->context, location, argument, statements);
}

/// grammar: control-statement      := break-statement | continue-statement | fallthrough-statement | return-statement
/// grammar: break-statement        := "break"
/// grammar: continue-statement     := "continue"
/// grammar: fallthrough-statement  := "fallthrough"
/// grammar: return-statement       := "return" [ expression ]
static inline ASTControlStatementRef _ParserParseControlStatement(ParserRef parser) {
    SourceRange location = parser->token.location;

    ASTControlKind kind;
    ASTExpressionRef result = NULL;
    if (_ParserConsumeToken(parser, TokenKindKeywordBreak)) {
        kind = ASTControlKindBreak;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordContinue)) {
        kind = ASTControlKindContinue;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFallthrough)) {
        kind = ASTControlKindFallthrough;
    } else if (_ParserConsumeToken(parser, TokenKindKeywordReturn)) {
        kind = ASTControlKindReturn;

        LexerStateRef state = LexerGetState(parser->lexer);
        result              = _ParserParseExpression(parser, 0, true);
        if (!result) {
            LexerSetState(parser->lexer, state);
            LexerPeekToken(parser->lexer, &parser->token);
        }
    } else {
        ReportError("Expected 'break', 'continue', 'fallthrough' or 'return' at start of control-statement!");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateControlStatement(parser->context, location, kind, result);
}

/// grammar: statement := variable-declaration | control-statement | loop-statement | if-statement | switch-statement | expression
static inline ASTNodeRef _ParserParseStatement(ParserRef parser) {
    if (_ParserIsToken(parser, TokenKindKeywordVar)) {
        ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
        if (!value) {
            return NULL;
        }

        assert(value->kind == ASTValueKindVariable);

        return (ASTNodeRef)value;
    }

    if (_ParserIsToken(parser, TokenKindKeywordBreak) || _ParserIsToken(parser, TokenKindKeywordContinue) ||
        _ParserIsToken(parser, TokenKindKeywordFallthrough) || _ParserIsToken(parser, TokenKindKeywordReturn)) {
        return (ASTNodeRef)_ParserParseControlStatement(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordDo) || _ParserIsToken(parser, TokenKindKeywordWhile)) {
        return (ASTNodeRef)_ParserParseLoopStatement(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordIf)) {
        return (ASTNodeRef)_ParserParseIfStatement(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordSwitch)) {
        return (ASTNodeRef)_ParserParseSwitchStatement(parser);
    }

    ASTExpressionRef expression = _ParserParseExpression(parser, 0, true);
    if (!expression) {
        ReportError("Expected statement");
        return NULL;
    }

    return (ASTNodeRef)expression;
}

/// grammar: atom-expression       := group-expression | literal-expression | identifier-expression
/// grammar: group-expression      := "(" expression ")"
/// grammar: literal-expression    := literal
/// grammar: identifier-expression := identifier
static inline ASTExpressionRef _ParserParseAtomExpression(ParserRef parser) {
    if (_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ASTExpressionRef expression = _ParserParseExpression(parser, 0, true);
        if (!expression) {
            return NULL;
        }

        if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
            return NULL;
        }

        return expression;
    }

    if (_ParserIsToken(parser, TokenKindKeywordNil) || _ParserIsToken(parser, TokenKindKeywordTrue) ||
        _ParserIsToken(parser, TokenKindKeywordFalse) || _ParserIsToken(parser, TokenKindLiteralInt) ||
        _ParserIsToken(parser, TokenKindLiteralFloat) || _ParserIsToken(parser, TokenKindLiteralString)) {
        return (ASTExpressionRef)_ParserParseConstantExpression(parser);
    }

    return (ASTExpressionRef)_ParserParseIdentifierExpression(parser);
}

/// grammar: primary-expression := unary-expression | atom-expression
static inline ASTExpressionRef _ParserParsePrimaryExpression(ParserRef parser) {
    SourceRange location   = parser->token.location;
    ASTUnaryOperator unary = _ParserConsumeUnaryOperator(parser);
    if (unary != ASTUnaryOperatorUnknown) {
        return (ASTExpressionRef)_ParserParseUnaryExpression(parser, unary, location);
    }

    return _ParserParseAtomExpression(parser);
}

/// grammar: unary-expression := prefix-operator expression
/// grammar: prefix-operator  := '!' | '~' | '+' | '-'
static inline ASTUnaryExpressionRef _ParserParseUnaryExpression(ParserRef parser, ASTUnaryOperator unary, SourceRange location) {
    assert(unary != ASTUnaryOperatorUnknown);

    ASTExpressionRef arguments[1];
    arguments[0] = _ParserParsePrimaryExpression(parser);
    if (!arguments[0]) {
        return NULL;
    }

    if (location.end != arguments[0]->location.start) {
        ReportError("Unary operator cannot be separated from its operand");
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateUnaryExpression(parser->context, location, unary, arguments);
}

/// grammar: expression        := binary-expression | primary-expression
/// grammar: binary-expression := primary-expression infix-operator expression
static inline ASTExpressionRef _ParserParseExpression(ParserRef parser, ASTOperatorPrecedence minPrecedence, Bool silentDiagnostics) {
    SourceRange location = parser->token.location;

    // TODO: Source location of expressions are not initialized correctly!

    ASTExpressionRef result = _ParserParsePrimaryExpression(parser);
    if (!result) {
        if (!silentDiagnostics) {
            ReportError("Expected expression");
        }

        return NULL;
    }

    location.end                     = parser->token.location.start;
    LexerStateRef state              = LexerGetState(parser->lexer);
    ASTBinaryOperator binary         = _ParserConsumeBinaryOperator(parser);
    ASTOperatorPrecedence precedence = ASTGetBinaryOperatorPrecedence(binary);

    ASTPostfixOperator postfix = ASTPostfixOperatorUnknown;
    if (binary == ASTBinaryOperatorUnknown) {
        postfix    = _ParserConsumePostfixOperatorHead(parser);
        precedence = ASTGetPostfixOperatorPrecedence(postfix);
    }

    if (binary == ASTBinaryOperatorUnknown && postfix == ASTPostfixOperatorUnknown) {
        return result;
    }

    if (minPrecedence >= precedence) {
        LexerSetState(parser->lexer, state);
        LexerPeekToken(parser->lexer, &parser->token);
        return result;
    }

    while (minPrecedence < precedence) {
        ASTOperatorPrecedence nextPrecedence = precedence;

        if (binary != ASTBinaryOperatorUnknown) {
            ASTOperatorAssociativity associativity = ASTGetBinaryOperatorAssociativity(binary);
            if (associativity == ASTOperatorAssociativityRight) {
                nextPrecedence = ASTGetOperatorPrecedenceBefore(nextPrecedence);
            }

            ASTExpressionRef right = _ParserParseExpression(parser, nextPrecedence, silentDiagnostics);
            if (!right) {
                return NULL;
            }

            location.end                  = parser->token.location.start;
            ASTExpressionRef arguments[2] = {result, right};
            result = (ASTExpressionRef)ASTContextCreateBinaryExpression(parser->context, location, binary, arguments);
            assert(result);
        } else if (postfix == ASTPostfixOperatorSelector) {
            StringRef memberName = _ParserConsumeIdentifier(parser);
            if (!memberName) {
                return NULL;
            }

            location.end = parser->token.location.start;
            result       = (ASTExpressionRef)ASTContextCreateMemberAccessExpression(parser->context, location, result, memberName);
        } else if (postfix == ASTPostfixOperatorCall) {
            result = (ASTExpressionRef)_ParserParseCallExpression(parser, result);
            assert(result);
        } else {
            return NULL;
        }

        LexerStateRef state = LexerGetState(parser->lexer);
        binary              = _ParserConsumeBinaryOperator(parser);
        precedence          = ASTGetBinaryOperatorPrecedence(binary);
        postfix             = ASTPostfixOperatorUnknown;

        if (binary == ASTBinaryOperatorUnknown) {
            postfix    = _ParserConsumePostfixOperatorHead(parser);
            precedence = ASTGetPostfixOperatorPrecedence(postfix);
        }

        if (binary == ASTBinaryOperatorUnknown && postfix == ASTPostfixOperatorUnknown) {
            return result;
        }

        if (minPrecedence >= precedence) {
            LexerSetState(parser->lexer, state);
            LexerPeekToken(parser->lexer, &parser->token);
            return result;
        }
    }

    return result;
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static inline ASTIdentifierExpressionRef _ParserParseIdentifierExpression(ParserRef parser) {
    SourceRange location = parser->token.location;
    StringRef name       = _ParserConsumeIdentifier(parser);
    if (!name) {
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateIdentifierExpression(parser->context, location, name);
}

/// grammar: call-expression := expression "(" [ expression { "," expression } ] ")"
static inline ASTCallExpressionRef _ParserParseCallExpression(ParserRef parser, ASTExpressionRef callee) {
    SourceRange location = {callee->location.start, parser->token.location.start};

    // We expect that the postfix operator head is already consumed earlier
    //    if (!_ParserConsumePostfixOperatorHead(parser)) {
    //        return NULL;
    //    }

    ArrayRef arguments = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    while (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        ASTExpressionRef argument = _ParserParseExpression(parser, 0, false);
        if (!argument) {
            return NULL;
        }

        ArrayAppendElement(arguments, &argument);

        if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
            break;
        }

        if (!_ParserConsumeToken(parser, TokenKindComma)) {
            return NULL;
        }
    }

    if (!_ParserConsumePostfixOperatorTail(parser, ASTPostfixOperatorCall)) {
        return NULL;
    }

    location.end = parser->token.location.start;
    return ASTContextCreateCallExpression(parser->context, location, callee, arguments);
}

/// grammar: constant-expression := nil-literal | bool-literal | numeric-literal | string-literal
/// grammar: nil-literal         := "nil"
/// grammar: bool-literal        := "true" | "false"
static inline ASTConstantExpressionRef _ParserParseConstantExpression(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (_ParserConsumeToken(parser, TokenKindKeywordNil)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantNilExpression(parser->context, location);
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordTrue)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantBoolExpression(parser->context, location, true);
    }

    if (_ParserConsumeToken(parser, TokenKindKeywordFalse)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantBoolExpression(parser->context, location, false);
    }

    UInt64 intValue = parser->token.intValue;
    if (_ParserConsumeToken(parser, TokenKindLiteralInt)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantIntExpression(parser->context, location, intValue);
    }

    Float64 floatValue = parser->token.floatValue;
    if (_ParserConsumeToken(parser, TokenKindLiteralFloat)) {
        location.end = parser->token.location.start;
        return ASTContextCreateConstantFloatExpression(parser->context, location, floatValue);
    }

    if (_ParserIsToken(parser, TokenKindLiteralString)) {
        assert(parser->token.location.end - parser->token.location.start >= 2);
        StringRef value = StringCreateRange(parser->tempAllocator, parser->token.location.start + 1, parser->token.location.end - 1);
        _ParserConsumeToken(parser, TokenKindLiteralString);
        location.end = parser->token.location.start;
        return ASTContextCreateConstantStringExpression(parser->context, location, value);
    }

    return NULL;
}

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
static inline ASTEnumerationDeclarationRef _ParserParseEnumerationDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;
    if (!_ParserConsumeToken(parser, TokenKindKeywordEnum)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected `name` of `enum-declaration`");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' after `name` of `enum-declaration`");
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope             = SymbolTablePushScope(symbolTable, ScopeKindEnumeration);

    ArrayRef elements = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);
    if (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        while (true) {
            SourceRange leadingTrivia      = parser->token.leadingTrivia;
            ASTValueDeclarationRef element = _ParserParseEnumerationElementDeclaration(parser);
            if (!element) {
                return NULL;
            }

            assert(element->kind == ASTValueKindEnumerationElement);

            if (ArrayGetElementCount(elements) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
                ReportError("Consecutive statements on a line are not allowed");
            }

            if (element->kind != ASTValueKindEnumerationElement) {
                ReportError("Only `enum-element`(s) are allowed inside of `enum-declaration`");
                return NULL;
            }

            ArrayAppendElement(elements, &element);

            if (_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
                break;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end = parser->token.location.start;
    return ASTContextCreateEnumerationDeclaration(parser->context, location, name, elements);
}

/// grammar: func-declaration := "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier block
static inline ASTFunctionDeclarationRef _ParserParseFunctionDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordFunc)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftParenthesis)) {
        ReportError("Expected parameter list after name of 'func'");
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope             = SymbolTablePushScope(symbolTable, ScopeKindFunction);
    ArrayRef parameters        = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightParenthesis)) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseParameterDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            assert(parameter->kind == ASTValueKindParameter);

            ArrayAppendElement(parameters, &parameter);

            if (_ParserIsToken(parser, TokenKindRightParenthesis)) {
                break;
            }

            if (!_ParserConsumeToken(parser, TokenKindComma)) {
                ReportError("Expected ',' or ')' in parameter list of 'func'");
                return NULL;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightParenthesis)) {
        ReportError("Expected ')' after parameter list of 'func'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindArrow)) {
        ReportError("Expected '->' after parameter list of 'func'");
        return NULL;
    }

    ASTTypeRef returnType = _ParserParseType(parser);
    if (!returnType) {
        ReportError("Expected type for function result");
        return NULL;
    }

    ASTBlockRef body = _ParserParseBlock(parser);
    if (!body) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end                          = parser->token.location.start;
    ASTFunctionDeclarationRef declaration = ASTContextCreateFunctionDeclaration(parser->context, location, name, parameters, returnType,
                                                                                body);

    SymbolRef symbol = ScopeInsertSymbol(SymbolTableGetCurrentScope(symbolTable), name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier");
        return NULL;
    }

    return declaration;
}

/// grammar: struct-declaration := "struct" identifier "{" { value-declaration } "}"
static inline ASTStructureDeclarationRef _ParserParseStructureDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordStruct)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected `name` of `struct-declaration`");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindLeftCurlyBracket)) {
        ReportError("Expected '{' after name of `struct-declaration`");
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope             = SymbolTablePushScope(symbolTable, ScopeKindStructure);
    ArrayRef values            = ArrayCreateEmpty(parser->tempAllocator, sizeof(ASTNodeRef), 8);

    if (!_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
        while (true) {
            SourceRange leadingTrivia    = parser->token.leadingTrivia;
            ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
            if (!value) {
                return NULL;
            }

            assert(value->kind == ASTValueKindVariable);

            if (ArrayGetElementCount(values) > 0 && !_SourceRangeContainsLineBreakCharacter(leadingTrivia)) {
                ReportError("Consecutive statements on a line are not allowed");
            }

            ArrayAppendElement(values, &value);

            if (_ParserIsToken(parser, TokenKindRightCurlyBracket)) {
                break;
            }
        }
    }

    if (!_ParserConsumeToken(parser, TokenKindRightCurlyBracket)) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end                           = parser->token.location.start;
    ASTStructureDeclarationRef declaration = ASTContextCreateStructureDeclaration(parser->context, location, name, values);

    SymbolRef symbol = ScopeInsertSymbol(SymbolTableGetCurrentScope(symbolTable), name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier");
        return NULL;
    }

    return declaration;
}

/// grammar: variable-declaration := "var" identifier ":" type-identifier [ "=" expression ]
static inline ASTValueDeclarationRef _ParserParseVariableDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordVar)) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of variable declaration");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindColon)) {
        ReportError("Expected ':' after name of variable declaration");
        return NULL;
    }

    ASTTypeRef type = _ParserParseType(parser);
    if (!type) {
        ReportError("Expected type of variable declaration");
        return NULL;
    }

    ASTExpressionRef initializer = NULL;
    ASTBinaryOperator binary     = _ParserConsumeBinaryOperator(parser);
    if (binary == ASTBinaryOperatorAssign) {
        initializer = _ParserParseExpression(parser, 0, true);
        if (!initializer) {
            ReportError("Expected expression");
            return NULL;
        }
    } else if (binary != ASTBinaryOperatorUnknown) {
        ReportError("Unexpected binary operator found!");
        return NULL;
    }

    location.end                       = parser->token.location.start;
    ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindVariable, name, type,
                                                                          initializer);
    SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
    SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier");
    }

    return declaration;
}

/// grammar: enumeration-element-declaration := "case" identifier [ "=" expression ]
static inline ASTValueDeclarationRef _ParserParseEnumerationElementDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    if (!_ParserConsumeToken(parser, TokenKindKeywordCase)) {
        // We assume that we are in a enumeration declaration here...
        ReportError("Expected 'case' or '}' in enumeration declaration");
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected name of enumeration element");
        return NULL;
    }

    ASTExpressionRef initializer = NULL;
    ASTBinaryOperator binary     = _ParserConsumeBinaryOperator(parser);
    if (binary == ASTBinaryOperatorAssign) {
        initializer = _ParserParseExpression(parser, 0, true);
        if (!initializer) {
            ReportError("Expected expression after '='");
            return NULL;
        }
    } else if (binary != ASTBinaryOperatorUnknown) {
        ReportError("Unexpected binary operator found!");
        return NULL;
    }

    ASTTypeRef type = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt);

    location.end                       = parser->token.location.start;
    ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindEnumerationElement, name,
                                                                          type, initializer);
    SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
    SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier");
    }

    return declaration;
}

/// grammar: parameter-declaration := identifier ":" type-identifier
static inline ASTValueDeclarationRef _ParserParseParameterDeclaration(ParserRef parser) {
    SourceRange location = parser->token.location;

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        ReportError("Expected parameter name followed by ':'");
        return NULL;
    }

    if (!_ParserConsumeToken(parser, TokenKindColon)) {
        ReportError("Expected ':' after name of parameter");
        return NULL;
    }

    ASTTypeRef type = _ParserParseType(parser);
    if (!type) {
        ReportError("Expected type of parameter");
        return NULL;
    }

    location.end                       = parser->token.location.start;
    ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindParameter, name, type,
                                                                          NULL);
    SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
    SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier");
    }

    return declaration;
}

/// grammar: type         := builtin-type | opaque-type | pointer-type | array-type
/// grammar: builtin-type := "Void" | "Bool" |
///                          "Int8" | "Int16" | "Int32" | "Int64" | "Int128" | "Int" |
///                          "UInt8" | "UInt16" | "UInt32" | "UInt64" | "UInt128" | "UInt" |
///                          "Float16" | "Float32" | "Float64" | "Float128" | "Float"
/// grammar: opaque-type  := identifier
/// grammar: pointer-type := type "*"
/// grammar: array-type   := type "[" [ expression ] "]"
static inline ASTTypeRef _ParserParseType(ParserRef parser) {
    SourceRange location = parser->token.location;
    ASTTypeRef result    = NULL;

    if (_ParserConsumeToken(parser, TokenKindKeywordVoid)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindVoid);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordBool)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindBool);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt8)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt8);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt16)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt16);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt32)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt32);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt64)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt64);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt128)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt128);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordInt)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt8)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt8);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt16)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt16);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt32)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt32);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt64)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt64);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt128)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt128);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordUInt)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat16)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat16);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat32)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat32);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat64)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat64);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat128)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat128);
    } else if (_ParserConsumeToken(parser, TokenKindKeywordFloat)) {
        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat);
    } else {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        location.end = parser->token.location.start;
        result       = (ASTTypeRef)ASTContextCreateOpaqueType(parser->context, location, name);
    }

    while (true) {
        if (_ParserConsumeToken(parser, TokenKindAsterisk)) {
            location.end = parser->token.location.start;
            result       = (ASTTypeRef)ASTContextCreatePointerType(parser->context, location, result);
        } else if (_ParserConsumeToken(parser, TokenKindLeftBracket)) {
            ASTExpressionRef size = NULL;
            if (!_ParserIsToken(parser, TokenKindRightBracket)) {
                size = _ParserParseExpression(parser, 0, false);
                if (!size) {
                    return NULL;
                }
            }

            if (!_ParserConsumeToken(parser, TokenKindRightBracket)) {
                return NULL;
            }

            location.end = parser->token.location.start;
            result       = (ASTTypeRef)ASTContextCreateArrayType(parser->context, location, result, size);
        } else {
            break;
        }
    }

    return result;
}

static inline ASTExpressionRef _ParserParseConditionList(ParserRef parser) {
    ASTExpressionRef condition = _ParserParseExpression(parser, 0, false);
    if (!condition) {
        return NULL;
    }

    while (_ParserConsumeToken(parser, TokenKindComma)) {
        ASTExpressionRef expression = _ParserParseExpression(parser, 0, false);
        if (!expression) {
            return NULL;
        }

        SourceRange location         = {condition->location.start, expression->location.end};
        ASTExpressionRef arguments[] = {condition, expression};
        condition = (ASTExpressionRef)ASTContextCreateBinaryExpression(parser->context, location, ASTBinaryOperatorLogicalAnd, arguments);
    }

    return condition;
}

/// grammar: top-level-node := load-declaration | enum-declaration | func-declaration | struct-declaration | variable-declaration
static inline ASTNodeRef _ParserParseTopLevelNode(ParserRef parser) {
    if (parser->token.kind == TokenKindEndOfFile) {
        return NULL;
    }

    if (_ParserIsToken(parser, TokenKindDirectiveLoad)) {
        return (ASTNodeRef)_ParserParseDirective(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordEnum)) {
        return (ASTNodeRef)_ParserParseEnumerationDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordFunc)) {
        return (ASTNodeRef)_ParserParseFunctionDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordStruct)) {
        return (ASTNodeRef)_ParserParseStructureDeclaration(parser);
    }

    if (_ParserIsToken(parser, TokenKindKeywordVar)) {
        ASTValueDeclarationRef value = _ParserParseVariableDeclaration(parser);
        if (!value) {
            return NULL;
        }

        assert(value->kind == ASTValueKindVariable);
        return (ASTNodeRef)value;
    }

    ReportError("Expected top level node!");
    return NULL;
}
