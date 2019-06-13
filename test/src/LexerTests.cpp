#include <gtest/gtest.h>
#include <JellyCore/JellyCore.h>

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
        EXPECT_EQ(token.kind, kinds[i]);                                        \
    }                                                                           \
                                                                                \
    LexerDestroy(lexer);                                                        \
    StringDestroy(buffer);                                                      \
}

TEST(Lexer, Directives) {
    EXPECT_TOKEN_KINDS_EQ("#load",
                          TokenKindDirectiveLoad);
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
    EXPECT_TOKEN_KINDS_EQ("func enum struct var let break case continue do else fallthrough if return switch while as false is nil true",
                          TokenKindKeywordFunc,
                          TokenKindKeywordEnum,
                          TokenKindKeywordStruct,
                          TokenKindKeywordVar,
                          TokenKindKeywordLet,
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

TEST(Lexer, BoolLetDecl) {
    EXPECT_TOKEN_KINDS_EQ("let myVar1: Bool",
                          TokenKindKeywordLet,
                          TokenKindIdentifier,
                          TokenKindColon,
                          TokenKindKeywordBool);
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
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k", TokenKindUnknown);
//    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k...10.2", TokenKindError, TokenKindOperator, TokenKindLiteralFloat);
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
                         "World!\"", TokenKindError, TokenKindIdentifier, TokenKindExclamationMark, TokenKindUnknown);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \r"
                         "World!\"", TokenKindError, TokenKindIdentifier, TokenKindExclamationMark, TokenKindUnknown);
}

TEST(Lexer, InvalidStringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello World!", TokenKindUnknown);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\World \"", TokenKindUnknown);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\\\\" World \"", TokenKindLiteralString, TokenKindIdentifier, TokenKindUnknown);
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
                         TokenKindUnknown);
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
