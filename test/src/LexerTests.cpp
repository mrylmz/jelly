#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>

static inline void _PrintTokenKindDescription(TokenKind kind);

#define EXPECT_TOKEN_KINDS_EQ(__SOURCE__, ...)                                  \
{                                                                               \
    const TokenKind kinds[] = {                                                 \
        __VA_ARGS__,                                                            \
        TokenKindEndOfFile                                                      \
    };                                                                          \
    StringRef buffer = StringCreate(AllocatorGetSystemDefault(), __SOURCE__);   \
    LexerRef lexer = LexerCreate(AllocatorGetSystemDefault(), buffer);          \
    Token token;                                                                \
                                                                                \
    for (Index i = 0; i < sizeof(kinds) / sizeof(TokenKind); i++) {             \
        LexerNextToken(lexer, &token);                                          \
        EXPECT_TOKEN_KIND_EQ(token.kind, kinds[i]);                             \
    }                                                                           \
                                                                                \
    LexerDestroy(lexer);                                                        \
    StringDestroy(buffer);                                                      \
}

#define EXPECT_TOKEN_KIND_EQ(__VALUE__,__EXPECTED__)    \
{                                                       \
    if (__VALUE__ != __EXPECTED__) {                    \
        printf("\n%s", "    Expected token kind: ");    \
        _PrintTokenKindDescription(__EXPECTED__);       \
        printf("%s", "\n");                             \
        printf("%s", "    Received token kind: ");      \
        _PrintTokenKindDescription(__VALUE__);          \
        printf("%s", "\n\n");                           \
        FAIL();                                         \
    }                                                   \
}

TEST(Lexer, Directives) {
    EXPECT_TOKEN_KINDS_EQ("#load #link #import #include",
                          TokenKindDirectiveLoad,
                          TokenKindDirectiveLink,
                          TokenKindDirectiveImport,
                          TokenKindDirectiveInclude);
}

TEST(Lexer, InvalidDirectiveName) {
    EXPECT_TOKEN_KINDS_EQ("#hello",
                          TokenKindUnknown);
}

TEST(Lexer, InvalidDirectiveWithoutName) {
    EXPECT_TOKEN_KINDS_EQ("#",
                          TokenKindUnknown);
}

TEST(Lexer, Keywords) {
    EXPECT_TOKEN_KINDS_EQ("func enum struct var break case continue do else fallthrough if return switch while as false is nil true",
                          TokenKindKeywordFunc,
                          TokenKindKeywordEnum,
                          TokenKindKeywordStruct,
                          TokenKindKeywordVar,
                          TokenKindKeywordBreak,
                          TokenKindKeywordCase,
                          TokenKindKeywordContinue,
                          TokenKindKeywordDo,
                          TokenKindKeywordElse,
                          TokenKindKeywordFallthrough,
                          TokenKindKeywordIf,
                          TokenKindKeywordReturn,
                          TokenKindKeywordSwitch,
                          TokenKindKeywordWhile,
                          TokenKindKeywordAs,
                          TokenKindKeywordFalse,
                          TokenKindKeywordIs,
                          TokenKindKeywordNil,
                          TokenKindKeywordTrue);
}

TEST(Lexer, EmptyFuncDecl) {
    EXPECT_TOKEN_KINDS_EQ("func _myFunc1() -> Void {}",
                          TokenKindKeywordFunc,
                          TokenKindIdentifier,
                          TokenKindLeftParenthesis,
                          TokenKindRightParenthesis,
                          TokenKindArrow,
                          TokenKindKeywordVoid,
                          TokenKindLeftCurlyBracket,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, EmptyEnumDecl) {
    EXPECT_TOKEN_KINDS_EQ("enum MyEnum {}",
                          TokenKindKeywordEnum,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, EmptyStructDecl) {
    EXPECT_TOKEN_KINDS_EQ("struct MyStruct {}",
                          TokenKindKeywordStruct,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, IntVarDecl) {
    EXPECT_TOKEN_KINDS_EQ("var myVar: Int",
                          TokenKindKeywordVar,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordInt);
}

TEST(Lexer, FuncDeclBreakStmt) {
    EXPECT_TOKEN_KINDS_EQ("func myFunc_1() { break }",
                          TokenKindKeywordFunc,
                          TokenKindIdentifier,
                          TokenKindLeftParenthesis,
                          TokenKindRightParenthesis,
                          TokenKindLeftCurlyBracket,
                          TokenKindKeywordBreak,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, SwitchWithCases) {
    EXPECT_TOKEN_KINDS_EQ("switch myEnum {\n"
                          "case myCaseA:fallthrough\n"
                          "case myCaseB :fallthrough\n"
                          "case myCaseC:fallthrough\n"
                          "else:break\n"
                          "}",
                          TokenKindKeywordSwitch,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindKeywordCase,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordFallthrough,
                          TokenKindKeywordCase,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordFallthrough,
                          TokenKindKeywordCase,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordFallthrough,
                          TokenKindKeywordElse,
                          TokenKindColon,
                          TokenKindKeywordBreak,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, WhileStmtContinueStmt) {
    EXPECT_TOKEN_KINDS_EQ("while true {"
                          "    if myCondition { continue }"
                          "}",
                          TokenKindKeywordWhile,
                          TokenKindKeywordTrue,
                          TokenKindLeftCurlyBracket,
                          TokenKindKeywordIf,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindKeywordContinue,
                          TokenKindRightCurlyBracket,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, DoWhileStmt) {
    EXPECT_TOKEN_KINDS_EQ("do { _123_myFunc19238() } while !false",
                          TokenKindKeywordDo,
                          TokenKindLeftCurlyBracket,
                          TokenKindIdentifier,
                          TokenKindLeftParenthesis,
                          TokenKindRightParenthesis,
                          TokenKindRightCurlyBracket,
                          TokenKindKeywordWhile,
                          TokenKindExclamationMark,
                          TokenKindKeywordFalse);
}

TEST(Lexer, IfStmt) {
    EXPECT_TOKEN_KINDS_EQ("if myCondition { }",
                          TokenKindKeywordIf,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, ReturnStmt) {
    EXPECT_TOKEN_KINDS_EQ("if myCondition return",
                          TokenKindKeywordIf,
                          TokenKindIdentifier,
                          TokenKindKeywordReturn);
}

TEST(Lexer, TypeCaseExpr) {
    EXPECT_TOKEN_KINDS_EQ("myType as Int",
                          TokenKindIdentifier,
                          TokenKindKeywordAs,
                          TokenKindKeywordInt);
}

TEST(Lexer, TypeCheckExpr) {
    EXPECT_TOKEN_KINDS_EQ("myType is Int",
                          TokenKindIdentifier,
                          TokenKindKeywordIs,
                          TokenKindKeywordInt);
}

TEST(Lexer, AssignNil) {
    EXPECT_TOKEN_KINDS_EQ("myType = nil",
                          TokenKindIdentifier,
                          TokenKindEqualsSign,
                          TokenKindKeywordNil);
}

TEST(Lexer, AssignUnaryExpression) {
    EXPECT_TOKEN_KINDS_EQ("var x: Int = -10",
                          TokenKindKeywordVar,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordInt,
                          TokenKindEqualsSign,
                          TokenKindMinusSign,
                          TokenKindLiteralInt);
}

TEST(Lexer, SelfDotPosition) {
    EXPECT_TOKEN_KINDS_EQ("self.position",
                          TokenKindIdentifier,
                          TokenKindDot,
                          TokenKindIdentifier);
}

TEST(Lexer, InvalidBinaryIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0b101001010181",
                          TokenKindError);
}

TEST(Lexer, InvalidOctalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0o8182380999",
                          TokenKindError);
}

TEST(Lexer, InvalidDecimalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("87712837a",
                          TokenKindError);
}

TEST(Lexer, InvalidHexIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0x812938APP",
                          TokenKindError);
}

TEST(Lexer, BinaryIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0b10101111010", TokenKindLiteralInt);
}

TEST(Lexer, OctalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0o761253715234176123", TokenKindLiteralInt);
}

TEST(Lexer, DecimalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("981723981273812398", TokenKindLiteralInt);
}

TEST(Lexer, HexIntegerLiteral0) {
    EXPECT_TOKEN_KINDS_EQ("0x198823affeb", TokenKindLiteralInt);
}

TEST(Lexer, HexIntegerLiteral1) {
    EXPECT_TOKEN_KINDS_EQ("0x198823AFFEB", TokenKindLiteralInt);
}


TEST(Lexer, DecimalFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0.8282178123781283", TokenKindLiteralFloat)
    EXPECT_TOKEN_KINDS_EQ("1238.19238193", TokenKindLiteralFloat);
}

TEST(Lexer, HexFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0xFFp123", TokenKindLiteralFloat);
}

TEST(Lexer, HexFloatLiteralWithFraction) {
    EXPECT_TOKEN_KINDS_EQ("0xFF.1FFEp12", TokenKindLiteralFloat);
}

TEST(Lexer, InvalidHexFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k",
                          TokenKindError,
                          TokenKindDot,
                          TokenKindError);
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k...10.2",
                          TokenKindError,
                          TokenKindDot,
                          TokenKindError,
                          TokenKindDot,
                          TokenKindDot,
                          TokenKindDot,
                          TokenKindLiteralFloat);
}

TEST(Lexer, StringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("var name: String = \"Hello World\"",
                          TokenKindKeywordVar,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindIdentifier,
                          TokenKindEqualsSign,
                          TokenKindLiteralString);
}

TEST(Lexer, StringLiteralWithEscapeCharacters) {
    EXPECT_TOKEN_KINDS_EQ("\" \\0 \\\\ \\t \\n \\r \\\" \\\' \"", TokenKindLiteralString);
}

TEST(Lexer, StringLiteralInvalidLineBreak) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello \n"
                         "World!\"", TokenKindError, TokenKindIdentifier, TokenKindExclamationMark, TokenKindError);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \r"
                         "World!\"", TokenKindError, TokenKindIdentifier, TokenKindExclamationMark, TokenKindError);
}

TEST(Lexer, InvalidStringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello World!", TokenKindError);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\World \"", TokenKindError);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\\\\" World \"", TokenKindLiteralString, TokenKindIdentifier, TokenKindError);
}

TEST(Lexer, PrefixOperatorBinaryNot) {
    EXPECT_TOKEN_KINDS_EQ("~myInteger", TokenKindTilde, TokenKindIdentifier);
}

TEST(Lexer, PrefixOperatorUnaryPlus) {
    EXPECT_TOKEN_KINDS_EQ("+x", TokenKindPlusSign, TokenKindIdentifier);
}

TEST(Lexer, PrefixOperatorUnaryMinus) {
    EXPECT_TOKEN_KINDS_EQ("-x", TokenKindMinusSign, TokenKindIdentifier);
}

TEST(Lexer, PrefixOperatorsMixed) {
    EXPECT_TOKEN_KINDS_EQ("!+-~x",
                          TokenKindExclamationMark,
                          TokenKindPlusSign,
                          TokenKindMinusSign,
                          TokenKindTilde,
                          TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBinaryLeftShift) {
    EXPECT_TOKEN_KINDS_EQ("a<<b", TokenKindIdentifier, TokenKindLessThanLessThan, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBinaryRightShift) {
    EXPECT_TOKEN_KINDS_EQ("a>>b", TokenKindIdentifier, TokenKindGreaterThanGreaterThan, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorMultiply) {
    EXPECT_TOKEN_KINDS_EQ("a*b", TokenKindIdentifier, TokenKindAsterisk, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorDivide) {
    EXPECT_TOKEN_KINDS_EQ("a/b", TokenKindIdentifier, TokenKindSlash, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorRemainder) {
    EXPECT_TOKEN_KINDS_EQ("a%b", TokenKindIdentifier, TokenKindPercentSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBinaryAnd) {
    EXPECT_TOKEN_KINDS_EQ("a&b", TokenKindIdentifier, TokenKindAmpersand, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorAdd) {
    EXPECT_TOKEN_KINDS_EQ("a+b", TokenKindIdentifier, TokenKindPlusSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorSubtract) {
    EXPECT_TOKEN_KINDS_EQ("a-b", TokenKindIdentifier, TokenKindMinusSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBinaryOr) {
    EXPECT_TOKEN_KINDS_EQ("a|b", TokenKindIdentifier, TokenKindPipe, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBinaryXor) {
    EXPECT_TOKEN_KINDS_EQ("a^b", TokenKindIdentifier, TokenKindCircumflex, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorTypeCheck) {
    EXPECT_TOKEN_KINDS_EQ("a is B", TokenKindIdentifier, TokenKindKeywordIs, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorTypeCast) {
    EXPECT_TOKEN_KINDS_EQ("a as B", TokenKindIdentifier, TokenKindKeywordAs, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorLessThan) {
    EXPECT_TOKEN_KINDS_EQ("a < b", TokenKindIdentifier, TokenKindLessThan, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorLessThanEqual) {
    EXPECT_TOKEN_KINDS_EQ("a <= b", TokenKindIdentifier, TokenKindLessThanEqualsSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorGreaterThan) {
    EXPECT_TOKEN_KINDS_EQ("a > b", TokenKindIdentifier, TokenKindGreaterThan, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorGreaterThanEqual) {
    EXPECT_TOKEN_KINDS_EQ("a >= b", TokenKindIdentifier, TokenKindGreaterThanEqualsSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorEqual) {
    EXPECT_TOKEN_KINDS_EQ("a == b", TokenKindIdentifier, TokenKindEqualsEqualsSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorNotEqual) {
    EXPECT_TOKEN_KINDS_EQ("a != b", TokenKindIdentifier, TokenKindExclamationMarkEqualsSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorLogicalAnd) {
    EXPECT_TOKEN_KINDS_EQ("a && b", TokenKindIdentifier, TokenKindAmpersandAmpersand, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorLogicalOr) {
    EXPECT_TOKEN_KINDS_EQ("a || b", TokenKindIdentifier, TokenKindPipePipe, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorAssign) {
    EXPECT_TOKEN_KINDS_EQ("a = b", TokenKindIdentifier, TokenKindEqualsSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorMultiplyAssign) {
    EXPECT_TOKEN_KINDS_EQ("a *= b", TokenKindIdentifier, TokenKindAsteriskEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorDivideAssign) {
    EXPECT_TOKEN_KINDS_EQ("a /= b", TokenKindIdentifier, TokenKindSlashEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorRemainderAssign) {
    EXPECT_TOKEN_KINDS_EQ("a %= b", TokenKindIdentifier, TokenKindPercentEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorAddAssign) {
    EXPECT_TOKEN_KINDS_EQ("a += b", TokenKindIdentifier, TokenKindPlusEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorSubtractAssign) {
    EXPECT_TOKEN_KINDS_EQ("a -= b", TokenKindIdentifier, TokenKindMinusEqualsSign, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBitwiseLeftShiftAssign) {
    EXPECT_TOKEN_KINDS_EQ("a <<= b", TokenKindIdentifier, TokenKindLessThanLessThanEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBitwiseRightShiftAssign) {
    EXPECT_TOKEN_KINDS_EQ("a >>= b", TokenKindIdentifier, TokenKindGreaterThanGreaterThanEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBitwiseAndAssign) {
    EXPECT_TOKEN_KINDS_EQ("a &= b", TokenKindIdentifier, TokenKindAmpersandEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBitwiseOrAssign) {
    EXPECT_TOKEN_KINDS_EQ("a |= b", TokenKindIdentifier, TokenKindPipeEquals, TokenKindIdentifier);
}

TEST(Lexer, InfixOperatorBitwiseXorAssign) {
    EXPECT_TOKEN_KINDS_EQ("a ^= b", TokenKindIdentifier, TokenKindCircumflexEquals, TokenKindIdentifier);
}

TEST(Lexer, SinglelineComment) {
    EXPECT_TOKEN_KINDS_EQ("if condition { // I am a singleline comment\n"
                          "//And i am another singleline comment\n"
                          "    return true\n"
                          "    //me too\n"
                          "}",
                          TokenKindKeywordIf,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindKeywordReturn,
                          TokenKindKeywordTrue,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, MultilineComment) {
    EXPECT_TOKEN_KINDS_EQ("if /* i am a really important note on the predicate */ predicate { /* and I am a multiline comment\\n"
                         "    splitten up into multiple lines\n"
                         "    just because i can ;)*/return true\n"
                         "    // me not ;(\n"
                         " /* multiline /*\n"
                         "comments /* can */ also \n"
                         "*/ be nested! */ \n"
                         "}",
                          TokenKindKeywordIf,
                          TokenKindIdentifier,
                          TokenKindLeftCurlyBracket,
                          TokenKindKeywordReturn,
                          TokenKindKeywordTrue,
                          TokenKindRightCurlyBracket);
}

TEST(Lexer, SkipHashBang) {
    EXPECT_TOKEN_KINDS_EQ("#! this will be skipped by the lexer\n"
                         "var this: Not",
                          TokenKindKeywordVar,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindIdentifier);
}

TEST(Lexer, InvalidNestedMultilineComment) {
    EXPECT_TOKEN_KINDS_EQ("/* This is a /*\n"
                         "broken comment */",
                         TokenKindError);
}

TEST(Lexer, LoadDirectiveStringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("#load \"path/to/Source.jelly\"",
                          TokenKindDirectiveLoad,
                          TokenKindLiteralString);
}

TEST(Lexer, ParameterList) {
    EXPECT_TOKEN_KINDS_EQ("(a: Int, b: Int)",
                          TokenKindLeftParenthesis,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordInt,
                          TokenKindComma,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordInt,
                          TokenKindRightParenthesis);
}

static inline void _PrintTokenKindDescription(TokenKind kind) {
    switch (kind) {
        case TokenKindUnknown:
            printf("%s", "UNKNOWN");
            break;

        case TokenKindError:
            printf("%s", "ERROR");
            break;

        case TokenKindSlash:
            printf("%s", "/");
            break;

        case TokenKindSlashEquals:
            printf("%s", "/=");
            break;

        case TokenKindEqualsSign:
            printf("%s", "=");
            break;

        case TokenKindEqualsEqualsSign:
            printf("%s", "==");
            break;

        case TokenKindMinusSign:
            printf("%s", "-");
            break;

        case TokenKindMinusEqualsSign:
            printf("%s", "-=");
            break;

        case TokenKindPlusSign:
            printf("%s", "+");
            break;

        case TokenKindPlusEquals:
            printf("%s", "+=");
            break;

        case TokenKindExclamationMark:
            printf("%s", "!");
            break;

        case TokenKindExclamationMarkEqualsSign:
            printf("%s", "!=");
            break;

        case TokenKindAsterisk:
            printf("%s", "*");
            break;

        case TokenKindAsteriskEquals:
            printf("%s", "*=");
            break;

        case TokenKindPercentSign:
            printf("%s", "%");
            break;

        case TokenKindPercentEquals:
            printf("%s", "%=");
            break;

        case TokenKindDot:
            printf("%s", ".");
            break;

        case TokenKindLessThan:
            printf("%s", "<");
            break;

        case TokenKindLessThanLessThan:
            printf("%s", "<<");
            break;

        case TokenKindLessThanLessThanEquals:
            printf("%s", "<<=");
            break;

        case TokenKindLessThanEqualsSign:
            printf("%s", "<=");
            break;

        case TokenKindGreaterThan:
            printf("%s", ">");
            break;

        case TokenKindGreaterThanGreaterThan:
            printf("%s", ">>");
            break;

        case TokenKindGreaterThanGreaterThanEquals:
            printf("%s", ">>=");
            break;

        case TokenKindGreaterThanEqualsSign:
            printf("%s", ">=");
            break;

        case TokenKindAmpersand:
            printf("%s", "&");
            break;

        case TokenKindAmpersandAmpersand:
            printf("%s", "&&");
            break;

        case TokenKindAmpersandEquals:
            printf("%s", "&=");
            break;

        case TokenKindPipe:
            printf("%s", "|");
            break;

        case TokenKindPipePipe:
            printf("%s", "||");
            break;

        case TokenKindPipeEquals:
            printf("%s", "|=");
            break;

        case TokenKindCircumflex:
            printf("%s", "^");
            break;

        case TokenKindCircumflexEquals:
            printf("%s", "^=");
            break;

        case TokenKindLeftParenthesis:
            printf("%s", "(");
            break;

        case TokenKindRightParenthesis:
            printf("%s", ")");
            break;

        case TokenKindColon:
            printf("%s", ":");
            break;

        case TokenKindLeftBracket:
            printf("%s", "[");
            break;

        case TokenKindRightBracket:
            printf("%s", "]");
            break;

        case TokenKindLeftCurlyBracket:
            printf("%s", "{");
            break;

        case TokenKindRightCurlyBracket:
            printf("%s", "}");
            break;

        case TokenKindComma:
            printf("%s", ",");
            break;

        case TokenKindTilde:
            printf("%s", "~");
            break;

        case TokenKindArrow:
            printf("%s", "->");
            break;

        case TokenKindIdentifier:
            printf("%s", "IDENTIFIER");
            break;

        case TokenKindKeywordIs:
            printf("%s", "is");
            break;

        case TokenKindKeywordAs:
            printf("%s", "as");
            break;

        case TokenKindKeywordAsExclamationMark:
            printf("%s", "as!");
            break;

        case TokenKindKeywordIf:
            printf("%s", "if");
            break;

        case TokenKindKeywordElse:
            printf("%s", "else");
            break;

        case TokenKindKeywordWhile:
            printf("%s", "while");
            break;

        case TokenKindKeywordDo:
            printf("%s", "do");
            break;

        case TokenKindKeywordCase:
            printf("%s", "case");
            break;

        case TokenKindKeywordSwitch:
            printf("%s", "switch");
            break;

        case TokenKindKeywordBreak:
            printf("%s", "break");
            break;

        case TokenKindKeywordContinue:
            printf("%s", "continue");
            break;

        case TokenKindKeywordFallthrough:
            printf("%s", "fallthrough");
            break;

        case TokenKindKeywordReturn:
            printf("%s", "return");
            break;

        case TokenKindKeywordNil:
            printf("%s", "nil");
            break;

        case TokenKindKeywordTrue:
            printf("%s", "true");
            break;

        case TokenKindKeywordFalse:
            printf("%s", "false");
            break;

        case TokenKindKeywordEnum:
            printf("%s", "enum");
            break;

        case TokenKindKeywordFunc:
            printf("%s", "func");
            break;

        case TokenKindKeywordInit:
            printf("%s", "init");
            break;

        case TokenKindKeywordPrefix:
            printf("%s", "prefix");
            break;

        case TokenKindKeywordInfix:
            printf("%s", "infix");
            break;

        case TokenKindKeywordStruct:
            printf("%s", "struct");
            break;

        case TokenKindKeywordVar:
            printf("%s", "var");
            break;

        case TokenKindKeywordTypeAlias:
            printf("%s", "typealias");
            break;

        case TokenKindKeywordSizeOf:
            printf("%s", "sizeof");
            break;

        case TokenKindKeywordVoid:
            printf("%s", "Void");
            break;

        case TokenKindKeywordBool:
            printf("%s", "Bool");
            break;

        case TokenKindKeywordInt8:
            printf("%s", "Int8");
            break;

        case TokenKindKeywordInt16:
            printf("%s", "Int16");
            break;

        case TokenKindKeywordInt32:
            printf("%s", "Int32");
            break;

        case TokenKindKeywordInt64:
            printf("%s", "Int64");
            break;

        case TokenKindKeywordInt:
            printf("%s", "Int");
            break;

        case TokenKindKeywordUInt8:
            printf("%s", "UInt8");
            break;

        case TokenKindKeywordUInt16:
            printf("%s", "UInt16");
            break;

        case TokenKindKeywordUInt32:
            printf("%s", "UInt32");
            break;

        case TokenKindKeywordUInt64:
            printf("%s", "UInt64");
            break;

        case TokenKindKeywordUInt:
            printf("%s", "UInt");
            break;

        case TokenKindKeywordFloat32:
            printf("%s", "Float32");
            break;

        case TokenKindKeywordFloat64:
            printf("%s", "Float64");
            break;

        case TokenKindKeywordFloat:
            printf("%s", "Float");
            break;

        case TokenKindKeywordModule:
            printf("%s", "module");
            break;

        case TokenKindDirectiveLoad:
            printf("%s", "#load");
            break;

        case TokenKindDirectiveLink:
            printf("%s", "#link");
            break;

        case TokenKindDirectiveIntrinsic:
            printf("%s", "#intrinsic");
            break;

        case TokenKindDirectiveForeign:
            printf("%s", "#foreign");
            break;

        case TokenKindDirectiveImport:
            printf("%s", "#import");
            break;

        case TokenKindDirectiveInclude:
            printf("%s", "#include");
            break;

        case TokenKindLiteralString:
            printf("%s", "STRING");
            break;

        case TokenKindLiteralInt:
            printf("%s", "INT");
            break;

        case TokenKindLiteralFloat:
            printf("%s", "FLOAT");
            break;

        case TokenKindEndOfFile:
            printf("%s", "EOF");
            break;
    }
}
