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

// @Incomplete Check if symbols of unary expressions are right bound ! (unexpected: ~ value, expected: ~value)

static bool isDeclarationNameEqual(NamedDeclaration* lhs, NamedDeclaration* rhs) {
    return lhs->getName() == rhs->getName();
}

Parser::Parser(Lexer* lexer, Context* context, DiagnosticEngine* diag) :
lexer(lexer), context(context), diag(diag), op(Operator::LogicalNot) {
}

void Parser::parseAllTopLevelNodes() {
    token = lexer->peekNextToken();

    auto module = context->getModule();

    bool checkConsecutiveTopLevelNodes = false;
    unsigned line = token.getLine();
    do {
        unsigned nodeLine = token.getLine();
        auto declaration = parseTopLevelDeclaration();
        if (!declaration) {
            break;
        }

        if (checkConsecutiveTopLevelNodes && line == nodeLine) {
            report(DIAG_ERROR, "Consecutive top level nodes on a line are not allowed!");
            break;
        }
        checkConsecutiveTopLevelNodes = true;
        line = nodeLine;

        if (declaration->isNamedDeclaration()) {
            auto namedDeclaration = reinterpret_cast<NamedDeclaration*>(declaration);
            if (module->lookupDeclaration(namedDeclaration->getName())) {
                report(DIAG_ERROR, "Invalid redeclaration of '{0}'", namedDeclaration->getName());
            } else {
                module->addDeclaration(declaration);
            }
        } else {
            module->addDeclaration(declaration);
        }

    } while (true);
}

void Parser::consumeToken() {
    lexer->lexToken();
    token = lexer->peekNextToken();
}

bool Parser::consumeToken(Token::Kind kind) {
    if (token.is(Token::Kind::Colon)) {
        consumeToken();
        return true;
    }

    // @Todo convert token kind and token to string description!
    report(DIAG_ERROR, "Expected token '{0}' found '{1}'!", (uint16_t)kind, token.getText());
    return false;
}

bool Parser::consumeIdentifier(Identifier& identifier) {
    if (token.is(Token::Kind::Identifier)) {
        identifier = context->getIdentifier(token.getText());
        consumeToken();
        return true;
    }

    // @Todo convert token kind and token to string description!
    report(DIAG_ERROR, "Expected token 'Identifier' found '{0}'!", token.getText());
    return false;
}

bool Parser::consumeOperator(Fixity fixity, Operator& op) {
    if (token.is(Token::Kind::Operator) && context->getOperator(token.getText(), fixity, op)) {
        consumeToken();
        return true;
    }

    // @Todo convert token kind and token to string description!
    report(DIAG_ERROR, "Expected operator token found '{0}'!", token.getText());
    return false;
}

bool Parser::tryConsumeOperator(Operator op) {
    if (token.is(Token::Kind::Operator) && context->getOperator(op.getSymbol(), op.getFixity(), this->op) && this->op == op) {
        consumeToken();
        return true;
    }

    return false;
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

    return new (context) MemberAccessExpression(left, memberName);
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

        condition = new (context) BinaryExpression(Operator::LogicalAnd, condition, expression);
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
            report(DIAG_ERROR, "Expected expression, found '{0}'", token.getText());
            return nullptr;
    }
}

/// grammar: primary-expression := unary-expression | atom-expression
Expression* Parser::parsePrimaryExpression() {
    if (token.is(Token::Kind::Operator)) {
        if (context->getOperator(token.getText(), Fixity::Prefix, op)) {
            return parseUnaryExpression();
        } else {
            report(DIAG_ERROR, "Expected prefix operator found '{0}'", token.getText());
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
            report(DIAG_ERROR, "Unexpected token found expected top level declaration!");
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

    return  new (context) ArrayTypeRef(elementTypeRef, value);
}

/// grammar: block := '{' { statement } '}'
BlockStatement* Parser::parseBlock() {
    if (!consumeToken(Token::Kind::LeftBrace)) {
        return nullptr;
    }

    jelly::Array<Statement*> statements;
    if (!token.is(Token::Kind::RightBrace)) {
        unsigned line = token.getLine();
        while (true) {
            if (!statements.empty() && line == token.getLine()) {
                report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
                return nullptr;
            }
            line = token.getLine();

            auto statement = parseStatement();
            if (!statement) {
                return nullptr;
            }

            statements.push_back(statement);

            if (token.is(Token::Kind::RightBrace)) {
                break;
            }
        }
    }
    consumeToken();

    return new (context) BlockStatement(statements);
}

/// grammar: bool-literal := "true" | "false"
BoolLiteral* Parser::parseBoolLiteral() {
    if (!token.is(Token::Kind::KeywordTrue, Token::Kind::KeywordFalse)) {
        report(DIAG_ERROR, "Expected keyword 'true' or 'false' found '{0}'", token.getText());
        return nullptr;
    }

    bool value = token.is(Token::Kind::KeywordTrue);
    consumeToken();

    return new (context) BoolLiteral(value);
}

/// grammar: break-statement := "break"
BreakStatement* Parser::parseBreak() {
    if (!consumeToken(Token::Kind::KeywordBreak)) {
        return nullptr;
    }

    return new (context) BreakStatement;
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

    return new (context) CallExpression(callee, arguments);
}

/// grammar: case-statement := conditional-case-statement | else-case-statement
CaseStatement* Parser::parseCaseStatement() {
    switch (token.getKind()) {
        case Token::Kind::KeywordCase: return parseConditionalCaseStatement();
        case Token::Kind::KeywordElse: return parseElseCaseStatement();

        default:
            report(DIAG_ERROR, "Expected 'case' or 'else' keyword, found '{0}'", token.getText());
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
            report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
            return nullptr;
        }

        line = token.getLine();

        auto statement = parseStatement();
        if (!statement) {
            return nullptr;
        }

        statements.push_back(statement);
    }

    auto body = new (context) BlockStatement(statements);

    return new (context) ConditionalCaseStatement(condition, body);
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

    return new (context) ConstantDeclaration(name, typeRef, initializer);
}

/// grammar: continue-statement := "continue"
ContinueStatement* Parser::parseContinue() {
    if (!consumeToken(Token::Kind::KeywordContinue)) {
        return nullptr;
    }

    return new (context) ContinueStatement;
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

    return new (context) DeferStatement(expression);
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

    return new (context) DoStatement(condition, body);
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
            report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
            return nullptr;
        }

        line = token.getLine();

        auto statement = parseStatement();
        if (!statement) {
            return nullptr;
        }

        statements.push_back(statement);
    }

    auto body = new (context) BlockStatement(statements);

    return new (context) ElseCaseStatement(body);
}

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
EnumerationDeclaration* Parser::parseEnumeration() {
    if (!consumeToken(Token::Kind::KeywordElse)) {
        return nullptr;
    }

    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    if (!consumeToken(Token::Kind::LeftBrace)) {
        return nullptr;
    }

    jelly::Array<EnumerationElementDeclaration*> elements;
    if (!token.is(Token::Kind::RightBrace)) {
        unsigned line = token.getLine();
        while (true) {
            if (!elements.empty() && line == token.getLine()) {
                report(DIAG_ERROR, "Consecutive enum elements on a line are not allowed!");
                return nullptr;
            }

            auto element = parseEnumerationElement();
            if (!element) {
                return nullptr;
            }

            if (elements.contains(element, isDeclarationNameEqual)) {
                report(DIAG_ERROR, "Invalid redeclaration of '{0}'", element->getName());
            }

            elements.push_back(element);

            if (token.is(Token::Kind::RightBrace)) {
                break;
            }

            if (!token.is(Token::Kind::KeywordCase)) {
                report(DIAG_ERROR, "Expected '}' at end of enum declaration!");
                return nullptr;
            }
        }
    }
    consumeToken();

    return new (context) EnumerationDeclaration(name, elements);
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

    return new (context) EnumerationElementDeclaration(name, value);
}

/// grammar: expression := binary-expression | unary-expression | atom-expression
/// grammar: binary-expression := ( atom-expression | unary-expression | call-expression | subscript-expression ) infix-operator expression
Expression* Parser::parseExpression(Precedence precedence) {
    auto left = parsePrimaryExpression();
    if (!left) {
        return nullptr;
    }

    if (!context->getOperator(token.getText(), Fixity::Infix, op) &&
        !context->getOperator(token.getText(), Fixity::Postfix, op)) {
        return left;
    }

    while (precedence < op.getPrecedence()) {
        if (op.getFixity() == Fixity::Infix) {
            consumeToken();

            auto nextPrecedence = op.getPrecedence();
            if (op.getAssociativity() == Associativity::Right) {
                nextPrecedence = context->getOperatorPrecedenceBefore(nextPrecedence);
            }

            auto right = parseExpression(nextPrecedence);
            if (!right) {
                return nullptr;
            }

            left = new (context) BinaryExpression(op, left, right);
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

        if (!context->getOperator(token.getText(), Fixity::Infix, op) &&
            !context->getOperator(token.getText(), Fixity::Postfix, op)) {
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

    return new (context) FallthroughStatement;
}

/// grammar: float-literal := @Todo describe float literal grammar
FloatLiteral* Parser::parseFloatLiteral() {
    if (!token.is(Token::Kind::LiteralFloat)) {
        report(DIAG_ERROR, "Expected float literal found '{0}'", token.getText());
        return nullptr;
    }

    double value = 0;
    if (!token.getText().getAsDouble(value)) {
        report(DIAG_ERROR, "Invalid floating point literal!");
        return nullptr;
    }

    consumeToken();

    return new (context) FloatLiteral(value);
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

            if (parameters.contains(parameter, isDeclarationNameEqual)) {
                report(DIAG_ERROR, "Invalid redeclaration of '{0}'", parameter->getName());
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

//    for (auto stmt : Func->body->stmts) {
//        if (stmt->isDecl()) {
//            auto decl = reinterpret_cast<ASTDecl*>(stmt);
//            if (decl->isNamedDecl()) {
//                auto namedDecl = reinterpret_cast<ASTNamedDecl*>(decl);
//                if (!Func->lookupDecl(namedDecl->name)) {
//                    Func->addDecl(namedDecl);
//                } else {
//                    Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", namedDecl->name);
//                }
//            } else {
//                Func->addDecl(decl);
//            }
//        }
//    }

    return new (context) FunctionDeclaration(name, parameters, returnTypeRef, body);
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

    return new (context) GuardStatement(condition, elseBlock);
}

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
IdentifierExpression* Parser::parseIdentifierExpression() {
    Identifier identifier;
    if (!consumeIdentifier(identifier)) {
        return nullptr;
    }

    return new (context) IdentifierExpression(identifier);
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

            elseBlock = new (context) BlockStatement(jelly::ArrayRef<Statement*>({ ifStatement }));
        } else {
            elseBlock = parseBlock();
            if (!elseBlock) {
                return nullptr;
            }
        }
    }

    return new (context) IfStatement(condition, thenBlock, elseBlock);
}

/// grammar: int-literal := @Todo describe float literal grammar
IntLiteral* Parser::parseIntLiteral() {
    if (!token.is(Token::Kind::LiteralInt)) {
        report(DIAG_ERROR, "Expected integer literal found '{0}'", token.getText());
        return nullptr;
    }

    uint64_t value = 0;
    if (!token.getText().getAsInteger(0, value)) {
        report(DIAG_ERROR, "Invalid integer literal!");
        return nullptr;
    }

    consumeToken();

    return new (context) IntLiteral(value);
}

/// grammar: load-directive := "#load" string-literal
LoadDeclaration* Parser::parseLoadDeclaration() {
    if (!consumeToken(Token::Kind::KeywordLoad)) {
        return nullptr;
    }

    if (!token.is(Token::Kind::LiteralString)) {
        report(DIAG_ERROR, "Expected string literal after load directive!");
        return nullptr;
    }

    assert(token.getText().size() >= 2 && "Invalid length of string literal text, has to contain at least \"\"");

    auto sourceFilePath = context->getIdentifier(token.getText().drop_front(1).drop_back(1)); // Makes a copy of the token text.
    consumeToken();

    return new (context) LoadDeclaration(sourceFilePath);
}

/// grammar: nil-literal := "nil"
NilLiteral* Parser::parseNilLiteral() {
    if (!consumeToken(Token::Kind::KeywordNil)) {
        return nullptr;
    }

    return new (context) NilLiteral();
}

/// grammar: opaque-type-ref := identifier
OpaqueTypeRef* Parser::parseOpaqueTypeRef() {
    Identifier name;
    if (!consumeIdentifier(name)) {
        return nullptr;
    }

    return new (context) OpaqueTypeRef(name);
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

    return new (context) ParameterDeclaration(name, typeRef);
}

/// grammar: pointer-type-ref := type-ref "*"
PointerTypeRef* Parser::parsePointerTypeRef(TypeRef* pointeeTypeRef) {
    uint32_t depth = 0;

    while (tryConsumeOperator(Operator::TypePointer)) {
        depth += 1;
    }

    if (depth < 1) {
        report(DIAG_ERROR, "Expected '*' found '{0}'", token.getText());
        return nullptr;
    }

    return new (context) PointerTypeRef(pointeeTypeRef, depth);
}

/// grammar: return-statement := "return" [ expression ]
ReturnStatement* Parser::parseReturn() {
    if (!consumeToken(Token::Kind::KeywordReturn)) {
        return nullptr;
    }

    auto value = tryParseExpression();
    return new (context) ReturnStatement(value);
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
                report(DIAG_ERROR, "Expected statement, found '{0}'", token.getText());
                return nullptr;
            }
            return expression;
        }
    }
}

/// grammar: string-literal := @Todo describe float literal grammar
StringLiteral* Parser::parseStringLiteral() {
    if (!token.is(Token::Kind::LiteralString)) {
        report(DIAG_ERROR, "Expected string literal found '{0}'", token.getText());
        return nullptr;
    }

    assert(token.getText().size() >= 2 && "Invalid length of string literal text, has to contain at least \"\"");

    // @Cleanup we form an identifier here to retain memory for value of StringLiteral
    auto value = context->getIdentifier(token.getText().drop_front(1).drop_back(1));

    consumeToken();

    return new (context) StringLiteral(value);
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
                report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
                return nullptr;
            }
            line = token.getLine();

            auto value = parseValueDeclaration();
            if (!value) {
                return nullptr;
            }

            if (values.contains(value, isDeclarationNameEqual)) {
                report(DIAG_ERROR, "Invalid redeclaration of '{0}'", value->getName());
            }

            values.push_back(value);

            if (token.is(Token::Kind::RightBrace)) {
                break;
            }
        }
    }
    consumeToken();

    return new (context) StructureDeclaration(name, values);
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
            report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
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

    return new (context) SwitchStatement(argument, cases);
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

    return new (context) TypeOfTypeRef(expression);
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
            report(DIAG_ERROR, "Expected type ref found '{0}'", token.getText());
            return nullptr;
    }

    if (!typeRef) {
        return nullptr;
    }

    do {
        switch (token.getKind()) {
            case Token::Kind::Operator: {
                auto pointerTypeRef = parsePointerTypeRef(typeRef);
                if (!pointerTypeRef) {
                    return nullptr;
                }

                typeRef = pointerTypeRef;
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

    return new (context) UnaryExpression(op, right);
}

/// grammar: value-declaration := var-declaration | let-declaration
ValueDeclaration* Parser::parseValueDeclaration() {
    switch (token.getKind()) {
        case Token::Kind::KeywordLet: return parseConstant();
        case Token::Kind::KeywordVar: return parseVariable();

        default:
            report(DIAG_ERROR, "Expected 'var' or 'let' at start of value-declaration!");
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

    return new (context) VariableDeclaration(name, typeRef, initializer);
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

    return new (context) WhileStatement(condition, body);
}
