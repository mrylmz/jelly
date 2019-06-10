//
// MIT License
//
// Copyright (c) 2018 Murat Yilmaz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <gtest/gtest.h>

using namespace jelly;
using namespace jelly::AST;
using namespace jelly::Parse;

#define EXPECT_TOKEN_KINDS_EQ(__SOURCE__, ...)                              \
{                                                                           \
    const Token::Kind kinds[] = {                                           \
        __VA_ARGS__,                                                        \
        Token::Kind::EndOfFile                                              \
    };                                                                      \
    SourceManager sourceManager;                                            \
    SourceBuffer sourceBuffer = sourceManager.addSourceBuffer(__SOURCE__);  \
    Context context;                                                        \
    Lexer lexer(&context, sourceBuffer);                                    \
                                                                            \
    for (size_t i = 0; i < sizeof(kinds) / sizeof(uint32_t); i++) {         \
        Token token = lexer.lexToken();                                     \
        EXPECT_EQ(token.getKind(), kinds[i]);                               \
    }                                                                       \
}

#define EXPECT_OPERATOR_KIND_EQ(__SOURCE__, __FIXITY__)                     \
{                                                                           \
    Context      context;                                                   \
    SourceManager sourceManager;                                            \
    SourceBuffer sourceBuffer = sourceManager.addSourceBuffer(__SOURCE__);  \
    Lexer lexer(&context, sourceBuffer);                                    \
    Operator     op = Operator::BitwiseNot;                                 \
    Token        token = lexer.lexToken();                                  \
                                                                            \
    EXPECT_EQ(token.getKind(), Token::Kind::Operator);                      \
    EXPECT_TRUE(context.getOperator(token.getText(), __FIXITY__, op));      \
    EXPECT_TRUE(op.getFixity() == __FIXITY__);                              \
}

TEST(Lexer, Directives) {
    EXPECT_TOKEN_KINDS_EQ("#load",
                          Token::Kind::KeywordLoad);
}

TEST(Lexer, InvalidDirectiveName) {
    EXPECT_TOKEN_KINDS_EQ("#hello",
                          Token::Kind::Unknown);
}

TEST(Lexer, InvalidDirectiveWithoutName) {
    EXPECT_TOKEN_KINDS_EQ("#",
                          Token::Kind::Unknown);
}

TEST(Lexer, Keywords) {
    EXPECT_TOKEN_KINDS_EQ("func enum struct var let break case continue defer do else fallthrough guard if return switch while as false is nil true",
                         Token::Kind::KeywordFunc, Token::Kind::KeywordEnum, Token::Kind::KeywordStruct, Token::Kind::KeywordVar, Token::Kind::KeywordLet, Token::Kind::KeywordBreak, Token::Kind::KeywordCase, Token::Kind::KeywordContinue, Token::Kind::KeywordDefer, Token::Kind::KeywordDo, Token::Kind::KeywordElse, Token::Kind::KeywordFallthrough,
                         Token::Kind::KeywordGuard, Token::Kind::KeywordIf, Token::Kind::KeywordReturn, Token::Kind::KeywordSwitch, Token::Kind::KeywordWhile, Token::Kind::Operator, Token::Kind::KeywordFalse, Token::Kind::Operator, Token::Kind::KeywordNil, Token::Kind::KeywordTrue);
}

TEST(Lexer, EmptyFuncDecl) {
    EXPECT_TOKEN_KINDS_EQ("func _myFunc1() -> Void {}", Token::Kind::KeywordFunc, Token::Kind::Identifier, Token::Kind::LeftParenthesis, Token::Kind::RightParenthesis, Token::Kind::Arrow, Token::Kind::Identifier, Token::Kind::LeftBrace, Token::Kind::RightBrace)
}

TEST(Lexer, EmptyEnumDecl) {
    EXPECT_TOKEN_KINDS_EQ("enum MyEnum {}", Token::Kind::KeywordEnum, Token::Kind::Identifier, Token::Kind::LeftBrace, Token::Kind::RightBrace)
}

TEST(Lexer, EmptyStructDecl) {
    EXPECT_TOKEN_KINDS_EQ("struct MyStruct {}", Token::Kind::KeywordStruct, Token::Kind::Identifier, Token::Kind::LeftBrace, Token::Kind::RightBrace)
}

TEST(Lexer, IntVarDecl) {
    EXPECT_TOKEN_KINDS_EQ("var myVar: Int", Token::Kind::KeywordVar, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::Identifier)
}

TEST(Lexer, BoolLetDecl) {
    EXPECT_TOKEN_KINDS_EQ("let myVar1: Bool", Token::Kind::KeywordLet, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::Identifier)
}

TEST(Lexer, FuncDeclBreakStmt) {
    EXPECT_TOKEN_KINDS_EQ("func myFunc_1() { break }", Token::Kind::KeywordFunc, Token::Kind::Identifier, Token::Kind::LeftParenthesis, Token::Kind::RightParenthesis, Token::Kind::LeftBrace, Token::Kind::KeywordBreak, Token::Kind::RightBrace)
}

TEST(Lexer, SwitchWithCases) {
    EXPECT_TOKEN_KINDS_EQ("switch myEnum {\n"
                         "case myCaseA:fallthrough\n"
                         "case myCaseB :fallthrough\n"
                         "case myCaseC:fallthrough\n"
                         "else:break\n"
                         "}",
                         Token::Kind::KeywordSwitch, Token::Kind::Identifier, Token::Kind::LeftBrace,
                         Token::Kind::KeywordCase, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::KeywordFallthrough,
                         Token::Kind::KeywordCase, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::KeywordFallthrough,
                         Token::Kind::KeywordCase, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::KeywordFallthrough,
                         Token::Kind::KeywordElse, Token::Kind::Colon, Token::Kind::KeywordBreak,
                         Token::Kind::RightBrace)
}

TEST(Lexer, WhileStmtContinueStmt) {
    EXPECT_TOKEN_KINDS_EQ("while true {"
                         "    guard myCondition else { continue }"
                         "}",
                         Token::Kind::KeywordWhile, Token::Kind::KeywordTrue, Token::Kind::LeftBrace,
                         Token::Kind::KeywordGuard, Token::Kind::Identifier, Token::Kind::KeywordElse, Token::Kind::LeftBrace, Token::Kind::KeywordContinue, Token::Kind::RightBrace,
                         Token::Kind::RightBrace)
}

TEST(Lexer, DeferStmt) {
    EXPECT_TOKEN_KINDS_EQ("defer free(memory)",
                         Token::Kind::KeywordDefer, Token::Kind::Identifier, Token::Kind::LeftParenthesis, Token::Kind::Identifier, Token::Kind::RightParenthesis)
}

TEST(Lexer, DoWhileStmt) {
    EXPECT_TOKEN_KINDS_EQ("do { _123_myFunc19238() } while !false",
                         Token::Kind::KeywordDo, Token::Kind::LeftBrace, Token::Kind::Identifier, Token::Kind::LeftParenthesis, Token::Kind::RightParenthesis, Token::Kind::RightBrace, Token::Kind::KeywordWhile, Token::Kind::Operator, Token::Kind::KeywordFalse)
}

TEST(Lexer, IfStmt) {
    EXPECT_TOKEN_KINDS_EQ("if myCondition { }",
                         Token::Kind::KeywordIf, Token::Kind::Identifier, Token::Kind::LeftBrace, Token::Kind::RightBrace)
}

TEST(Lexer, ReturnStmt) {
    EXPECT_TOKEN_KINDS_EQ("guard myCondition else return",
                         Token::Kind::KeywordGuard, Token::Kind::Identifier, Token::Kind::KeywordElse, Token::Kind::KeywordReturn)
}

TEST(Lexer, TypeCaseExpr) {
    EXPECT_TOKEN_KINDS_EQ("myType as Int",
                         Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier)
}

TEST(Lexer, TypeCheckExpr) {
    EXPECT_TOKEN_KINDS_EQ("myType is Int",
                         Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier)
}

TEST(Lexer, AssignNil) {
    EXPECT_TOKEN_KINDS_EQ("myType = nil",
                         Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::KeywordNil)
}

TEST(Lexer, SelfDotPosition) {
    EXPECT_TOKEN_KINDS_EQ("self.position",
                         Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier)
}

TEST(Lexer, InvalidBinaryIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0b101001010181", Token::Kind::Unknown);
}

TEST(Lexer, InvalidOctalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0o8182380999", Token::Kind::Unknown);
}

TEST(Lexer, InvalidDecimalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("87712837a", Token::Kind::Unknown);
}

TEST(Lexer, InvalidHexIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0x812938APP", Token::Kind::Unknown);
}

TEST(Lexer, BinaryIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0b10101111010", Token::Kind::LiteralInt);
}

TEST(Lexer, OctalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0o761253715234176123", Token::Kind::LiteralInt);
}

TEST(Lexer, DecimalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("981723981273812398", Token::Kind::LiteralInt);
}

TEST(Lexer, HexIntegerLiteral0) {
    EXPECT_TOKEN_KINDS_EQ("0x198823affeb", Token::Kind::LiteralInt);
}

TEST(Lexer, HexIntegerLiteral1) {
    EXPECT_TOKEN_KINDS_EQ("0x198823AFFEB", Token::Kind::LiteralInt);
}


TEST(Lexer, DecimalFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0.8282178123781283", Token::Kind::LiteralFloat)
    EXPECT_TOKEN_KINDS_EQ("1238.19238193", Token::Kind::LiteralFloat);
}

TEST(Lexer, HexFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0xFFp123", Token::Kind::LiteralFloat);
}

TEST(Lexer, HexFloatLiteralWithFraction) {
    EXPECT_TOKEN_KINDS_EQ("0xFF.1FFEp12", Token::Kind::LiteralFloat);
}

TEST(Lexer, InvalidHexFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k", Token::Kind::Unknown);
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k...10.2", Token::Kind::Unknown, Token::Kind::Operator, Token::Kind::LiteralFloat);
}

TEST(Lexer, StringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("var name: String = \"Hello World\"",
                         Token::Kind::KeywordVar, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::LiteralString);
}

TEST(Lexer, StringLiteralWithEscapeCharacters) {
    EXPECT_TOKEN_KINDS_EQ("\" \\0 \\\\ \\t \\n \\r \\\" \\\' \"", Token::Kind::LiteralString);
}

TEST(Lexer, StringLiteralInvalidLineBreak) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello \n"
                         "World!\"", Token::Kind::Unknown, Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Unknown);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \r"
                         "World!\"", Token::Kind::Unknown, Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Unknown);
}

TEST(Lexer, InvalidStringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello World!", Token::Kind::Unknown);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\World \"", Token::Kind::Unknown);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\\\\" World \"", Token::Kind::LiteralString, Token::Kind::Identifier, Token::Kind::Unknown);
}

TEST(Lexer, PrefixOperatorBinaryNot) {
    EXPECT_TOKEN_KINDS_EQ("~myInteger", Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("~", Fixity::Prefix);
}

TEST(Lexer, PrefixOperatorUnaryPlus) {
    EXPECT_TOKEN_KINDS_EQ("+x", Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("+", Fixity::Prefix);
}

TEST(Lexer, PrefixOperatorUnaryMinus) {
    EXPECT_TOKEN_KINDS_EQ("-x", Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("-", Fixity::Prefix);
}

TEST(Lexer, PrefixOperatorsMixed) {
    EXPECT_TOKEN_KINDS_EQ("!+-~x", Token::Kind::Operator, Token::Kind::Operator, Token::Kind::Operator, Token::Kind::Operator, Token::Kind::Identifier);
}

TEST(Lexer, InfixOperatorBinaryLeftShift) {
    EXPECT_TOKEN_KINDS_EQ("a<<b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("<<", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBinaryRightShift) {
    EXPECT_TOKEN_KINDS_EQ("a>>b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ(">>", Fixity::Infix);
}

TEST(Lexer, InfixOperatorMultiply) {
    EXPECT_TOKEN_KINDS_EQ("a*b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("*", Fixity::Infix);
}

TEST(Lexer, InfixOperatorDivide) {
    EXPECT_TOKEN_KINDS_EQ("a/b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("/", Fixity::Infix);
}

TEST(Lexer, InfixOperatorRemainder) {
    EXPECT_TOKEN_KINDS_EQ("a%b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("%", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBinaryAnd) {
    EXPECT_TOKEN_KINDS_EQ("a&b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("&", Fixity::Infix);
}

TEST(Lexer, InfixOperatorAdd) {
    EXPECT_TOKEN_KINDS_EQ("a+b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("+", Fixity::Infix);
}

TEST(Lexer, InfixOperatorSubtract) {
    EXPECT_TOKEN_KINDS_EQ("a-b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("-", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBinaryOr) {
    EXPECT_TOKEN_KINDS_EQ("a|b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("|", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBinaryXor) {
    EXPECT_TOKEN_KINDS_EQ("a^b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("^", Fixity::Infix);
}

TEST(Lexer, InfixOperatorTypeCheck) {
    EXPECT_TOKEN_KINDS_EQ("a is B", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("is", Fixity::Infix);
}

TEST(Lexer, InfixOperatorTypeCast) {
    EXPECT_TOKEN_KINDS_EQ("a as B", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("as", Fixity::Infix);
}

TEST(Lexer, InfixOperatorLessThan) {
    EXPECT_TOKEN_KINDS_EQ("a < b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("<", Fixity::Infix);
}

TEST(Lexer, InfixOperatorLessThanEqual) {
    EXPECT_TOKEN_KINDS_EQ("a <= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("<=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorGreaterThan) {
    EXPECT_TOKEN_KINDS_EQ("a > b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ(">", Fixity::Infix);
}

TEST(Lexer, InfixOperatorGreaterThanEqual) {
    EXPECT_TOKEN_KINDS_EQ("a >= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ(">=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorEqual) {
    EXPECT_TOKEN_KINDS_EQ("a == b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("==", Fixity::Infix);
}

TEST(Lexer, InfixOperatorNotEqual) {
    EXPECT_TOKEN_KINDS_EQ("a != b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("!=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorLogicalAnd) {
    EXPECT_TOKEN_KINDS_EQ("a && b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("&&", Fixity::Infix);
}

TEST(Lexer, InfixOperatorLogicalOr) {
    EXPECT_TOKEN_KINDS_EQ("a || b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("||", Fixity::Infix);
}

TEST(Lexer, InfixOperatorAssign) {
    EXPECT_TOKEN_KINDS_EQ("a = b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorMultiplyAssign) {
    EXPECT_TOKEN_KINDS_EQ("a *= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("*=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorDivideAssign) {
    EXPECT_TOKEN_KINDS_EQ("a /= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("/=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorRemainderAssign) {
    EXPECT_TOKEN_KINDS_EQ("a %= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("%=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorAddAssign) {
    EXPECT_TOKEN_KINDS_EQ("a += b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("+=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorSubtractAssign) {
    EXPECT_TOKEN_KINDS_EQ("a -= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("-=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBitwiseLeftShiftAssign) {
    EXPECT_TOKEN_KINDS_EQ("a <<= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("<<=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBitwiseRightShiftAssign) {
    EXPECT_TOKEN_KINDS_EQ("a >>= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ(">>=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBitwiseAndAssign) {
    EXPECT_TOKEN_KINDS_EQ("a &= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("<<=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBitwiseOrAssign) {
    EXPECT_TOKEN_KINDS_EQ("a |= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("|=", Fixity::Infix);
}

TEST(Lexer, InfixOperatorBitwiseXorAssign) {
    EXPECT_TOKEN_KINDS_EQ("a ^= b", Token::Kind::Identifier, Token::Kind::Operator, Token::Kind::Identifier);
    EXPECT_OPERATOR_KIND_EQ("^=", Fixity::Infix);
}

TEST(Lexer, SinglelineComment) {
    EXPECT_TOKEN_KINDS_EQ("if condition { // I am a singleline comment\n"
                         "//And i am another singleline comment\n"
                         "    return true\n"
                         "    //me too\n"
                         "}",
                         Token::Kind::KeywordIf, Token::Kind::Identifier, Token::Kind::LeftBrace,
                         Token::Kind::KeywordReturn, Token::Kind::KeywordTrue,
                         Token::Kind::RightBrace);
}

TEST(Lexer, MultilineComment) {
    EXPECT_TOKEN_KINDS_EQ("if /* i am a really important note on the predicate */ predicate { /* and I am a multiline comment\n"
                         "    splitten up into multiple lines\n"
                         "    just because i can ;)*/return true\n"
                         "    // me not ;(\n"
                         " /* multiline /*\n"
                         "comments /* can */ also \n"
                         "*/ be nested! */ \n"
                         "}",
                         Token::Kind::KeywordIf, Token::Kind::Identifier, Token::Kind::LeftBrace,
                         Token::Kind::KeywordReturn, Token::Kind::KeywordTrue,
                         Token::Kind::RightBrace);
}

TEST(Lexer, SkipHashBang) {
    EXPECT_TOKEN_KINDS_EQ("#! this will be skipped by the lexer\n"
                         "var this: Not",
                         Token::Kind::KeywordVar, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::Identifier);
}

TEST(Lexer, InvalidNestedMultilineComment) {
    EXPECT_TOKEN_KINDS_EQ("/* This is a /*\n"
                         "broken comment */",
                         Token::Kind::Unknown);
}

TEST(Lexer, lex$load_directive_string_literal) {
    EXPECT_TOKEN_KINDS_EQ("#load \"path/to/Source.jelly\"",
                          Token::Kind::KeywordLoad, Token::Kind::LiteralString);
}

TEST(Lexer, lex$parameter_list) {
    EXPECT_TOKEN_KINDS_EQ("(a: Int, b: Int)",
                          Token::Kind::LeftParenthesis, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::Identifier, Token::Kind::Comma, Token::Kind::Identifier, Token::Kind::Colon, Token::Kind::Identifier, Token::Kind::RightParenthesis);
}
