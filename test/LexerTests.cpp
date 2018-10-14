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
#include <Basic/Basic.h>
#include <Syntax/Syntax.h>
#include <Parse/Parse.h>

#define EXPECT_TOKEN_KINDS_EQ(__SOURCE__, ...)                           \
{                                                                       \
    const uint32_t kinds[] = {                                          \
        __VA_ARGS__,                                                    \
        TOKEN_EOF                                                       \
    };                                                                  \
    Lexer lexer(__SOURCE__);                                            \
    Token token;                                                        \
                                                                        \
    for (size_t i = 0; i < sizeof(kinds) / sizeof(uint32_t); i++) {     \
        lexer.lex(token);                                               \
        EXPECT_EQ(token.kind, kinds[i]);                                \
    }                                                                   \
}

#define EXPECT_OPERATOR_KIND_EQ(__SOURCE__, __OP_KIND__)        \
{                                                               \
    Operator     op;                                            \
    Lexer        lexer(__SOURCE__);                             \
    Token        token;                                         \
                                                                \
    lexer.lex(token);                                           \
                                                                \
    EXPECT_EQ(token.kind, TOKEN_OPERATOR);                      \
    EXPECT_TRUE(lexer.get_operator(token, __OP_KIND__, op));    \
    EXPECT_TRUE(op.is_valid());                                 \
}

TEST(Lexer, Directives) {
    EXPECT_TOKEN_KINDS_EQ("#load",
                          TOKEN_KEYWORD_LOAD);
}

TEST(Lexer, InvalidDirectiveName) {
    EXPECT_TOKEN_KINDS_EQ("#hello",
                          TOKEN_UNKNOWN);
}

TEST(Lexer, InvalidDirectiveWithoutName) {
    EXPECT_TOKEN_KINDS_EQ("#",
                          TOKEN_UNKNOWN);
}

TEST(Lexer, Keywords) {
    EXPECT_TOKEN_KINDS_EQ("func enum struct var let break case continue defer do else fallthrough for guard if in return switch while as Any false is nil true",
                         TOKEN_KEYWORD_FUNC, TOKEN_KEYWORD_ENUM, TOKEN_KEYWORD_STRUCT, TOKEN_KEYWORD_VAR, TOKEN_KEYWORD_LET, TOKEN_KEYWORD_BREAK, TOKEN_KEYWORD_CASE, TOKEN_KEYWORD_CONTINUE, TOKEN_KEYWORD_DEFER, TOKEN_KEYWORD_DO, TOKEN_KEYWORD_ELSE, TOKEN_KEYWORD_FALLTHROUGH,
                         TOKEN_KEYWORD_FOR, TOKEN_KEYWORD_GUARD, TOKEN_KEYWORD_IF, TOKEN_KEYWORD_IN, TOKEN_KEYWORD_RETURN, TOKEN_KEYWORD_SWITCH, TOKEN_KEYWORD_WHILE, TOKEN_OPERATOR, TOKEN_KEYWORD_ANY, TOKEN_KEYWORD_FALSE, TOKEN_OPERATOR, TOKEN_KEYWORD_NIL, TOKEN_KEYWORD_TRUE);
}

TEST(Lexer, EmptyFuncDecl) {
    EXPECT_TOKEN_KINDS_EQ("func _myFunc1() -> Void {}", TOKEN_KEYWORD_FUNC, TOKEN_IDENTIFIER, '(', ')', TOKEN_ARROW, TOKEN_IDENTIFIER, '{', '}')
}

TEST(Lexer, EmptyEnumDecl) {
    EXPECT_TOKEN_KINDS_EQ("enum MyEnum {}", TOKEN_KEYWORD_ENUM, TOKEN_IDENTIFIER, '{', '}')
}

TEST(Lexer, EmptyStructDecl) {
    EXPECT_TOKEN_KINDS_EQ("struct MyStruct {}", TOKEN_KEYWORD_STRUCT, TOKEN_IDENTIFIER, '{', '}')
}

TEST(Lexer, IntVarDecl) {
    EXPECT_TOKEN_KINDS_EQ("var myVar: Int", TOKEN_KEYWORD_VAR, TOKEN_IDENTIFIER, ':', TOKEN_IDENTIFIER)
}

TEST(Lexer, BoolLetDecl) {
    EXPECT_TOKEN_KINDS_EQ("let myVar1: Bool", TOKEN_KEYWORD_LET, TOKEN_IDENTIFIER, ':', TOKEN_IDENTIFIER)
}

TEST(Lexer, FuncDeclBreakStmt) {
    EXPECT_TOKEN_KINDS_EQ("func myFunc_1() { break }", TOKEN_KEYWORD_FUNC, TOKEN_IDENTIFIER, '(', ')', '{', TOKEN_KEYWORD_BREAK, '}')
}

TEST(Lexer, SwitchWithCases) {
    EXPECT_TOKEN_KINDS_EQ("switch myEnum {\n"
                         "case myCaseA:fallthrough\n"
                         "case myCaseB :fallthrough\n"
                         "case myCaseC:fallthrough\n"
                         "else:break\n"
                         "}",
                         TOKEN_KEYWORD_SWITCH, TOKEN_IDENTIFIER, '{',
                         TOKEN_KEYWORD_CASE, TOKEN_IDENTIFIER, ':', TOKEN_KEYWORD_FALLTHROUGH,
                         TOKEN_KEYWORD_CASE, TOKEN_IDENTIFIER, ':', TOKEN_KEYWORD_FALLTHROUGH,
                         TOKEN_KEYWORD_CASE, TOKEN_IDENTIFIER, ':', TOKEN_KEYWORD_FALLTHROUGH,
                         TOKEN_KEYWORD_ELSE, ':', TOKEN_KEYWORD_BREAK,
                         '}')
}

TEST(Lexer, WhileStmtContinueStmt) {
    EXPECT_TOKEN_KINDS_EQ("while true {"
                         "    guard myCondition else { continue }"
                         "}",
                         TOKEN_KEYWORD_WHILE, TOKEN_KEYWORD_TRUE, '{',
                         TOKEN_KEYWORD_GUARD, TOKEN_IDENTIFIER, TOKEN_KEYWORD_ELSE, '{', TOKEN_KEYWORD_CONTINUE, '}',
                         '}')
}

TEST(Lexer, DeferStmt) {
    EXPECT_TOKEN_KINDS_EQ("defer free(memory)",
                         TOKEN_KEYWORD_DEFER, TOKEN_IDENTIFIER, '(', TOKEN_IDENTIFIER, ')')
}

TEST(Lexer, DoWhileStmt) {
    EXPECT_TOKEN_KINDS_EQ("do { _123_myFunc19238() } while !false",
                         TOKEN_KEYWORD_DO, '{', TOKEN_IDENTIFIER, '(', ')', '}', TOKEN_KEYWORD_WHILE, TOKEN_OPERATOR, TOKEN_KEYWORD_FALSE)
}

TEST(Lexer, ForLoop) {
    EXPECT_TOKEN_KINDS_EQ("for i in array { print(i) }",
                         TOKEN_KEYWORD_FOR, TOKEN_IDENTIFIER, TOKEN_KEYWORD_IN, TOKEN_IDENTIFIER, '{', TOKEN_IDENTIFIER, '(', TOKEN_IDENTIFIER, ')', '}')
}

TEST(Lexer, IfStmt) {
    EXPECT_TOKEN_KINDS_EQ("if myCondition { }",
                         TOKEN_KEYWORD_IF, TOKEN_IDENTIFIER, '{', '}')
}

TEST(Lexer, ReturnStmt) {
    EXPECT_TOKEN_KINDS_EQ("guard myCondition else return",
                         TOKEN_KEYWORD_GUARD, TOKEN_IDENTIFIER, TOKEN_KEYWORD_ELSE, TOKEN_KEYWORD_RETURN)
}

TEST(Lexer, TypeCaseExpr) {
    EXPECT_TOKEN_KINDS_EQ("myType as Int",
                         TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER)
}

TEST(Lexer, AnyTypeDecl) {
    EXPECT_TOKEN_KINDS_EQ("var myVar: Any",
                         TOKEN_KEYWORD_VAR, TOKEN_IDENTIFIER, ':', TOKEN_KEYWORD_ANY)
}

TEST(Lexer, TypeCheckExpr) {
    EXPECT_TOKEN_KINDS_EQ("myType is Int",
                         TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER)
}

TEST(Lexer, AssignNil) {
    EXPECT_TOKEN_KINDS_EQ("myType = nil",
                         TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_KEYWORD_NIL)
}

TEST(Lexer, SelfDotPosition) {
    EXPECT_TOKEN_KINDS_EQ("self.position",
                         TOKEN_IDENTIFIER, '.', TOKEN_IDENTIFIER)
}

TEST(Lexer, InvalidBinaryIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0b101001010181", TOKEN_UNKNOWN);
}

TEST(Lexer, InvalidOctalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0o8182380999", TOKEN_UNKNOWN);
}

TEST(Lexer, InvalidDecimalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("87712837a", TOKEN_UNKNOWN);
}

TEST(Lexer, InvalidHexIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0x812938APP", TOKEN_UNKNOWN);
}

TEST(Lexer, BinaryIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0b10101111010", TOKEN_LITERAL_INT);
}

TEST(Lexer, OctalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0o761253715234176123", TOKEN_LITERAL_INT);
}

TEST(Lexer, DecimalIntegerLiteral) {
    EXPECT_TOKEN_KINDS_EQ("981723981273812398", TOKEN_LITERAL_INT);
}

TEST(Lexer, HexIntegerLiteral0) {
    EXPECT_TOKEN_KINDS_EQ("0x198823affeb", TOKEN_LITERAL_INT);
}

TEST(Lexer, HexIntegerLiteral1) {
    EXPECT_TOKEN_KINDS_EQ("0x198823AFFEB", TOKEN_LITERAL_INT);
}


TEST(Lexer, DecimalFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0.8282178123781283", TOKEN_LITERAL_FLOAT)
    EXPECT_TOKEN_KINDS_EQ("1238.19238193", TOKEN_LITERAL_FLOAT);
}

TEST(Lexer, HexFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0xFFp123", TOKEN_LITERAL_FLOAT);
}

TEST(Lexer, HexFloatLiteralWithFraction) {
    EXPECT_TOKEN_KINDS_EQ("0xFF.1FFEp12", TOKEN_LITERAL_FLOAT);
}

TEST(Lexer, InvalidHexFloatLiteral) {
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k", TOKEN_UNKNOWN);
    EXPECT_TOKEN_KINDS_EQ("0xFgF.2p2k...10.2", TOKEN_UNKNOWN, TOKEN_OPERATOR, TOKEN_LITERAL_FLOAT);
}

TEST(Lexer, StringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("var name: String = \"Hello World\"",
                         TOKEN_KEYWORD_VAR, TOKEN_IDENTIFIER, ':', TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_LITERAL_STRING);
}

TEST(Lexer, StringLiteralWithEscapeCharacters) {
    EXPECT_TOKEN_KINDS_EQ("\" \\0 \\\\ \\t \\n \\r \\\" \\\' \"", TOKEN_LITERAL_STRING);
}

TEST(Lexer, StringLiteralInvalidLineBreak) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello \n"
                         "World!\"", TOKEN_UNKNOWN, TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_UNKNOWN);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \r"
                         "World!\"", TOKEN_UNKNOWN, TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_UNKNOWN);
}

TEST(Lexer, InvalidStringLiteral) {
    EXPECT_TOKEN_KINDS_EQ("\"Hello World!", TOKEN_UNKNOWN);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\World \"", TOKEN_UNKNOWN);
    EXPECT_TOKEN_KINDS_EQ("\"Hello \\\\\" World \"", TOKEN_LITERAL_STRING, TOKEN_IDENTIFIER, TOKEN_UNKNOWN);
}

TEST(Lexer, PrefixOperatorBinaryNot) {
    EXPECT_TOKEN_KINDS_EQ("~myInteger", TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("~", OPERATOR_PREFIX);
}

TEST(Lexer, PrefixOperatorUnaryPlus) {
    EXPECT_TOKEN_KINDS_EQ("+x", TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("+", OPERATOR_PREFIX);
}

TEST(Lexer, PrefixOperatorUnaryMinus) {
    EXPECT_TOKEN_KINDS_EQ("-x", TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("-", OPERATOR_PREFIX);
}

TEST(Lexer, PrefixOperatorsMixed) {
    EXPECT_TOKEN_KINDS_EQ("!+-~x", TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
}

TEST(Lexer, InfixOperatorBinaryLeftShift) {
    EXPECT_TOKEN_KINDS_EQ("a<<b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("<<", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBinaryRightShift) {
    EXPECT_TOKEN_KINDS_EQ("a>>b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ(">>", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorMultiply) {
    EXPECT_TOKEN_KINDS_EQ("a*b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("*", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorDivide) {
    EXPECT_TOKEN_KINDS_EQ("a/b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("/", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorRemainder) {
    EXPECT_TOKEN_KINDS_EQ("a%b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("%", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBinaryAnd) {
    EXPECT_TOKEN_KINDS_EQ("a&b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("&", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorAdd) {
    EXPECT_TOKEN_KINDS_EQ("a+b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("+", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorSubtract) {
    EXPECT_TOKEN_KINDS_EQ("a-b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("-", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBinaryOr) {
    EXPECT_TOKEN_KINDS_EQ("a|b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("|", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBinaryXor) {
    EXPECT_TOKEN_KINDS_EQ("a^b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("^", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorRange) {
    EXPECT_TOKEN_KINDS_EQ("a..<b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("..<", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorRangeWithIntLiterals) {
    EXPECT_TOKEN_KINDS_EQ("0..<10", TOKEN_LITERAL_INT, TOKEN_OPERATOR, TOKEN_LITERAL_INT);
}

TEST(Lexer, InfixOperatorRangeWithFloatLiterals) {
    EXPECT_TOKEN_KINDS_EQ("3.87..<10.192", TOKEN_LITERAL_FLOAT, TOKEN_OPERATOR, TOKEN_LITERAL_FLOAT);
}

TEST(Lexer, InfixOperatorClosedRange) {
    EXPECT_TOKEN_KINDS_EQ("a...b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("...", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorClosedRangeWithIntLiterals) {
    EXPECT_TOKEN_KINDS_EQ("0...10", TOKEN_LITERAL_INT, TOKEN_OPERATOR, TOKEN_LITERAL_INT);
}

TEST(Lexer, InfixOperatorClosedRangeWithFloatLiterals) {
    EXPECT_TOKEN_KINDS_EQ("9.238e2...0xFF.1p8", TOKEN_LITERAL_FLOAT, TOKEN_OPERATOR, TOKEN_LITERAL_FLOAT);
}

TEST(Lexer, InfixOperatorTypeCheck) {
    EXPECT_TOKEN_KINDS_EQ("a is B", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("is", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorTypeCast) {
    EXPECT_TOKEN_KINDS_EQ("a as B", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("as", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorLessThan) {
    EXPECT_TOKEN_KINDS_EQ("a < b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("<", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorLessThanEqual) {
    EXPECT_TOKEN_KINDS_EQ("a <= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("<=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorGreaterThan) {
    EXPECT_TOKEN_KINDS_EQ("a > b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ(">", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorGreaterThanEqual) {
    EXPECT_TOKEN_KINDS_EQ("a >= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ(">=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorEqual) {
    EXPECT_TOKEN_KINDS_EQ("a == b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("==", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorNotEqual) {
    EXPECT_TOKEN_KINDS_EQ("a != b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("!=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorLogicalAnd) {
    EXPECT_TOKEN_KINDS_EQ("a && b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("&&", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorLogicalOr) {
    EXPECT_TOKEN_KINDS_EQ("a || b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("||", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorAssign) {
    EXPECT_TOKEN_KINDS_EQ("a = b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorMultiplyAssign) {
    EXPECT_TOKEN_KINDS_EQ("a *= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("*=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorDivideAssign) {
    EXPECT_TOKEN_KINDS_EQ("a /= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("/=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorRemainderAssign) {
    EXPECT_TOKEN_KINDS_EQ("a %= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("%=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorAddAssign) {
    EXPECT_TOKEN_KINDS_EQ("a += b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("+=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorSubtractAssign) {
    EXPECT_TOKEN_KINDS_EQ("a -= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("-=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBitwiseLeftShiftAssign) {
    EXPECT_TOKEN_KINDS_EQ("a <<= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("<<=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBitwiseRightShiftAssign) {
    EXPECT_TOKEN_KINDS_EQ("a >>= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ(">>=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBitwiseAndAssign) {
    EXPECT_TOKEN_KINDS_EQ("a &= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("<<=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBitwiseOrAssign) {
    EXPECT_TOKEN_KINDS_EQ("a |= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("|=", OPERATOR_INFIX);
}

TEST(Lexer, InfixOperatorBitwiseXorAssign) {
    EXPECT_TOKEN_KINDS_EQ("a ^= b", TOKEN_IDENTIFIER, TOKEN_OPERATOR, TOKEN_IDENTIFIER);
    EXPECT_OPERATOR_KIND_EQ("^=", OPERATOR_INFIX);
}

TEST(Lexer, SinglelineComment) {
    EXPECT_TOKEN_KINDS_EQ("if condition { // I am a singleline comment\n"
                         "//And i am another singleline comment\n"
                         "    return true\n"
                         "    //me too\n"
                         "}",
                         TOKEN_KEYWORD_IF, TOKEN_IDENTIFIER, '{',
                         TOKEN_KEYWORD_RETURN, TOKEN_KEYWORD_TRUE,
                         '}');
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
                         TOKEN_KEYWORD_IF, TOKEN_IDENTIFIER, '{',
                         TOKEN_KEYWORD_RETURN, TOKEN_KEYWORD_TRUE,
                         '}');
}

TEST(Lexer, SkipHashBang) {
    EXPECT_TOKEN_KINDS_EQ("#! this will be skipped by the lexer\n"
                         "var this: Not",
                         TOKEN_KEYWORD_VAR, TOKEN_IDENTIFIER, ':', TOKEN_IDENTIFIER);
}

TEST(Lexer, InvalidNestedMultilineComment) {
    EXPECT_TOKEN_KINDS_EQ("/* This is a /*\n"
                         "broken comment */",
                         TOKEN_UNKNOWN);
}

TEST(Lexer, lex$load_directive_string_literal) {
    EXPECT_TOKEN_KINDS_EQ("#load \"path/to/Source.jelly\"",
                          TOKEN_KEYWORD_LOAD, TOKEN_LITERAL_STRING);
}

TEST(Lexer, lex$parameter_list) {
    EXPECT_TOKEN_KINDS_EQ("(a: Int, b: Int)",
                          '(', TOKEN_IDENTIFIER, ':', TOKEN_IDENTIFIER, ',', TOKEN_IDENTIFIER, ':', TOKEN_IDENTIFIER, ')');
}
