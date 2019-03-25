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

#include "Parse/Parser.h"

using namespace jelly::AST;
using namespace jelly::Parse;

// @Incomplete Check if symbols of unary expressions are right bound ! (unexpected: ~ value, expected: ~value)

Parser::Parser(Lexer* lexer, DiagnosticEngine* diag) :
lexer(lexer), diagnostic(diag), op(Operator::LogicalNot) {
}

void Parser::parseAllTopLevelNodes() {
    diagnostic->begin(lexer->getSourceBuffer());

    token = lexer->peekNextToken();

    auto module = getContext()->getModule();
    bool checkConsecutiveTopLevelNodes = false;
    unsigned line = token.getLine();
    do {

        unsigned nodeLine = token.getLine();
        auto declaration = parseTopLevelDeclaration();
        if (!declaration) {
            break;
        }

        if (checkConsecutiveTopLevelNodes && line == nodeLine) {
            report(Diagnostic::Level::Error, "Consecutive top level nodes on a line are not allowed!");
            break;
        }
        checkConsecutiveTopLevelNodes = true;
        line = nodeLine;

        module->addChild(declaration);

        if (declaration->isNamedDeclaration()) {
            auto namedDeclaration = reinterpret_cast<NamedDeclaration*>(declaration);
            if (!module->getScope()->insertDeclaration(namedDeclaration)) {
                diagnostic->report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", namedDeclaration->getName());
            }
        }

    } while (true);
}

void Parser::consumeToken() {
    lexer->lexToken();
    token = lexer->peekNextToken();
}

bool Parser::consumeToken(Token::Kind kind) {
    if (token.is(kind)) {
        consumeToken();
        return true;
    }

    report(Diagnostic::Level::Error, "Expected token {0} found {1}", kind, token);
    return false;
}

bool Parser::consumeIdentifier(Identifier& identifier) {
    if (token.is(Token::Kind::Identifier)) {
        identifier = getContext()->getIdentifier(token.getText());
        consumeToken();
        return true;
    }

    report(Diagnostic::Level::Error, "Expected token {0} found {1}", Token::Kind::Identifier, token);
    return false;
}

bool Parser::consumeOperator(Fixity fixity, Operator& op) {
    if (token.is(Token::Kind::Operator) && getContext()->getOperator(token.getText(), fixity, op)) {
        consumeToken();
        return true;
    }

    // @Todo convert token kind and token to string description!
    report(Diagnostic::Level::Error, "Expected Operator found {0}", token);
    return false;
}

bool Parser::tryConsumeOperator(Operator op) {
    if (!token.is(Token::Kind::Operator)) {
        return false;
    }

    if (!getContext()->getOperator(token.getText(), op.getFixity(), this->op)) {
        return false;
    }

    if (this->op != op) {
        return false;
    }

    consumeToken();
    return true;
}

Expression* Parser::tryParseExpression(Precedence precedence) {
    auto silentErrors = this->silentErrors;
    this->silentErrors = true;
    defer(this->silentErrors = silentErrors);

    auto state = lexer->getState();
    auto expression = parseExpression(precedence);
    if (!expression) {
        lexer->setState(state);
        return nullptr;
    }

    return expression;
}

MemberAccessExpression* Parser::tryParseMemberAccessExpression(Expression* left) {
    if (!tryConsumeOperator(Operator::Selector)) {
        return nullptr;
    }

    Identifier memberName;
    if (!consumeIdentifier(memberName)) {
        return nullptr;
    }

    return new (getContext()) MemberAccessExpression(left, memberName);
}

Expression* Parser::parseConditionList() {
    auto condition = parseExpression();
    if (!condition) {
        return nullptr;
    }

    while (token.is(Token::Kind::Comma)) {
        consumeToken();

        auto expression = parseExpression();
        if (!expression) {
            return nullptr;
        }

        condition = new (getContext()) BinaryExpression(Operator::LogicalAnd, condition, expression);
    }

    return condition;
}

/// grammar: atom-expression := group-expression | literal-expression | identifier-expression
/// grammar: literal-expression := literal
/// grammar: identifier-expression := identifier
Expression* Parser::parseAtomExpression() {
    switch (token.getKind()) {
        case Token::Kind::LeftParenthesis: return parseGroupExpression();
        case Token::Kind::KeywordNil:      return parseNilLiteral();
        case Token::Kind::KeywordTrue:     return parseBoolLiteral();
        case Token::Kind::KeywordFalse:    return parseBoolLiteral();
        case Token::Kind::LiteralInt:      return parseIntLiteral();
        case Token::Kind::LiteralFloat:    return parseFloatLiteral();
        case Token::Kind::LiteralString:   return parseStringLiteral();
        case Token::Kind::Identifier:      return parseIdentifierExpression();
        default:
            report(Diagnostic::Level::Error, "Expected Expression found {0}", token);
            return nullptr;
    }
}

/// grammar: primary-expression := unary-expression | atom-expression
Expression* Parser::parsePrimaryExpression() {
    if (token.is(Token::Kind::Operator)) {
        if (getContext()->getOperator(token.getText(), Fixity::Prefix, op)) {
            return parseUnaryExpression();
        } else {
            report(Diagnostic::Level::Error, "Expected Prefix Operator found {0}", token);
            return nullptr;
        }
    }

    return parseAtomExpression();
}

/// grammar: top-level-node := load-declaration | enum-declaration | func-declaration | struct-declaration | variable-declaration
Declaration* Parser::parseTopLevelDeclaration() {
    switch (token.getKind()) {
        case Token::Kind::KeywordLoad:   return parseLoadDeclaration();
        case Token::Kind::KeywordEnum:   return parseEnumeration();
        case Token::Kind::KeywordFunc:   return parseFunction();
        case Token::Kind::KeywordStruct: return parseStructure();
        case Token::Kind::KeywordVar:    return parseVariable();
        case Token::Kind::KeywordLet:    return parseConstant();
        case Token::Kind::EndOfFile:     return nullptr;
        default:
            report(Diagnostic::Level::Error, "Unexpected token {0} found expected top level declaration!", token);
            return nullptr;
    }
}

/// grammar: array-type-ref := type-ref "[" [ expression ] "]"
ArrayTypeRef* Parser::parseArrayTypeRef(TypeRef* elementTypeRef) {
    if (!consumeToken(Token::Kind::LeftBracket)) {
        return nullptr;
    }

    auto value = parseExpression();
    if (!value) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::RightBracket)) {
        return nullptr;
    }

    return  new (getContext()) ArrayTypeRef(elementTypeRef, value);
}

/// grammar: block := '{' { statement } '}'
BlockStatement* Parser::parseBlock() {
    if (!consumeToken(Token::Kind::LeftBrace)) {
        return nullptr;
    }

    jelly::Array<Statement*> statements;

    auto line = token.getLine();
    while (!token.is(Token::Kind::RightBrace)) {
        if (!statements.empty() && line == token.getLine()) {
            report(Diagnostic::Level::Error, "Consecutive statements on a line are not allowed!");
            return nullptr;
        }

        line = token.getLine();

        auto statement = parseStatement();
        if (!statement) {
            return nullptr;
        }

        statements.push_back(statement);
    }

    if (!consumeToken(Token::Kind::RightBrace)) {
        return nullptr;
    }

    auto statement = new (getContext()) BlockStatement(statements);

    for (auto child : statement->getChildren()) {
        if (child->isNamedDeclaration()) {
            auto namedChild = reinterpret_cast<NamedDeclaration*>(child);
            if (!statement->getScope()->insertDeclaration(namedChild)) {
                diagnostic->report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", namedChild->getName());
            }
        }
    }

    return statement;
}

/// grammar: bool-literal := "true" | "false"
BoolLiteral* Parser::parseBoolLiteral() {
    if (!token.is(Token::Kind::KeywordTrue, Token::Kind::KeywordFalse)) {
        report(Diagnostic::Level::Error, "Expected 'true' or 'false' found {0}", token);
        return nullptr;
    }

    bool value = token.is(Token::Kind::KeywordTrue);
    consumeToken();

    return new (getContext()) BoolLiteral(value);
}

/// grammar: break-statement := "break"
BreakStatement* Parser::parseBreak() {
    if (!consumeToken(Token::Kind::KeywordBreak)) {
        return nullptr;
    }

    return new (getContext()) BreakStatement;
}

/// grammar: call-expression := expression "(" [ expression { "," expression } ] ")"
CallExpression* Parser::parseCallExpression(Expression* callee) {
    if (!consumeToken(Token::Kind::LeftParenthesis)) {
        return nullptr;
    }

    jelly::Array<Expression*> arguments;
    while (!token.is(Token::Kind::RightParenthesis)) {
        auto argument = parseExpression();
        if (!argument) {
            return nullptr;
        }

        arguments.push_back(argument);

        if (token.is(Token::Kind::RightParenthesis)) {
            break;
        }

        if (!consumeToken(Token::Kind::Comma)) {
            return nullptr;
        }
    }

    if (!consumeToken(Token::Kind::RightParenthesis)) {
        return nullptr;
    }

    return new (getContext()) CallExpression(callee, arguments);
}

/// grammar: case-statement := conditional-case-statement | else-case-statement
CaseStatement* Parser::parseCaseStatement() {
    switch (token.getKind()) {
        case Token::Kind::KeywordCase: return parseConditionalCaseStatement();
        case Token::Kind::KeywordElse: return parseElseCaseStatement();

        default:
            report(Diagnostic::Level::Error, "Expected 'case' or 'else' found {0}", token);
            return nullptr;
    }
}

/// grammar: conditional-case-statement := "case" expression ":" statement { line-break statement }
ConditionalCaseStatement* Parser::parseConditionalCaseStatement() {
    if (!consumeToken(Token::Kind::KeywordCase)) {
        return nullptr;
    }

    auto condition = parseExpression();
    if (!condition) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::Colon)) {
        return nullptr;
    }

    jelly::Array<Statement*> statements;
    unsigned line = token.getLine();
    while (!token.is(Token::Kind::KeywordCase, Token::Kind::KeywordElse, Token::Kind::RightBrace)) {
        if (!statements.empty() && line == token.getLine()) {
            report(Diagnostic::Level::Error, "Consecutive statements on a line are not allowed!");
            return nullptr;
        }

        line = token.getLine();

        auto statement = parseStatement();
        if (!statement) {
            return nullptr;
        }

        statements.push_back(statement);
    }

    auto body = new (getContext()) BlockStatement(statements);

    for (auto child : body->getChildren()) {
        if (child->isNamedDeclaration()) {
            auto namedChild = reinterpret_cast<NamedDeclaration*>(child);
            if (!body->getScope()->insertDeclaration(namedChild)) {
                diagnostic->report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", namedChild->getName());
            }
        }
    }

    return new (getContext()) ConditionalCaseStatement(condition, body);
}

/// grammar: let-declaration := "let" identifier ":" type-identifier [ "=" expression ]
ConstantDeclaration* Parser::parseConstant() {
    if (!consumeToken(Token::Kind::KeywordLet)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::Colon)) {
        return nullptr;
    }

    auto typeRef = parseTypeRef();
    if (!typeRef) {
        return nullptr;
    }

    Expression* initializer = nullptr;
    if (tryConsumeOperator(Operator::Assign)) {
        initializer = parseExpression();
        if (!initializer) {
            return nullptr;
        }
    }

    return new (getContext()) ConstantDeclaration(name, typeRef, initializer);
}

/// grammar: continue-statement := "continue"
ContinueStatement* Parser::parseContinue() {
    if (!consumeToken(Token::Kind::KeywordContinue)) {
        return nullptr;
    }

    return new (getContext()) ContinueStatement;
}

/// grammar: defer-statement := "defer" expression
DeferStatement* Parser::parseDefer() {
    if (!consumeToken(Token::Kind::KeywordDefer)) {
        return nullptr;
    }

    auto expression = parseExpression();
    if (!expression) {
        return nullptr;
    }

    return new (getContext()) DeferStatement(expression);
}

/// grammar: do-statement := "do" block "while" expression
DoStatement* Parser::parseDoStatement() {
    if (!consumeToken(Token::Kind::KeywordDo)) {
        return nullptr;
    }

    auto body = parseBlock();
    if (!body) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::KeywordWhile)) {
        return nullptr;
    }

    auto condition = parseConditionList();
    if (!condition) {
        return nullptr;
    }

    return new (getContext()) DoStatement(condition, body);
}

/// grammar: else-case-statement := "else" ":" statement { line-break statement }
ElseCaseStatement* Parser::parseElseCaseStatement() {
    if (!consumeToken(Token::Kind::KeywordElse)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::Colon)) {
        return nullptr;
    }

    jelly::Array<Statement*> statements;
    unsigned line = token.getLine();
    while (!token.is(Token::Kind::KeywordCase, Token::Kind::KeywordElse, Token::Kind::RightBrace)) {
        if (!statements.empty() && line == token.getLine()) {
            report(Diagnostic::Level::Error, "Consecutive statements on a line are not allowed!");
            return nullptr;
        }

        line = token.getLine();

        auto statement = parseStatement();
        if (!statement) {
            return nullptr;
        }

        statements.push_back(statement);
    }

    auto body = new (getContext()) BlockStatement(statements);

    for (auto child : body->getChildren()) {
        if (child->isNamedDeclaration()) {
            auto namedChild = reinterpret_cast<NamedDeclaration*>(child);
            if (!body->getScope()->insertDeclaration(namedChild)) {
                diagnostic->report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", namedChild->getName());
            }
        }
    }

    return new (getContext()) ElseCaseStatement(body);
}

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
EnumerationDeclaration* Parser::parseEnumeration() {
    if (!consumeToken(Token::Kind::KeywordEnum)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::LeftBrace)) {
        return nullptr;
    }

    jelly::Array<EnumerationElementDeclaration*> children;
    if (!token.is(Token::Kind::RightBrace)) {
        unsigned line = token.getLine();
        while (true) {
            if (!children.empty() && line == token.getLine()) {
                report(Diagnostic::Level::Error, "Consecutive enum elements on a line are not allowed!");
                return nullptr;
            }

            auto element = parseEnumerationElement();
            if (!element) {
                return nullptr;
            }

            children.push_back(element);

            if (token.is(Token::Kind::RightBrace)) {
                break;
            }

            if (!token.is(Token::Kind::KeywordCase)) {
                report(Diagnostic::Level::Error, "Expected {0} at end of enum declaration!", Token::Kind::RightBrace);
                return nullptr;
            }
        }
    }
    consumeToken();

    auto declaration = new (getContext()) EnumerationDeclaration(name, children);

    for (auto child : declaration->getChildren()) {
        if (!declaration->getScope()->insertDeclaration(child)) {
            diagnostic->report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", child->getName());
        }
    }

    return declaration;
}

/// grammar: enum-element := "case" identifier [ "=" expression ]
EnumerationElementDeclaration* Parser::parseEnumerationElement() {
    if (!consumeToken(Token::Kind::KeywordCase)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    Expression* value = nullptr;
    if (tryConsumeOperator(Operator::Assign)) {
        value = parseExpression();
        if (!value) {
            return nullptr;
        }
    }

    return new (getContext()) EnumerationElementDeclaration(name, value);
}

/// grammar: expression := binary-expression | unary-expression | atom-expression
/// grammar: binary-expression := ( atom-expression | unary-expression | call-expression | subscript-expression ) infix-operator expression
Expression* Parser::parseExpression(Precedence precedence) {
    auto left = parsePrimaryExpression();
    if (!left) {
        return nullptr;
    }

    if (!getContext()->getOperator(token.getText(), Fixity::Infix, op) &&
        !getContext()->getOperator(token.getText(), Fixity::Postfix, op)) {
        return left;
    }

    while (precedence < op.getPrecedence()) {
        if (op.getFixity() == Fixity::Infix) {
            consumeToken();

            auto nextPrecedence = op.getPrecedence();
            if (op.getAssociativity() == Associativity::Right) {
                nextPrecedence = getContext()->getOperatorPrecedenceBefore(nextPrecedence);
            }

            auto right = parseExpression(nextPrecedence);
            if (!right) {
                return nullptr;
            }

            left = new (getContext()) BinaryExpression(op, left, right);
        } else if (op.getSymbol().equals(".")) {
            auto expr = tryParseMemberAccessExpression(left);
            if (expr) {
                left = expr;
            }
        }
        // @Bug postfix expressions should always be parsed as primary expressions without precedence!
        else if (op == Operator::Call) {
            left = parseCallExpression(left);
        } else {
            return nullptr;
        }

        if (!getContext()->getOperator(token.getText(), Fixity::Infix, op) &&
            !getContext()->getOperator(token.getText(), Fixity::Postfix, op)) {
            return left;
        }
    }

    return left;
}

/// grammar: fallthrough-statement := "fallthrough"
FallthroughStatement* Parser::parseFallthrough() {
    if (!consumeToken(Token::Kind::KeywordFallthrough)) {
        return nullptr;
    }

    return new (getContext()) FallthroughStatement;
}

/// grammar: float-literal := @Todo describe float literal grammar
FloatLiteral* Parser::parseFloatLiteral() {
    if (!token.is(Token::Kind::LiteralFloat)) {
        report(Diagnostic::Level::Error, "Expected Float Literal found {0}", token);
        return nullptr;
    }

    double value = 0;
    if (token.getText().getAsDouble(value)) {
        report(Diagnostic::Level::Error, "Invalid Float Literal found {0}", token);
        return nullptr;
    }

    consumeToken();

    return new (getContext()) FloatLiteral(value);
}

/// grammar: func-declaration := "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier block
FunctionDeclaration* Parser::parseFunction() {
    if (!consumeToken(Token::Kind::KeywordFunc)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::LeftParenthesis)) {
        return nullptr;
    }

    jelly::Array<ParameterDeclaration*> parameters;
    if (!token.is(Token::Kind::RightParenthesis)) {
        while (true) {
            auto parameter = parseParameter();
            if (!parameter) {
                return nullptr;
            }

            parameters.push_back(parameter);

            if (token.is(Token::Kind::RightParenthesis)) {
                break;
            }

            if (!consumeToken(Token::Kind::Comma)) {
                return nullptr;
            }
        }
    }
    consumeToken();

    if (!consumeToken(Token::Kind::Arrow)) {
        return nullptr;
    }

    auto returnTypeRef = parseTypeRef();
    if (!returnTypeRef) {
        return nullptr;
    }

    auto body = parseBlock();
    if (!body) {
        return nullptr;
    }

    auto declaration = new (getContext()) FunctionDeclaration(name, parameters, returnTypeRef, body);

    for (auto parameter : declaration->getParameters()) {
        if (!declaration->getScope()->insertDeclaration(parameter)) {
            diagnostic->report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", parameter->getName());
        }
    }

    return declaration;
}

/// grammar: group-expression := "(" expression ")"
Expression* Parser::parseGroupExpression() {
    if (!consumeToken(Token::Kind::LeftParenthesis)) {
        return nullptr;
    }

    auto expression = parseExpression();
    if (!expression) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::RightParenthesis)) {
        return nullptr;
    }

    return expression;
}

/// grammar: guard-statement := "guard" expression { "," expression } else block
GuardStatement* Parser::parseGuard() {
    if (!consumeToken(Token::Kind::KeywordGuard)) {
        return nullptr;
    }

    auto condition = parseConditionList();
    if (!condition) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::KeywordElse)) {
        return nullptr;
    }

    auto elseBlock = parseBlock();
    if (!elseBlock) {
        return nullptr;
    }

    return new (getContext()) GuardStatement(condition, elseBlock);
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
IdentifierExpression* Parser::parseIdentifierExpression() {
    Identifier identifier;
    if (!consumeIdentifier(identifier)) {
        return nullptr;
    }

    return new (getContext()) IdentifierExpression(identifier);
}

/// grammar: if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]
IfStatement* Parser::parseIf() {
    if (!consumeToken(Token::Kind::KeywordIf)) {
        return nullptr;
    }

    auto condition = parseConditionList();
    if (!condition) {
        return nullptr;
    }

    auto thenBlock = parseBlock();
    if (!thenBlock) {
        return nullptr;
    }

    BlockStatement* elseBlock = nullptr;
    if (token.is(Token::Kind::KeywordElse)) {
        consumeToken();

        if (token.is(Token::Kind::KeywordIf)) {
            auto ifStatement = parseIf();
            if (!ifStatement) {
                return nullptr;
            }

            elseBlock = new (getContext()) BlockStatement(jelly::ArrayRef<Statement*>({ ifStatement }));
        } else {
            elseBlock = parseBlock();
            if (!elseBlock) {
                return nullptr;
            }
        }
    }

    return new (getContext()) IfStatement(condition, thenBlock, elseBlock);
}

/// grammar: int-literal := @Todo describe float literal grammar
IntLiteral* Parser::parseIntLiteral() {
    if (!token.is(Token::Kind::LiteralInt)) {
        report(Diagnostic::Level::Error, "Expected Integer Literal found {0}", token);
        return nullptr;
    }

    uint64_t value = 0;
    if (token.getText().getAsInteger(0, value)) {
        report(Diagnostic::Level::Error, "Invalid Integer Literal!");
        return nullptr;
    }

    consumeToken();

    return new (getContext()) IntLiteral(value);
}

/// grammar: load-directive := "#load" string-literal
LoadDeclaration* Parser::parseLoadDeclaration() {
    if (!consumeToken(Token::Kind::KeywordLoad)) {
        return nullptr;
    }

    if (!token.is(Token::Kind::LiteralString)) {
        report(Diagnostic::Level::Error, "Expected String Literal after load directive!");
        return nullptr;
    }

    assert(token.getText().size() >= 2 && "Invalid length of string literal text, has to contain at least \"\"");

    auto sourceFilePath = getContext()->getIdentifier(token.getText().drop_front(1).drop_back(1)); // Makes a copy of the token text.
    consumeToken();

    return new (getContext()) LoadDeclaration(sourceFilePath);
}

/// grammar: nil-literal := "nil"
NilLiteral* Parser::parseNilLiteral() {
    if (!consumeToken(Token::Kind::KeywordNil)) {
        return nullptr;
    }

    return new (getContext()) NilLiteral();
}

/// grammar: opaque-type-ref := identifier
OpaqueTypeRef* Parser::parseOpaqueTypeRef() {
    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    return new (getContext()) OpaqueTypeRef(name);
}

/// grammar: parameter := identifier ":" type-identifier
ParameterDeclaration* Parser::parseParameter() {
    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::Colon)) {
        return nullptr;
    }

    auto typeRef = parseTypeRef();
    if (!typeRef) {
        return nullptr;
    }

    return new (getContext()) ParameterDeclaration(name, typeRef);
}

/// grammar: pointer-type-ref := type-ref "*"
PointerTypeRef* Parser::parsePointerTypeRef(TypeRef* pointeeTypeRef) {
    uint32_t depth = 0;

    while (tryConsumeOperator(Operator::TypePointer)) {
        depth += 1;
    }

    if (depth < 1) {
        report(Diagnostic::Level::Error, "Expected Operator = '*' found {0}", token);
        return nullptr;
    }

    return new (getContext()) PointerTypeRef(pointeeTypeRef, depth);
}

/// grammar: return-statement := "return" [ expression ]
ReturnStatement* Parser::parseReturn() {
    if (!consumeToken(Token::Kind::KeywordReturn)) {
        return nullptr;
    }

    auto value = tryParseExpression();
    return new (getContext()) ReturnStatement(value);
}


/// grammar: statement := variable-declaration | control-statement | defer-statement | do-statement | for-statement | guard-statement | if-statement | switch-statement | while-statement | expression
Statement* Parser::parseStatement() {
    switch (token.getKind()) {
        case Token::Kind::KeywordEnum:        return parseEnumeration();
        case Token::Kind::KeywordFunc:        return parseFunction();
        case Token::Kind::KeywordStruct:      return parseStructure();
        case Token::Kind::KeywordVar:         return parseVariable();
        case Token::Kind::KeywordLet:         return parseConstant();
        case Token::Kind::KeywordBreak:       return parseBreak();
        case Token::Kind::KeywordContinue:    return parseContinue();
        case Token::Kind::KeywordFallthrough: return parseFallthrough();
        case Token::Kind::KeywordReturn:      return parseReturn();
        case Token::Kind::KeywordDefer:       return parseDefer();
        case Token::Kind::KeywordDo:          return parseDoStatement();
        case Token::Kind::KeywordGuard:       return parseGuard();
        case Token::Kind::KeywordIf:          return parseIf();
        case Token::Kind::KeywordSwitch:      return parseSwitchStatement();
        case Token::Kind::KeywordWhile:       return parseWhileStatement();

        default: {
            auto expression = tryParseExpression();
            if (!expression) {
                report(Diagnostic::Level::Error, "Expected Statement found {0}", token);
                return nullptr;
            }
            return expression;
        }
    }
}

/// grammar: string-literal := @Todo describe float literal grammar
StringLiteral* Parser::parseStringLiteral() {
    if (!token.is(Token::Kind::LiteralString)) {
        report(Diagnostic::Level::Error, "Expected String Literal found {0}", token);
        return nullptr;
    }

    assert(token.getText().size() >= 2 && "Invalid length of string literal text, has to contain at least \"\"");

    // @Cleanup we form an identifier here to retain memory for value of StringLiteral
    auto value = getContext()->getIdentifier(token.getText().drop_front(1).drop_back(1));

    consumeToken();

    return new (getContext()) StringLiteral(value);
}

/// grammar: struct-declaration := "struct" identifier "{" { value-declaration } "}"
StructureDeclaration* Parser::parseStructure() {
    if (!consumeToken(Token::Kind::KeywordStruct)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::LeftBrace)) {
        return nullptr;
    }

    jelly::Array<ValueDeclaration*> values;
    if (!token.is(Token::Kind::RightBrace)) {
        unsigned line = token.getLine();
        while (true) {
            if (!values.empty() && line == token.getLine()) {
                report(Diagnostic::Level::Error, "Consecutive statements on a line are not allowed!");
                return nullptr;
            }
            line = token.getLine();

            auto value = parseValueDeclaration();
            if (!value) {
                return nullptr;
            }

            values.push_back(value);

            if (token.is(Token::Kind::RightBrace)) {
                break;
            }
        }
    }
    consumeToken();

    auto declaration = new (getContext()) StructureDeclaration(name, values);

    for (auto child : declaration->getChildren()) {
        if (!declaration->getScope()->insertDeclaration(child)) {
            report(Diagnostic::Level::Error, "Invalid redeclaration of '{0}'", child->getName());
        }
    }

    return declaration;
}

/// grammar: switch-statement := "switch" expression "{" [ switch-case { line-break switch-case } ] "}"
SwitchStatement* Parser::parseSwitchStatement() {
    if (!consumeToken(Token::Kind::KeywordSwitch)) {
        return nullptr;
    }

    auto argument = parseExpression();
    if (!argument) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::LeftBrace)) {
        return nullptr;
    }

    jelly::Array<CaseStatement*> cases;
    unsigned line = token.getLine();
    while (!token.is(Token::Kind::RightBrace)) {
        if (!cases.empty() && line == token.getLine()) {
            report(Diagnostic::Level::Error, "Consecutive statements on a line are not allowed!");
            return nullptr;
        }

        line = token.getLine();

        auto statement = parseCaseStatement();
        if (!statement) {
            return nullptr;
        }

        cases.push_back(statement);
    }
    consumeToken();

    return new (getContext()) SwitchStatement(argument, cases);
}

/// grammar: type-of-type-ref := "typeof" "(" expression ")"
TypeOfTypeRef* Parser::parseTypeOfTypeRef() {
    if (!consumeToken(Token::Kind::KeywordTypeof)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::LeftParenthesis)) {
        return nullptr;
    }

    auto expression = parseExpression();

    if (!consumeToken(Token::Kind::RightParenthesis)) {
        return nullptr;
    }

    return new (getContext()) TypeOfTypeRef(expression);
}

/// grammar: type-ref := opaque-type-ref | pointer-type-ref | array-type-ref | type-of-type-ref
TypeRef* Parser::parseTypeRef() {
    TypeRef* typeRef = nullptr;

    switch (token.getKind()) {
        case Token::Kind::Identifier:
            typeRef = parseOpaqueTypeRef();
            break;

        case Token::Kind::KeywordTypeof:
            typeRef = parseTypeOfTypeRef();
            break;

        default:
            report(Diagnostic::Level::Error, "Expected TypeRef found {0}", token);
            return nullptr;
    }

    if (!typeRef) {
        return nullptr;
    }

    do {
        switch (token.getKind()) {
            case Token::Kind::Operator: {
                if (getContext()->getOperator(token.getText(), Operator::TypePointer.getFixity(), this->op) && this->op == Operator::TypePointer) {
                    auto pointerTypeRef = parsePointerTypeRef(typeRef);
                    if (!pointerTypeRef) {
                        return nullptr;
                    }

                    typeRef = pointerTypeRef;
                } else {
                    return typeRef;
                }
            } break;

            case Token::Kind::LeftBracket: {
                auto arrayTypeRef = parseArrayTypeRef(typeRef);
                if (!arrayTypeRef) {
                    return nullptr;
                }

                typeRef = arrayTypeRef;
            } break;

            default:
                return typeRef;
        }
    } while (true);

    return typeRef;
}

/// grammar: unary-expression := prefix-operator expression
UnaryExpression* Parser::parseUnaryExpression() {
    if (!consumeOperator(Fixity::Prefix, op)) {
        return nullptr;
    }

    auto right = parsePrimaryExpression();
    if (!right) {
        return nullptr;
    }

    return new (getContext()) UnaryExpression(op, right);
}

/// grammar: value-declaration := var-declaration | let-declaration
ValueDeclaration* Parser::parseValueDeclaration() {
    switch (token.getKind()) {
        case Token::Kind::KeywordLet: return parseConstant();
        case Token::Kind::KeywordVar: return parseVariable();

        default:
            report(Diagnostic::Level::Error, "Expected 'var' or 'let' found {0}", token);
            return nullptr;
    }
}

/// grammar: var-declaration := "var" identifier ":" type-identifier [ "=" expression ]
VariableDeclaration* Parser::parseVariable() {
    if (!consumeToken(Token::Kind::KeywordVar)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::Colon)) {
        return nullptr;
    }

    auto typeRef = parseTypeRef();
    if (!typeRef) {
        return nullptr;
    }

    Expression* initializer = nullptr;
    if (tryConsumeOperator(Operator::Assign)) {
        initializer = parseExpression();
        if (!initializer) {
            return nullptr;
        }
    }

    return new (getContext()) VariableDeclaration(name, typeRef, initializer);
}

/// grammar: while-statement := "while" expression { "," expression } block
WhileStatement* Parser::parseWhileStatement() {
    if (!consumeToken(Token::Kind::KeywordWhile)) {
        return nullptr;
    }

    auto condition = parseConditionList();
    if (!condition) {
        return nullptr;
    }

    auto body = parseBlock();
    if (!body) {
        return nullptr;
    }

    return new (getContext()) WhileStatement(condition, body);
}

jelly::AST::Context* Parser::getContext() const {
    return lexer->getContext();
}
