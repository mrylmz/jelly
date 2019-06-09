#include "JellyCore/ASTContext.h"
#include "JellyCore/ASTFunctions.h"
#include "JellyCore/ASTNodes.h"
#include "JellyCore/Diagnostic.h"
#include "JellyCore/Parser.h"
#include "JellyCore/SourceRange.h"

struct _Parser {
    AllocatorRef allocator;
    ASTContextRef context;
    const Char *bufferStart;
    const Char *bufferEnd;
    const Char *cursor;
    Index line;
};

static inline Bool _CharIsStartOfIdentifier(Char character);
static inline Bool _CharIsContinuationOfIdentifier(Char character);
static inline Bool _CharIsBinaryDigit(Char character);
static inline Bool _CharIsOctalDigit(Char character);
static inline Bool _CharIsDecimalDigit(Char character);
static inline Bool _CharIsHexadecimalDigit(Char character);
static inline Bool _StringIsValidFilePath(StringRef string);
static inline void _ParserSkipWhitespaceAndNewlines(ParserRef parser);
static inline Bool _ParserIsChar(ParserRef parser, Char character);
static inline Bool _ParserConsumeChar(ParserRef parser, Char character);
static inline Bool _ParserConsumeString(ParserRef parser, const Char *string);
static inline Bool _ParserIsKeyword(ParserRef parser, const Char *keyword);
static inline Bool _ParserConsumeKeyword(ParserRef parser, const Char *keyword);
static inline StringRef _ParserConsumeIdentifier(ParserRef parser);
static inline ASTUnaryOperator _ParserConsumeUnaryOperator(ParserRef parser);
static inline ASTBinaryOperator _ParserConsumeBinaryOperator(ParserRef parser);
static inline ASTLoadDirectiveRef _ParserParseDirective(ParserRef parser);
static inline ASTBlockRef _ParserParseBlock(ParserRef parser);
static inline ASTIfStatementRef _ParserParseIfStatement(ParserRef parser);
static inline ASTLoopStatementRef _ParserParseLoopStatement(ParserRef parser);
static inline ASTCaseStatementRef _ParserParseCaseStatement(ParserRef parser);
static inline ASTSwitchStatementRef _ParserParseSwitchStatement(ParserRef parser);
static inline ASTControlStatementRef _ParserParseControlStatement(ParserRef parser);
static inline ASTNodeRef _ParserParseStatement(ParserRef parser);
static inline ASTExpressionRef _ParserParsePrimaryExpression(ParserRef parser);
static inline ASTUnaryExpressionRef _ParserParseUnaryExpression(ParserRef parser);
static inline ASTExpressionRef _ParserParseExpression(ParserRef parser, ASTOperatorPrecedence minPrecedence, Bool silentDiagnostics);
static inline ASTIdentifierExpressionRef _ParserParseIdentifierExpression(ParserRef parser);
static inline ASTCallExpressionRef _ParserParseCallExpression(ParserRef parser, ASTExpressionRef callee);
static inline ASTConstantExpressionRef _ParserParseConstantExpression(ParserRef parser);
static inline ASTConstantExpressionRef _ParserParseNumericLiteral(ParserRef parser);
static inline ASTConstantExpressionRef _ParserParseStringLiteral(ParserRef parser);
static inline ASTEnumerationDeclarationRef _ParserParseEnumerationDeclaration(ParserRef parser);
static inline ASTFunctionDeclarationRef _ParserParseFunctionDeclaration(ParserRef parser);
static inline ASTStructureDeclarationRef _ParserParseStructureDeclaration(ParserRef parser);
static inline ASTValueDeclarationRef _ParserParseValueDeclaration(ParserRef parser);
static inline ASTTypeRef _ParserParseType(ParserRef parser);
static inline ASTExpressionRef _ParserParseConditionList(ParserRef parser);
static inline ASTNodeRef _ParserParseTopLevelNode(ParserRef parser);

ParserRef ParserCreate(AllocatorRef allocator, ASTContextRef context) {
    ParserRef parser    = AllocatorAllocate(allocator, sizeof(struct _Parser));
    parser->allocator   = allocator;
    parser->context     = context;
    parser->bufferStart = NULL;
    parser->bufferEnd   = NULL;
    parser->cursor      = NULL;
    parser->line        = 0;
    return parser;
}

void ParserDestroy(ParserRef parser) {
    AllocatorDeallocate(parser->allocator, parser);
}

ASTSourceUnitRef ParserParseSourceUnit(ParserRef parser, StringRef filePath, StringRef source) {
    ASTModuleDeclarationRef module = ASTContextGetModule(parser->context);

    parser->bufferStart = StringGetCharacters(source);
    parser->bufferEnd   = parser->bufferStart + StringGetLength(source);
    parser->cursor      = parser->bufferStart;
    parser->line        = 1;

    SourceRange location               = {parser->cursor, parser->cursor};
    ArrayRef declarations              = ArrayCreateEmpty(parser->allocator, sizeof(ASTNodeRef), 8);
    Bool checkConsecutiveTopLevelNodes = false;
    Index line                         = parser->line;
    while (true) {
        Index nodeLine         = parser->line;
        ASTNodeRef declaration = _ParserParseTopLevelNode(parser);
        if (!declaration) {
            break;
        }

        if (checkConsecutiveTopLevelNodes && line == nodeLine) {
            ReportError("Consecutive top level nodes on a line are not allowed!");
            break;
        }

        checkConsecutiveTopLevelNodes = true;
        line                          = nodeLine;

        ArrayAppendElement(declarations, declaration);
    }

    location.end                = parser->cursor;
    ASTSourceUnitRef sourceUnit = ASTContextCreateSourceUnit(parser->context, location, filePath, declarations);
    ArrayAppendElement(module->sourceUnits, sourceUnit);
    return sourceUnit;
}

static inline Bool _CharIsStartOfIdentifier(Char character) {
    switch (character) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
        return true;

    default:
        return false;
    }
}

static inline Bool _CharIsContinuationOfIdentifier(Char character) {
    switch (character) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
    case '0' ... '9':
        return true;

    default:
        return false;
    }
}

static inline Bool _CharIsBinaryDigit(Char character) {
    switch (character) {
    case '0' ... '1':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsOctalDigit(Char character) {
    switch (character) {
    case '0' ... '7':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsDecimalDigit(Char character) {
    switch (character) {
    case '0' ... '9':
        return true;
    default:
        return false;
    }
}

static inline Bool _CharIsHexadecimalDigit(Char character) {
    switch (character) {
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F':
        return true;
    default:
        return false;
    }
}

static inline Bool _StringIsValidFilePath(StringRef string) {
    // TODO: Implement check for file path validation
    return true;
}

static inline void _ParserSkipWhitespaceAndNewlines(ParserRef parser) {
    while (parser->cursor != parser->bufferEnd)
        switch (*parser->cursor) {
        case 0x09:
        case 0x20:
            parser->cursor += 1;
            break;

        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
            parser->cursor += 1;
            parser->line += 1;
            break;

        default:
            return;
        }
}

static inline Bool _ParserIsChar(ParserRef parser, Char character) {
    return *parser->cursor == character;
}

static inline Bool _ParserConsumeChar(ParserRef parser, Char character) {
    if (*parser->cursor != character) {
        return false;
    }

    parser->cursor += 1;
    return true;
}

static inline Bool _ParserConsumeString(ParserRef parser, const Char *string) {
    Index length = strlen(string);
    if (length != strnlen(parser->cursor, length)) {
        return false;
    }

    if (strncmp(parser->cursor, string, length) != 0) {
        return false;
    }

    return true;
}

static inline Bool _ParserIsKeyword(ParserRef parser, const Char *keyword) {
    Index length = strlen(keyword);
    if (length != strnlen(parser->cursor, length)) {
        return false;
    }

    if (strncmp(parser->cursor, keyword, length) != 0) {
        return false;
    }

    const Char *next = parser->cursor + length;
    if (_CharIsContinuationOfIdentifier(next)) {
        return false;
    }

    return true;
}

static inline Bool _ParserConsumeKeyword(ParserRef parser, const Char *keyword) {
    Index length = strlen(keyword);
    if (length != strnlen(parser->cursor, length)) {
        return false;
    }

    if (strncmp(parser->cursor, keyword, length) != 0) {
        return false;
    }

    const Char *next = parser->cursor + length;
    if (_CharIsContinuationOfIdentifier(next)) {
        return false;
    }

    parser->cursor = next;
    return true;
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static inline StringRef _ParserConsumeIdentifier(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_CharIsStartOfIdentifier(parser->cursor)) {
        return NULL;
    }

    parser->cursor += 1;
    while (_CharIsContinuationOfIdentifier(parser->cursor)) {
        parser->cursor += 1;
    }

    location.end = parser->cursor;
    return StringCreateRange(kAllocatorSystemDefault, location.start, location.end);
}

static inline ASTUnaryOperator _ParserConsumeUnaryOperator(ParserRef parser) {
    if (_ParserConsumeChar(parser, '!')) {
        return ASTUnaryOperatorLogicalNot;
    } else if (_ParserConsumeChar(parser, '~')) {
        return ASTUnaryOperatorBitwiseNot;
    } else if (_ParserConsumeChar(parser, '+')) {
        return ASTUnaryOperatorUnaryPlus;
    } else if (_ParserConsumeChar(parser, '-')) {
        return ASTUnaryOperatorUnaryMinus;
    } else {
        return ASTUnaryOperatorUnknown;
    }
}

static inline ASTBinaryOperator _ParserConsumeBinaryOperator(ParserRef parser) {
    // @TODO Order by ambiguity (string length in descending order)
    if (_ParserConsumeString(parser, "<<")) {
        return ASTBinaryOperatorBitwiseLeftShift;
    } else if (_ParserConsumeString(parser, ">>")) {
        return ASTBinaryOperatorBitwiseRightShift;
    } else if (_ParserConsumeChar(parser, "*")) {
        return ASTBinaryOperatorMultiply;
    } else if (_ParserConsumeChar(parser, "/")) {
        return ASTBinaryOperatorDivide;
    } else if (_ParserConsumeChar(parser, "%")) {
        return ASTBinaryOperatorReminder;
    } else if (_ParserConsumeChar(parser, "&")) {
        return ASTBinaryOperatorBitwiseAnd;
    } else if (_ParserConsumeChar(parser, "+")) {
        return ASTBinaryOperatorAdd;
    } else if (_ParserConsumeChar(parser, "-")) {
        return ASTBinaryOperatorSubtract;
    } else if (_ParserConsumeChar(parser, "|")) {
        return ASTBinaryOperatorBitwiseOr;
    } else if (_ParserConsumeChar(parser, "^")) {
        return ASTBinaryOperatorBitwiseXor;
    } else if (_ParserConsumeKeyword(parser, "is")) {
        return ASTBinaryOperatorTypeCheck;
    } else if (_ParserConsumeKeyword(parser, "as")) {
        return ASTBinaryOperatorTypeCast;
    } else if (_ParserConsumeChar(parser, "<")) {
        return ASTBinaryOperatorLessThan;
    } else if (_ParserConsumeString(parser, "<=")) {
        return ASTBinaryOperatorLessThanEqual;
    } else if (_ParserConsumeChar(parser, ">")) {
        return ASTBinaryOperatorGreaterThan;
    } else if (_ParserConsumeString(parser, ">=")) {
        return ASTBinaryOperatorGreaterThanEqual;
    } else if (_ParserConsumeString(parser, "==")) {
        return ASTBinaryOperatorEqual;
    } else if (_ParserConsumeString(parser, "!=")) {
        return ASTBinaryOperatorNotEqual;
    } else if (_ParserConsumeString(parser, "&&")) {
        return ASTBinaryOperatorLogicalAnd;
    } else if (_ParserConsumeString(parser, "||")) {
        return ASTBinaryOperatorLogicalOr;
    } else if (_ParserConsumeChar(parser, "=")) {
        return ASTBinaryOperatorAssign;
    } else if (_ParserConsumeString(parser, "*=")) {
        return ASTBinaryOperatorMultiplyAssign;
    } else if (_ParserConsumeString(parser, "/=")) {
        return ASTBinaryOperatorDivideAssign;
    } else if (_ParserConsumeString(parser, "%=")) {
        return ASTBinaryOperatorReminderAssign;
    } else if (_ParserConsumeString(parser, "+=")) {
        return ASTBinaryOperatorAddAssign;
    } else if (_ParserConsumeString(parser, "-=")) {
        return ASTBinaryOperatorSubtractAssign;
    } else if (_ParserConsumeString(parser, "<<=")) {
        return ASTBinaryOperatorBitwiseLeftShiftAssign;
    } else if (_ParserConsumeString(parser, ">>=")) {
        return ASTBinaryOperatorBitwiseRightShiftAssign;
    } else if (_ParserConsumeString(parser, "&=")) {
        return ASTBinaryOperatorBitwiseAndAssign;
    } else if (_ParserConsumeString(parser, "|=")) {
        return ASTBinaryOperatorBitwiseOrAssign;
    } else if (_ParserConsumeString(parser, "^=")) {
        return ASTBinaryOperatorBitwiseXorAssign;
    } else {
        return ASTBinaryOperatorUnknown;
    }
}

/// grammar: directive      := load-directive
/// grammar: load-directive := "#load" string-literal
static inline ASTLoadDirectiveRef _ParserParseDirective(ParserRef parser) {
    assert(*parser->cursor == '#');

    SourceRange location = {parser->cursor, parser->cursor};

    if (_ParserConsumeKeyword(parser, "#load")) {
        ASTConstantExpressionRef filePath = _ParserParseConstantExpression(parser);
        if (!filePath || filePath->kind != ASTConstantKindString) {
            ReportError("Expected string literal after `#load` directive!");
            return NULL;
        }

        assert(filePath->kind == ASTConstantKindString);

        if (!_StringIsValidFilePath(filePath->stringValue)) {
            ReportError("Expected valid file path after `#load` directive!");
            return NULL;
        }

        location.end = parser->cursor;
        return ASTContextCreateLoadDirective(parser->context, location, filePath);
    }

    ReportError("Unknown compiler directive!");
    return NULL;
}

/// grammar: block := '{' { statement } '}'
static inline ASTBlockRef _ParserParseBlock(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_ParserConsumeChar(parser, '{')) {
        return NULL;
    }

    // @TODO: Push block scope externally !
    // @TODO: Add temporary pool allocator to parser, for now this will leak memory!
    ArrayRef statements = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);

    Index line = parser->line;
    while (*parser->cursor != '}') {
        if (ArrayGetElementCount(statements) > 0 && line == parser->line) {
            ReportError("Consecutive statements on a line are not allowed!");
            return NULL;
        }

        line = parser->line;

        ASTNodeRef statement = _ParserParseStatement(parser);
        if (!statement) {
            return NULL;
        }

        ArrayAppendElement(statements, statement);
    }

    if (!_ParserConsumeChar(parser, '}')) {
        return NULL;
    }

    location.end = parser->cursor;
    return ASTContextCreateBlock(parser->context, location, statements);
}

/// grammar: if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]
static inline ASTIfStatementRef _ParserParseIfStatement(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_ParserConsumeKeyword(parser, "if")) {
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
    if (_ParserConsumeKeyword(parser, "else")) {
        SymbolTablePushScope(symbolTable, ScopeKindBranch);

        location.end = parser->cursor;
        if (_ParserIsKeyword(parser, "if")) {
            SourceRange location          = {parser->cursor, parser->cursor};
            ASTIfStatementRef ifStatement = _ParserParseIfStatement(parser);
            if (!ifStatement) {
                return NULL;
            }

            // @TODO: Add temporary pool allocator to parser, for now this will leak memory!
            ArrayRef statements = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 1);
            ArrayAppendElement(statements, ifStatement);

            location.end = parser->cursor;
            elseBlock    = ASTContextCreateBlock(parser->context, location, statements);
        } else {
            elseBlock = _ParserParseBlock(parser);
            if (!elseBlock) {
                return NULL;
            }
        }

        SymbolTablePopScope(symbolTable);
    }

    location.end = parser->cursor;
    return ASTContextCreateIfStatement(parser->context, location, condition, thenBlock, elseBlock);
}

/// grammar: loop-statement  := while-statement | do-statement
/// grammar: while-statement := "while" expression { "," expression } block
/// grammar: do-statement    := "do" block "while" expression
static inline ASTLoopStatementRef _ParserParseLoopStatement(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (_ParserConsumeKeyword(parser, "while")) {
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

        location.end = parser->cursor;
        return ASTContextCreateLoopStatement(parser->context, location, ASTLoopKindWhile, condition, loopBlock);
    }

    if (_ParserConsumeKeyword(parser, "do")) {
        SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
        SymbolTablePushScope(symbolTable, ScopeKindLoop);

        ASTBlockRef loopBlock = _ParserParseBlock(parser);
        if (!loopBlock) {
            return NULL;
        }

        if (!_ParserConsumeKeyword(parser, "while")) {
            return NULL;
        }

        ASTExpressionRef condition = _ParserParseConditionList(parser);
        if (!condition) {
            return NULL;
        }

        SymbolTablePopScope(symbolTable);

        location.end = parser->cursor;
        return ASTContextCreateLoopStatement(parser->context, location, ASTLoopKindDo, condition, loopBlock);
    }

    ReportError("Expected 'while' or 'do' at start of loop-statement!");
    return NULL;
}

/// grammar: case-statement             := conditional-case-statement | else-case-statement
/// grammar: conditional-case-statement := "case" expression ":" statement { line-break statement }
/// grammar: else-case-statement        := "else" ":" statement { line-break statement }
static inline ASTCaseStatementRef _ParserParseCaseStatement(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    ASTCaseKind kind           = ASTCaseKindElse;
    ASTExpressionRef condition = NULL;
    if (_ParserConsumeKeyword(parser, "case")) {
        kind      = ASTCaseKindConditional;
        condition = _ParserParseExpression(parser, 0, false);
        if (!condition) {
            return NULL;
        }
    } else if (!_ParserConsumeKeyword(parser, "else")) {
        ReportError("Expected 'case' or 'else' at start of case-statement!");
        return NULL;
    }

    if (!_ParserConsumeChar(parser, ':')) {
        return NULL;
    }

    SourceRange blockLocation  = {parser->cursor, parser->cursor};
    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SymbolTablePushScope(symbolTable, ScopeKindCase);

    ArrayRef statements = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);
    Index line          = parser->line;

    while (!_ParserIsKeyword(parser, "case") && !_ParserIsKeyword(parser, "else") && !_ParserIsChar(parser, '}')) {
        if (ArrayGetElementCount(statements) > 0 && line == parser->line) {
            ReportError("Consecutive statements on a line are not allowed!");
            return NULL;
        }

        line                 = parser->line;
        ASTNodeRef statement = _ParserParseStatement(parser);
        if (!statement) {
            return NULL;
        }

        ArrayAppendElement(statements, statement);
    }

    SymbolTablePopScope(symbolTable);

    blockLocation.end = parser->cursor;
    ASTBlockRef body  = ASTContextCreateBlock(parser->context, blockLocation, statements);

    location.end = parser->cursor;
    return ASTContextCreateCaseStatement(parser->context, location, kind, condition, body);
}

/// grammar: switch-statement := "switch" expression "{" [ case-statement { line-break case-statement } ] "}"
static inline ASTSwitchStatementRef _ParserParseSwitchStatement(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_ParserConsumeKeyword(parser, "switch")) {
        return NULL;
    }

    ASTExpressionRef argument = _ParserParseExpression(parser, 0, false);
    if (!argument) {
        return NULL;
    }

    if (!_ParserConsumeChar(parser, '{')) {
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    SymbolTablePushScope(symbolTable, ScopeKindSwitch);

    ArrayRef statements = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);
    Index line          = parser->line;
    while (!_ParserIsChar(parser, '}')) {
        if (ArrayGetElementCount(statements) > 0 && line == parser->line) {
            ReportError("Consecutive statements on a line are not allowed!");
            return NULL;
        }

        line = parser->line;

        ASTCaseStatementRef statement = _ParserParseCaseStatement(parser);
        if (!statement) {
            return NULL;
        }

        ArrayAppendElement(statements, statement);
    }

    if (!_ParserConsumeChar(parser, '}')) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end = parser->cursor;
    return ASTContextCreateSwitchStatement(parser->context, location, argument, statements);
}

/// grammar: control-statement      := break-statement | continue-statement | fallthrough-statement | return-statement
/// grammar: break-statement        := "break"
/// grammar: continue-statement     := "continue"
/// grammar: fallthrough-statement  := "fallthrough"
/// grammar: return-statement       := "return" [ expression ]
static inline ASTControlStatementRef _ParserParseControlStatement(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    ASTControlKind kind;
    ASTExpressionRef result = NULL;
    if (_ParserConsumeKeyword(parser, "break")) {
        kind = ASTControlKindBreak;
    } else if (_ParserConsumeKeyword(parser, "continue")) {
        kind = ASTControlKindContinue;
    } else if (_ParserConsumeKeyword(parser, "fallthrough")) {
        kind = ASTControlKindFallthrough;
    } else if (_ParserConsumeKeyword(parser, "return")) {
        kind         = ASTControlKindReturn;
        location.end = parser->cursor;
        result       = _ParserParseExpression(parser, 0, true);
        if (!result) {
            parser->cursor = location.end;
        }
    } else {
        ReportError("Expected 'break', 'continue', 'fallthrough' or 'return' at start of control-statement!");
        return NULL;
    }

    location.end = parser->cursor;
    return ASTContextCreateControlStatement(parser->context, location, kind, result);
}

/// grammar: statement := variable-declaration | control-statement | loop-statement | if-statement | switch-statement | expression
static inline ASTNodeRef _ParserParseStatement(ParserRef parser) {
    if (_ParserIsKeyword(parser, "var") || _ParserIsKeyword(parser, "let")) {
        ASTValueDeclarationRef value = _ParserParseValueDeclaration(parser);
        if (!value || (value->kind != ASTValueKindVariable && value->kind != ASTValueKindConstant)) {
            return NULL;
        }

        return (ASTNodeRef)value;
    }

    if (_ParserIsKeyword(parser, "break") || _ParserIsKeyword(parser, "continue") || _ParserIsKeyword(parser, "fallthrough") ||
        _ParserIsKeyword(parser, "return")) {
        return (ASTNodeRef)_ParserParseControlStatement(parser);
    }

    if (_ParserIsKeyword(parser, "do") || _ParserIsKeyword(parser, "while")) {
        return (ASTNodeRef)_ParserParseLoopStatement(parser);
    }

    if (_ParserIsKeyword(parser, "if")) {
        return (ASTNodeRef)_ParserParseIfStatement(parser);
    }

    if (_ParserIsKeyword(parser, "switch")) {
        return (ASTNodeRef)_ParserParseSwitchStatement(parser);
    }

    return (ASTNodeRef)_ParserParseExpression(parser, 0, false);
}

/// grammar: atom-expression       := group-expression | literal-expression | identifier-expression
/// grammar: group-expression      := "(" expression ")"
/// grammar: literal-expression    := literal
/// grammar: identifier-expression := identifier
static inline ASTExpressionRef _ParserParseAtomExpression(ParserRef parser) {
    if (_ParserConsumeChar(parser, '(')) {
        ASTExpressionRef expression = _ParserParseExpression(parser, 0, false);
        if (!expression) {
            return NULL;
        }

        if (!_ParserConsumeChar(parser, ')')) {
            return NULL;
        }

        return expression;
    }

    if (_ParserIsKeyword(parser, "nil") || _ParserIsKeyword(parser, "true") || _ParserIsKeyword(parser, "false") ||
        _CharIsDecimalDigit(*parser->cursor) || *parser->cursor == '"') {
        return (ASTExpressionRef)_ParserParseConstantExpression(parser);
    }

    return (ASTExpressionRef)_ParserParseIdentifierExpression(parser);
}

/// grammar: primary-expression := unary-expression | atom-expression
static inline ASTExpressionRef _ParserParsePrimaryExpression(ParserRef parser) {
    SourceRange location   = {parser->cursor, parser->cursor};
    ASTUnaryOperator unary = _ParserConsumeUnaryOperator(parser);
    if (unary != ASTUnaryOperatorUnknown) {
        parser->cursor = location.start;
        return (ASTExpressionRef)_ParserParseUnaryExpression(parser);
    }

    return _ParserParseAtomExpression(parser);
}

/// grammar: unary-expression := prefix-operator expression
/// grammar: prefix-operator  := '!' | '~' | '+' | '-'
static inline ASTUnaryExpressionRef _ParserParseUnaryExpression(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    ASTUnaryOperator unary = _ParserConsumeUnaryOperator(parser);
    if (unary == ASTUnaryOperatorUnknown) {
        ReportError("Unknown unary operator!");
        return NULL;
    }

    const Char *unaryEnd = parser->cursor;

    ASTExpressionRef arguments[1];
    arguments[0] = _ParserParsePrimaryExpression(parser);
    if (!arguments[0]) {
        return NULL;
    }

    if (unaryEnd != arguments[0]->location.start - 1) {
        ReportError("Unary operator has to be right bound!");
        return NULL;
    }

    location.end = parser->cursor;
    return ASTContextCreateUnaryExpression(parser->context, location, unary, arguments);
}

/// grammar: expression        := binary-expression | primary-expression
/// grammar: binary-expression := primary-expression infix-operator expression
static inline ASTExpressionRef _ParserParseExpression(ParserRef parser, ASTOperatorPrecedence minPrecedence, Bool silentDiagnostics) {
    SourceRange location = {parser->cursor, parser->cursor};

    ASTExpressionRef result = _ParserParsePrimaryExpression(parser);
    if (!result) {
        return NULL;
    }

    location.end             = parser->cursor;
    ASTBinaryOperator binary = _ParserConsumeBinaryOperator(parser);
    if (binary == ASTBinaryOperatorUnknown) {
        parser->cursor = location.end;
        return result;
    }

    ASTOperatorPrecedence precedence = ASTGetBinaryOperatorPrecedence(binary);
    if (minPrecedence >= precedence) {
        parser->cursor = location.end;
        return result;
    }

    while (minPrecedence < precedence) {
        ASTOperatorPrecedence nextPrecedence   = precedence;
        ASTOperatorAssociativity associativity = ASTGetBinaryOperatorAssociativity(binary);
        if (associativity == ASTOperatorAssociativityRight) {
            nextPrecedence = ASTGetOperatorPrecedenceBefore(nextPrecedence);
        }

        ASTExpressionRef right = _ParserParseExpression(parser, nextPrecedence, silentDiagnostics);
        if (!right) {
            return NULL;
        }

        location.end                  = parser->cursor;
        ASTExpressionRef arguments[2] = {result, right};
        result                        = (ASTExpressionRef)ASTContextCreateBinaryExpression(parser->context, location, binary, arguments);
        if (!result) {
            return NULL;
        }

        // TODO: Add support for postfix operator!
        //        else if (_ParserIsChar(parser, '.')) {
        //            StringRef memberName = _ParserConsumeIdentifier(parser);
        //            if (!memberName) {
        //                return NULL;
        //            }
        //
        //            location.end = parser->cursor;
        //            result       = ASTContextCreateMemberAccessExpression(parser->context, location, result, memberName);
        //        }
        //        // @Bug postfix expressions should always be parsed as primary expressions without precedence!
        //        else if (op == Operator::Subscript) {
        //            left = parseSubscriptExpression(left);
        //        } else if (op == Operator::Call) {
        //            result = _ParserParseCallExpression(parser, result);
        //        } else {
        //            return NULL;
        //        }

        location.end = parser->cursor;
        binary       = _ParserConsumeBinaryOperator(parser);
        if (binary == ASTBinaryOperatorUnknown) {
            parser->cursor = location.end;
            return result;
        }
    }

    return result;
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static inline ASTIdentifierExpressionRef _ParserParseIdentifierExpression(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};
    StringRef name       = _ParserConsumeIdentifier(parser);
    if (!name) {
        return NULL;
    }

    location.end = parser->cursor;
    return ASTContextCreateIdentifierExpression(parser->context, location, name);
}

/// grammar: call-expression := expression "(" [ expression { "," expression } ] ")"
static inline ASTCallExpressionRef _ParserParseCallExpression(ParserRef parser, ASTExpressionRef callee) {
    SourceRange location = {callee->location.start, parser->cursor};

    if (!_ParserConsumeChar(parser, '(')) {
        return NULL;
    }

    ArrayRef arguments = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);
    while (!_ParserIsChar(parser, ')')) {
        ASTExpressionRef argument = _ParserParseExpression(parser, 0, false);
        if (!argument) {
            return NULL;
        }

        ArrayAppendElement(arguments, argument);

        if (_ParserIsChar(parser, ')')) {
            break;
        }

        if (!_ParserConsumeChar(parser, ',')) {
            return NULL;
        }
    }

    if (!_ParserConsumeChar(parser, ')')) {
        return NULL;
    }

    location.end = parser->cursor;
    return ASTContextCreateCallExpression(parser->context, location, callee, arguments);
}

/// grammar: constant-expression := nil-literal | bool-literal | numeric-literal | string-literal
/// grammar: nil-literal         := "nil"
/// grammar: bool-literal        := "true" | "false"
static inline ASTConstantExpressionRef _ParserParseConstantExpression(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (_ParserConsumeKeyword(parser, "nil")) {
        location.end = parser->cursor;
        return ASTContextCreateConstantNilExpression(parser->context, location);
    }

    if (_ParserConsumeKeyword(parser, "true")) {
        location.end = parser->cursor;
        return ASTContextCreateConstantBoolExpression(parser->context, location, true);
    }

    if (_ParserConsumeKeyword(parser, "false")) {
        location.end = parser->cursor;
        return ASTContextCreateConstantBoolExpression(parser->context, location, false);
    }

    if (_ParserIsChar(parser, "\"")) {
        return _ParserParseStringLiteral(parser);
    }

    return _ParserParseNumericLiteral(parser);
}

/// grammar: numeric-literal := integer-literal | float-litereal
/// grammar: integer-literal := TODO: Describe int literal grammar
/// grammar: float-literal   := TODO: Describe float literal grammar
static inline ASTConstantExpressionRef _ParserParseNumericLiteral(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_CharIsDecimalDigit(parser->cursor)) {
        return NULL;
    }
    parser->cursor += 1;

    if (*(parser->cursor - 1) == '0') {
        if (_ParserConsumeChar(parser, "b")) {
            parser->cursor += 1;

            if (!_CharIsBinaryDigit(*parser->cursor)) {
                return NULL;
            }

            while (_CharIsBinaryDigit(*parser->cursor)) {
                parser->cursor += 1;
            }

            if (_CharIsContinuationOfIdentifier(*parser->cursor)) {
                return NULL;
            }

            location.end              = parser->cursor;
            SourceRange valueLocation = {location.start + 2, location.end};
            if (valueLocation.end - valueLocation.start - 1 > 64) {
                return NULL;
            }

            UInt64 value = 0;
            for (const Char *cursor = valueLocation.start; cursor < valueLocation.end; cursor++) {
                value *= 2;
                value += *cursor - '0';
            }

            return ASTContextCreateConstantIntExpression(parser->context, location, value);
        }

        if (_ParserConsumeChar(parser, "o")) {
            parser->cursor += 1;

            if (!_CharIsOctalDigit(*parser->cursor)) {
                return NULL;
            }

            while (_CharIsOctalDigit(*parser->cursor)) {
                parser->cursor += 1;
            }

            if (_CharIsContinuationOfIdentifier(*parser->cursor)) {
                return NULL;
            }

            location.end              = parser->cursor;
            SourceRange valueLocation = {location.start + 2, location.end};

            UInt64 value = 0;
            for (const Char *cursor = valueLocation.start; cursor < valueLocation.end; cursor++) {
                UInt64 oldValue = value;

                value *= 8;
                value += *cursor - '0';

                if (value < oldValue) {
                    return NULL;
                }
            }

            return ASTContextCreateConstantIntExpression(parser->context, location, value);
        }

        if (_ParserConsumeChar(parser, "x")) {
            if (!_CharIsHexadecimalDigit(*parser->cursor)) {
                return NULL;
            }

            while (_CharIsHexadecimalDigit(*parser->cursor)) {
                parser->cursor += 1;
            }

            Bool isInteger = !(_ParserIsChar(parser, ".") || _ParserIsChar(parser, "p") || _ParserIsChar(parser, "P"));
            if (isInteger) {
                if (_CharIsContinuationOfIdentifier(*parser->cursor)) {
                    return NULL;
                }

                location.end              = parser->cursor;
                SourceRange valueLocation = {location.start + 2, location.end};

                UInt64 value = 0;
                for (const Char *cursor = valueLocation.start; cursor < valueLocation.end; cursor++) {
                    UInt64 oldValue = value;

                    value *= 16;

                    if ('0' <= *cursor && *cursor <= '9') {
                        value += *cursor - '0';
                    } else if ('a' <= *cursor && *cursor <= 'f') {
                        value += *cursor - 'a' + 10;
                    } else if ('A' <= *cursor && *cursor <= 'F') {
                        value += *cursor - 'A' + 10;
                    } else {
                        return NULL;
                    }

                    if (value < oldValue) {
                        return NULL;
                    }
                }

                return ASTContextCreateConstantIntExpression(parser->context, location, value);
            }

            SourceRange headLocation     = {location.start + 2, parser->cursor};
            SourceRange tailLocation     = kSourceRangeNull;
            SourceRange exponentLocation = kSourceRangeNull;

            if (_ParserConsumeChar(parser, ".")) {
                tailLocation.start = parser->cursor;

                if (!_CharIsHexadecimalDigit(*parser->cursor)) {
                    return NULL;
                }

                while (_CharIsHexadecimalDigit(*parser->cursor)) {
                    parser->cursor += 1;
                }

                tailLocation.end = parser->cursor;
            }

            Bool isExponentPositive = true;
            if (_ParserConsumeChar(parser, "p") || _ParserConsumeChar(parser, "P")) {
                if (_ParserConsumeChar(parser, "-")) {
                    isExponentPositive = false;
                } else {
                    _ParserConsumeChar(parser, "+");
                }

                exponentLocation.start = parser->cursor;

                if (!_CharIsHexadecimalDigit(*parser->cursor)) {
                    return NULL;
                }

                while (_CharIsHexadecimalDigit(*parser->cursor)) {
                    parser->cursor += 1;
                }

                exponentLocation.end = parser->cursor;
            }

            if (_CharIsContinuationOfIdentifier(*parser->cursor)) {
                return NULL;
            }

            location.end = parser->cursor;

            Float64 value = 0;
            for (const Char *cursor = headLocation.start; cursor < headLocation.end; cursor++) {
                value *= 16;

                if ('0' <= *cursor && *cursor <= '9') {
                    value += *cursor - '0';
                } else if ('a' <= *cursor && *cursor <= 'f') {
                    value += *cursor - 'a' + 10;
                } else if ('A' <= *cursor && *cursor <= 'F') {
                    value += *cursor - 'A' + 10;
                } else {
                    return NULL;
                }
            }

            if (tailLocation.start) {
                Float64 fraction = 0;
                for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
                    fraction *= 16;

                    if ('0' <= *cursor && *cursor <= '9') {
                        fraction += *cursor - '0';
                    } else if ('a' <= *cursor && *cursor <= 'f') {
                        fraction += *cursor - 'a' + 10;
                    } else if ('A' <= *cursor && *cursor <= 'F') {
                        fraction += *cursor - 'A' + 10;
                    } else {
                        return NULL;
                    }
                }

                for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
                    fraction /= 16;
                }

                value += fraction;
            }

            if (exponentLocation.start) {
                UInt64 exponent = 0;
                for (const Char *cursor = exponentLocation.start; cursor < exponentLocation.end; cursor++) {
                    UInt64 oldValue = exponent;

                    exponent *= 16;

                    if ('0' <= *cursor && *cursor <= '9') {
                        exponent += *cursor - '0';
                    } else if ('a' <= *cursor && *cursor <= 'f') {
                        exponent += *cursor - 'a' + 10;
                    } else if ('A' <= *cursor && *cursor <= 'F') {
                        exponent += *cursor - 'A' + 10;
                    } else {
                        return NULL;
                    }

                    if (exponent < oldValue) {
                        return NULL;
                    }
                }

                Float64 sign = isExponentPositive ? 10 : 0.1;
                while (exponent--) {
                    value *= sign;
                }
            }

            return ASTContextCreateConstantFloatExpression(parser->context, location, value);
        }
    }

    while (_CharIsDecimalDigit(*parser->cursor)) {
        parser->cursor += 1;
    }

    Bool isInteger = !(_ParserIsChar(parser, ".") || _ParserIsChar(parser, "e") || _ParserIsChar(parser, "E"));
    if (isInteger) {
        if (_CharIsContinuationOfIdentifier(*parser->cursor)) {
            return NULL;
        }

        location.end = parser->cursor;

        UInt64 value = 0;
        for (const Char *cursor = location.start; cursor < location.end; cursor++) {
            UInt64 oldValue = value;

            value *= 10;
            value += *cursor - '0';

            if (value < oldValue) {
                return NULL;
            }
        }

        return ASTContextCreateConstantIntExpression(parser->context, location, value);
    }

    SourceRange headLocation     = {location.start, parser->cursor};
    SourceRange tailLocation     = kSourceRangeNull;
    SourceRange exponentLocation = kSourceRangeNull;

    if (_ParserConsumeChar(parser, ".")) {
        tailLocation.start = parser->cursor;

        if (!_CharIsDecimalDigit(*parser->cursor)) {
            return NULL;
        }

        while (_CharIsDecimalDigit(*parser->cursor)) {
            parser->cursor += 1;
        }

        tailLocation.end = parser->cursor;
    }

    Bool isExponentPositive = true;
    if (_ParserConsumeChar(parser, "e") || _ParserConsumeChar(parser, "E")) {
        if (_ParserConsumeChar(parser, "-")) {
            isExponentPositive = false;
        } else {
            _ParserConsumeChar(parser, "+");
        }

        exponentLocation.start = parser->cursor;

        if (!_CharIsDecimalDigit(*parser->cursor)) {
            return NULL;
        }

        while (_CharIsDecimalDigit(*parser->cursor)) {
            parser->cursor += 1;
        }

        exponentLocation.end = parser->cursor;
    }

    if (_CharIsContinuationOfIdentifier(*parser->cursor)) {
        return NULL;
    }

    location.end = parser->cursor;

    Float64 value = 0;
    for (const Char *cursor = headLocation.start; cursor < headLocation.end; cursor++) {
        value *= 10;
        value += *cursor - '0';
    }

    if (tailLocation.start) {
        Float64 fraction = 0;
        for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
            fraction *= 10;
            fraction += *cursor - '0';
        }

        for (const Char *cursor = tailLocation.start; cursor < tailLocation.end; cursor++) {
            fraction /= 10;
        }

        value += fraction;
    }

    if (exponentLocation.start) {
        UInt64 exponent = 0;
        for (const Char *cursor = exponentLocation.start; cursor < exponentLocation.end; cursor++) {
            UInt64 oldValue = exponent;

            exponent *= 10;
            exponent += *cursor - '0';

            if (exponent < oldValue) {
                return NULL;
            }
        }

        Float64 sign = isExponentPositive ? 10 : 0.1;
        while (exponent--) {
            value *= sign;
        }
    }

    return ASTContextCreateConstantFloatExpression(parser->context, location, value);
}

/// grammar: string-literal      := """ {string-literal-body } """
/// grammar: string-literal-body := TODO: Add grammar description for string literal body!
static inline ASTConstantExpressionRef _ParserParseStringLiteral(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_ParserConsumeChar(parser, "\"")) {
        return NULL;
    }

    Bool valid    = true;
    Bool finished = false;
    while (!finished) {
        if (parser->cursor == parser->bufferEnd) {
            return NULL;
        }

        switch (*parser->cursor) {
        case '\n':
        case '\r':
        case '\0':
            return NULL;

        case '"':
            finished = true;
            break;

        case '\\':
            parser->cursor += 1;

            switch (*parser->cursor) {
            case '0':
            case '\\':
            case 'r':
            case 'n':
            case 't':
            case '"':
            case '\'':
                parser->cursor += 1;
                break;

            default:
                valid = false;
                break;
            }

            break;

        default:
            parser->cursor += 1;
            break;
        }
    }

    if (!valid || !_ParserConsumeChar(parser, "\"")) {
        return NULL;
    }

    StringRef value = StringCreateRange(kAllocatorSystemDefault, location.start + 1, location.end - 1);
    location.end    = parser->cursor;
    return ASTContextCreateConstantStringExpression(parser->context, location, value);
}

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
static inline ASTEnumerationDeclarationRef _ParserParseEnumerationDeclaration(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};
    if (!_ParserConsumeKeyword(parser, "enum")) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        return NULL;
    }

    if (!_ParserConsumeChar(parser, "{")) {
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope             = SymbolTablePushScope(symbolTable, ScopeKindEnumeration);

    ArrayRef elements = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);
    if (!_ParserIsChar(parser, "}")) {
        Index line = parser->line;

        while (true) {
            if (ArrayGetElementCount(elements) > 0 && line == parser->line) {
                ReportError("Consecutive enum elements on a line are not allowed!");
                return NULL;
            }

            ASTValueDeclarationRef element = _ParserParseValueDeclaration(parser);
            if (!element) {
                return NULL;
            }

            if (element->kind != ASTValueKindEnumerationElement) {
                ReportError("Only enumeration elements are allowed inside of enumeration!");
                return NULL;
            }

            ArrayAppendElement(elements, element);

            if (_ParserIsChar(parser, "}")) {
                break;
            }
        }
    }

    if (!_ParserConsumeChar(parser, "}")) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end = parser->cursor;
    return ASTContextCreateEnumerationDeclaration(parser->context, location, name, elements);
}

/// grammar: func-declaration := "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier block
static inline ASTFunctionDeclarationRef _ParserParseFunctionDeclaration(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_ParserConsumeKeyword(parser, "func")) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        return NULL;
    }

    if (!_ParserConsumeChar(parser, "(")) {
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope             = SymbolTablePushScope(symbolTable, ScopeKindFunction);
    ArrayRef parameters        = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);

    if (!_ParserIsChar(parser, ")")) {
        while (true) {
            ASTValueDeclarationRef parameter = _ParserParseValueDeclaration(parser);
            if (!parameter) {
                return NULL;
            }

            if (parameter->kind != ASTValueKindParameter) {
                ReportError("Only parameter declarations are allowed in parameter list of function declaration!");
                return NULL;
            }

            ArrayAppendElement(parameters, parameter);

            if (_ParserIsChar(parser, ")")) {
                break;
            }

            if (!_ParserConsumeChar(parser, ",")) {
                return NULL;
            }
        }
    }

    if (!_ParserConsumeChar(parser, ")")) {
        return NULL;
    }

    if (!_ParserConsumeString(parser, "->")) {
        return NULL;
    }

    ASTTypeRef returnType = _ParserParseType(parser);
    if (!returnType) {
        return NULL;
    }

    ASTBlockRef body = _ParserParseBlock(parser);
    if (!body) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end                          = parser->cursor;
    ASTFunctionDeclarationRef declaration = ASTContextCreateFunctionDeclaration(parser->context, location, name, parameters, returnType,
                                                                                body);

    SymbolRef symbol = ScopeInsertSymbol(SymbolTableGetCurrentScope(symbolTable), name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier!");
        return NULL;
    }

    return declaration;
}

/// grammar: struct-declaration := "struct" identifier "{" { value-declaration } "}"
static inline ASTStructureDeclarationRef _ParserParseStructureDeclaration(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (!_ParserConsumeKeyword(parser, "struct")) {
        return NULL;
    }

    StringRef name = _ParserConsumeIdentifier(parser);
    if (!name) {
        return NULL;
    }

    if (!_ParserConsumeChar(parser, "{")) {
        return NULL;
    }

    SymbolTableRef symbolTable = ASTContextGetSymbolTable(parser->context);
    ScopeRef scope             = SymbolTablePushScope(symbolTable, ScopeKindStructure);
    ArrayRef values            = ArrayCreateEmpty(kAllocatorSystemDefault, sizeof(ASTNodeRef), 8);

    if (!_ParserIsChar(parser, "}")) {
        Index line = parser->line;
        while (true) {
            if (ArrayGetElementCount(values) > 0 && line == parser->line) {
                ReportError("Consecutive statements on a line are not allowed!");
                return NULL;
            }

            line = parser->line;

            ASTValueDeclarationRef value = _ParserParseValueDeclaration(parser);
            if (!value) {
                return NULL;
            }

            if (value->kind != ASTValueKindVariable && value->kind != ASTValueKindConstant) {
                ReportError("Only variable and constant declarations are allowed in structure declarations!");
                return NULL;
            }

            ArrayAppendElement(values, value);

            if (_ParserIsChar(parser, "}")) {
                break;
            }
        }
    }

    if (!_ParserConsumeChar(parser, "}")) {
        return NULL;
    }

    SymbolTablePopScope(symbolTable);

    location.end                           = parser->cursor;
    ASTStructureDeclarationRef declaration = ASTContextCreateStructureDeclaration(parser->context, location, name, values);

    SymbolRef symbol = ScopeInsertSymbol(SymbolTableGetCurrentScope(symbolTable), name, location);
    if (!symbol) {
        ReportError("Invalid redeclaration of identifier!");
        return NULL;
    }

    return declaration;
}

/// grammar: value-declaration               := constant-declaration | variable-declaration | parameter-declaration |
///                                             enumeration-element-declaration
/// grammar: constant-declaration            := "let" identifier ":" type-identifier "=" expression
/// grammar: variable-declaration            := "var" identifier ":" type-identifier [ "=" expression ]
/// grammar: parameter-declaration           := identifier ":" type-identifier
/// grammar: enumeration-element-declaration := "case" identifier [ "=" expression ]
static inline ASTValueDeclarationRef _ParserParseValueDeclaration(ParserRef parser) {
    SourceRange location = {parser->cursor, parser->cursor};

    if (_ParserConsumeKeyword(parser, "let")) {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        if (!_ParserConsumeChar(parser, ":")) {
            return NULL;
        }

        ASTTypeRef type = _ParserParseType(parser);
        if (!type) {
            return NULL;
        }

        ASTBinaryOperator binary = _ParserConsumeBinaryOperator(parser);
        if (binary != ASTBinaryOperatorAssign) {
            return NULL;
        }

        ASTExpressionRef initializer = _ParserParseExpression(parser, 0, false);
        if (!initializer) {
            return NULL;
        }

        location.end                       = parser->cursor;
        ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindConstant, name, type,
                                                                              initializer);
        SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
        ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
        SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier!");
        }

        return declaration;
    } else if (_ParserConsumeKeyword(parser, "var")) {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        if (!_ParserConsumeChar(parser, ":")) {
            return NULL;
        }

        ASTTypeRef type = _ParserParseType(parser);
        if (!type) {
            return NULL;
        }

        ASTExpressionRef initializer = NULL;
        ASTBinaryOperator binary     = _ParserConsumeBinaryOperator(parser);
        if (binary == ASTBinaryOperatorAssign) {
            initializer = _ParserParseExpression(parser, 0, false);
            if (!initializer) {
                return NULL;
            }
        } else if (binary != ASTBinaryOperatorUnknown) {
            ReportError("Unexpected binary operator found!");
            return NULL;
        }

        location.end                       = parser->cursor;
        ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindVariable, name, type,
                                                                              initializer);
        SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
        ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
        SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier!");
        }

        return declaration;
    } else if (_ParserConsumeKeyword(parser, "case")) {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        ASTExpressionRef initializer = NULL;
        ASTBinaryOperator binary     = _ParserConsumeBinaryOperator(parser);
        if (binary == ASTBinaryOperatorAssign) {
            initializer = _ParserParseExpression(parser, 0, false);
            if (!initializer) {
                return NULL;
            }
        } else if (binary != ASTBinaryOperatorUnknown) {
            ReportError("Unexpected binary operator found!");
            return NULL;
        }

        ASTTypeRef type = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt);

        location.end                       = parser->cursor;
        ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindEnumerationElement,
                                                                              name, type, initializer);
        SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
        ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
        SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier!");
        }

        return declaration;
    } else {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        if (!_ParserConsumeChar(parser, ":")) {
            return NULL;
        }

        ASTTypeRef type = _ParserParseType(parser);
        if (!type) {
            return NULL;
        }

        location.end                       = parser->cursor;
        ASTValueDeclarationRef declaration = ASTContextCreateValueDeclaration(parser->context, location, ASTValueKindParameter, name, type,
                                                                              NULL);
        SymbolTableRef symbolTable         = ASTContextGetSymbolTable(parser->context);
        ScopeRef scope                     = SymbolTableGetCurrentScope(symbolTable);
        SymbolRef symbol                   = ScopeInsertSymbol(scope, name, location);
        if (!symbol) {
            ReportError("Invalid redeclaration of identifier!");
        }

        return declaration;
    }
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
    SourceRange location = {parser->cursor, parser->cursor};
    ASTTypeRef result    = NULL;

    // TODO: Add support for source location of builtin type!
    if (_ParserConsumeKeyword(parser, "Void")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindVoid);
    } else if (_ParserConsumeKeyword(parser, "Bool")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindBool);
    } else if (_ParserConsumeKeyword(parser, "Int8")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt8);
    } else if (_ParserConsumeKeyword(parser, "Int16")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt16);
    } else if (_ParserConsumeKeyword(parser, "Int32")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt32);
    } else if (_ParserConsumeKeyword(parser, "Int64")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt64);
    } else if (_ParserConsumeKeyword(parser, "Int128")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt128);
    } else if (_ParserConsumeKeyword(parser, "Int")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindInt);
    } else if (_ParserConsumeKeyword(parser, "UInt8")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt8);
    } else if (_ParserConsumeKeyword(parser, "UInt16")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt16);
    } else if (_ParserConsumeKeyword(parser, "UInt32")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt32);
    } else if (_ParserConsumeKeyword(parser, "UInt64")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt64);
    } else if (_ParserConsumeKeyword(parser, "UInt128")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt128);
    } else if (_ParserConsumeKeyword(parser, "UInt")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindUInt);
    } else if (_ParserConsumeKeyword(parser, "Float16")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat16);
    } else if (_ParserConsumeKeyword(parser, "Float32")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat32);
    } else if (_ParserConsumeKeyword(parser, "Float64")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat64);
    } else if (_ParserConsumeKeyword(parser, "Float128")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat128);
    } else if (_ParserConsumeKeyword(parser, "Float")) {
        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextGetBuiltinType(parser->context, ASTBuiltinTypeKindFloat);
    } else {
        StringRef name = _ParserConsumeIdentifier(parser);
        if (!name) {
            return NULL;
        }

        location.end = parser->cursor;
        result       = (ASTTypeRef)ASTContextCreateOpaqueType(parser->context, location, name);
    }

    while (true) {
        if (_ParserConsumeChar(parser, "*")) {
            location.end = parser->cursor;
            result       = (ASTTypeRef)ASTContextCreatePointerType(parser->context, location, result);
        } else if (_ParserConsumeChar(parser, "[")) {
            ASTExpressionRef size = NULL;
            if (!_ParserIsChar(parser, "]")) {
                size = _ParserParseExpression(parser, 0, false);
                if (!size) {
                    return NULL;
                }
            }

            if (!_ParserConsumeChar(parser, "]")) {
                return NULL;
            }

            location.end = parser->cursor;
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

    while (_ParserConsumeChar(parser, ",")) {
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
    if (parser->cursor >= parser->bufferEnd) {
        return NULL;
    }

    if (*parser->cursor == '#') {
        return (ASTNodeRef)_ParserParseDirective(parser);
    }

    if (_ParserIsKeyword(parser, "enum")) {
        return (ASTNodeRef)_ParserParseEnumerationDeclaration(parser);
    }

    if (_ParserIsKeyword(parser, "func")) {
        return (ASTNodeRef)_ParserParseFunctionDeclaration(parser);
    }

    if (_ParserIsKeyword(parser, "struct")) {
        return (ASTNodeRef)_ParserParseStructureDeclaration(parser);
    }

    if (_ParserIsKeyword(parser, "var") || _ParserIsKeyword(parser, "let")) {
        ASTValueDeclarationRef value = _ParserParseValueDeclaration(parser);
        if (!value || (value->kind != ASTValueKindVariable && value->kind != ASTValueKindConstant)) {
            return NULL;
        }

        return (ASTNodeRef)value;
    }

    ReportError("Expected top level node!");
    return NULL;
}
