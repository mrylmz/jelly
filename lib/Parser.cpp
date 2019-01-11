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

#include "Core/Defer.h"
#include "Core/Parser.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/Support/ErrorHandling.h>

#define PushParent(__Parser__, __Parent__) \
__Parent__->parent = __Parser__->parent; \
__Parser__->parent = __Parent__

#define PopParent(__Parser__) \
assert(__Parser__->parent); \
__Parser__->parent = __Parser__->parent->parent

#define PushDeclContext(__Parser__, __Context__)      \
__Context__->setDeclContext(__Parser__->declContext); \
__Parser__->declContext = __Context__

#define PopDeclContext(__Parser__) \
assert(__Parser__->declContext); \
__Parser__->declContext = __Parser__->declContext->getDeclContext();

// @Incomplete Check if symbols of unary expressions are right bound ! (unexpected: ~ value, expected: ~value)
// @Incomplete Write unit-tests for assignment of parents

// @Refactor Make all static global scope functions member functions of Parser!
Parser::Parser(Lexer* lexer, ASTContext* context, DiagnosticEngine* diag) :
lexer(lexer), context(context), diag(diag) {
}

void ParseAllTopLevelNodes(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag);
void Parser::parseAllTopLevelNodes() {
    return ParseAllTopLevelNodes(this, context, diag);
}

static void ConsumeToken(Parser* Parser) {
    Parser->token = Parser->lexer->lexToken();
    Parser->token = Parser->lexer->peekNextToken();
}

// MARK: - Literals

/// grammar: literal := integer-literal | float-literal | string-literal | bool-literal | nil-literal

/// grammar: nil-literal := "nil"
static ASTNilLit* ParseNilLiteral(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_NIL));

    ASTNilLit* Nil = new (Context) ASTNilLit;
    Nil->declContext = Parser->declContext;
    PushParent(Parser, Nil);
    PopParent(Parser);
    ConsumeToken(Parser);
    return Nil;
}

/// grammar: bool-literal := "true" | "false"
static ASTBoolLit* ParseBoolLiteral(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_TRUE, TOKEN_KEYWORD_FALSE));

    ASTBoolLit* Bool = new (Context) ASTBoolLit;
    Bool->declContext = Parser->declContext;
    PushParent(Parser, Bool);
    {
        if (Parser->token.kind == TOKEN_KEYWORD_TRUE) {
            Bool->value = true;
        } else {
            Bool->value = false;
        }
        ConsumeToken(Parser);
    }
    PopParent(Parser);

    return Bool;
}

/// grammar: int-literal :=  #todo
static ASTIntLit* ParseIntLiteral(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_LITERAL_INT));

    ASTIntLit* Int = new (Context) ASTIntLit;
    Int->declContext = Parser->declContext;
    PushParent(Parser, Int);
    {
        if (Parser->token.text.getAsInteger(0, Int->value)) {
            Parser->report(DIAG_ERROR, "Invalid integer literal!");
            return nullptr;
        }
        ConsumeToken(Parser);
    }
    PopParent(Parser);

    return Int;
}

/// grammar: float-literal := #todo
static ASTFloatLit* ParseFloatLiteral(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_LITERAL_FLOAT);

    ASTFloatLit* Float = new (Context) ASTFloatLit;
    Float->declContext = Parser->declContext;
    PushParent(Parser, Float);
    {
        if (Parser->token.text.getAsDouble(Float->value)) {
            Parser->report(DIAG_ERROR, "Invalid floating point literal!");
            return nullptr;
        }
        ConsumeToken(Parser);
    }
    PopParent(Parser);

    return Float;
}

/// grammar: string-literal := #todo
static ASTStringLit* ParseStringLiteral(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (Parser->token.kind != TOKEN_LITERAL_STRING) { return nullptr; }

    assert(Parser->token.text.size() >= 2 && "Invalid length of string literal text, has to contain at least \"\"");

    ASTStringLit* String = new (Context) ASTStringLit;
    String->declContext = Parser->declContext;
    PushParent(Parser, String);
    {
        // @Cleanup we form a lexeme here to retain memory for String->Value
        String->value = Context->getLexeme(Parser->token.text.drop_front(1).drop_back(1));
        ConsumeToken(Parser);
    }
    PopParent(Parser);

    return String;
}

// MARK: - Expressions

static ASTExpr* ParseExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, Precedence Precedence = 0);
static ASTExpr* TryParseExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, Precedence Precedence = 0);

/// grammar: identifier := identifier-head { identifier-tail }
/// grammar: identifier-head := "a" ... "z" | "A" ... "Z" | "_"
/// grammar: identifier-tail := identifier-head | "0" ... "9"
static ASTIdentExpr* ParseIdent(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (Parser->token.kind != TOKEN_IDENTIFIER) { return nullptr; }

    ASTIdentExpr* Ident = new (Context) ASTIdentExpr;
    Ident->declContext = Parser->declContext;
    PushParent(Parser, Ident);
    {
        Ident->declName = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);
    }
    PopParent(Parser);

    return Ident;
}

/// grammar: group-expression := "(" expression ")"
static ASTExpr* ParseGroupExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is('(') && "Invalid token given for start of group expression!");

    ConsumeToken(Parser);

    ASTExpr* Expr = ParseExpr(Parser, Context, Diag);
    if (Expr == nullptr) {
        return nullptr;
    }

    if (!Parser->token.is(')')) {
        Parser->report(DIAG_ERROR, "Expected ')' at end of group expression!");
        return nullptr;
    }
    ConsumeToken(Parser);

    return Expr;
}

/// grammar: atom-expression := group-expression | literal-expression | identifier-expression
/// grammar: literal-expression := literal
/// grammar: identifier-expression := identifier
static ASTExpr* ParseAtomExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    switch (Parser->token.kind) {
        case '(':                  return ParseGroupExpr(Parser, Context, Diag);
        case TOKEN_KEYWORD_NIL:    return ParseNilLiteral(Parser, Context, Diag);
        case TOKEN_KEYWORD_TRUE:   return ParseBoolLiteral(Parser, Context, Diag);
        case TOKEN_KEYWORD_FALSE:  return ParseBoolLiteral(Parser, Context, Diag);
        case TOKEN_LITERAL_INT:    return ParseIntLiteral(Parser, Context, Diag);
        case TOKEN_LITERAL_FLOAT:  return ParseFloatLiteral(Parser, Context, Diag);
        case TOKEN_LITERAL_STRING: return ParseStringLiteral(Parser, Context, Diag);
        case TOKEN_IDENTIFIER:     return ParseIdent(Parser, Context, Diag);
        default:
            Parser->report(DIAG_ERROR, "Expected expression, found '{0}'", Parser->token.text);
            return nullptr;
    }
}

static ASTExpr* ParsePrimaryExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag);

/// grammar: unary-expression := prefix-operator expression
static ASTUnaryExpr* ParseUnaryExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_OPERATOR)
           && Parser->lexer->getOperator(Parser->token.text, OPERATOR_PREFIX, Parser->op) // TODO: Move this out of assert, won't get compiled in release builds!
           && "Invalid token given for start of unary-expression!");

    ConsumeToken(Parser);

    ASTUnaryExpr* Expr = new (Context) ASTUnaryExpr;
    Expr->declContext = Parser->declContext;
    PushParent(Parser, Expr);
    {
        Expr->op = Parser->op;
        Expr->right = ParsePrimaryExpr(Parser, Context, Diag);

        if (Expr->right == nullptr) {
            return nullptr;
        }
    }
    PopParent(Parser);

    return Expr;
}

/// grammar: primary-expression := unary-expression | atom-expression
static ASTExpr* ParsePrimaryExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (Parser->token.is(TOKEN_OPERATOR)) {
        if (Parser->lexer->getOperator(Parser->token.text, OPERATOR_PREFIX, Parser->op)) {
            return ParseUnaryExpr(Parser, Context, Diag);
        } else {
            Parser->report(DIAG_ERROR, "Unknown prefix operator!");
            return nullptr;
        }
    }

    return ParseAtomExpr(Parser, Context, Diag);
}

/// grammar: call-expression := expression "(" [ expression { "," expression } ] ")"
static ASTCallExpr* ParseCallExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, ASTExpr* Left) {
    assert(Parser->token.is('(') && "Invalid token given for start of call-expression");
    ConsumeToken(Parser);

    ASTCallExpr* callExpr = new (Context) ASTCallExpr;
    callExpr->left = Left; // @Bug is the parent of the left broken here ???
    callExpr->declContext = Parser->declContext;

    PushParent(Parser, callExpr);
    {
        llvm::SmallVector<ASTExpr*, 0> arguments;
        if (!Parser->token.is(')')) {
            while (true) {
                ASTExpr* argument = ParseExpr(Parser, Context, Diag);
                if (argument == nullptr) {
                    return nullptr;
                }

                arguments.push_back(argument);

                if (Parser->token.is(')')) {
                    break;
                } else if (!Parser->token.is(',')) {
                    Parser->report(DIAG_ERROR, "Expected ')' or ',' in argument list of call-expression!");
                    return nullptr;
                }

                ConsumeToken(Parser);
            }
        }
        ConsumeToken(Parser);

        callExpr->args = llvm::makeArrayRef(arguments).copy(Context->nodeAllocator);
    }
    PopParent(Parser);

    return callExpr;
}

/// grammar: subscript-expression := expression "[" [ expression { "," expression } ] "]"
static ASTSubscriptExpr* ParseSubscriptExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, ASTExpr* Left) {
    assert(Parser->token.is('[') && "Invalid token given for start of call-expression");
    ConsumeToken(Parser);

    ASTSubscriptExpr* Subscript = new (Context) ASTSubscriptExpr;
    Subscript->left = Left; // @Bug is the parent of the left broken here ???
    Subscript->declContext = Parser->declContext;

    PushParent(Parser, Subscript);
    {
        llvm::SmallVector<ASTExpr*, 0> arguments;
        if (!Parser->token.is(']')) {
            while (true) {
                ASTExpr* argument = ParseExpr(Parser, Context, Diag);
                if (argument == nullptr) {
                    return nullptr;
                }

                arguments.push_back(argument);

                if (Parser->token.is(']')) {
                    break;
                } else if (!Parser->token.is(',')) {
                    Parser->report(DIAG_ERROR, "Expected ']' or ',' in argument list of subscript-expression!");
                    return nullptr;
                }

                ConsumeToken(Parser);
            }
        }
        ConsumeToken(Parser);

        Subscript->args = llvm::makeArrayRef(arguments).copy(Context->nodeAllocator);
    }
    PopParent(Parser);

    return Subscript;
}

/// grammar: expression := binary-expression | unary-expression | atom-expression
/// grammar: binary-expression := ( atom-expression | unary-expression | call-expression | subscript-expression ) infix-operator expression
static ASTExpr* ParseExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, Precedence Prec) {
    ASTExpr* Left = ParsePrimaryExpr(Parser, Context, Diag);
    if (Left == nullptr) {
        return nullptr;
    }

    if (!Parser->lexer->getOperator(Parser->token.text, OPERATOR_INFIX, Parser->op)
        && !Parser->lexer->getOperator(Parser->token.text, OPERATOR_POSTFIX, Parser->op)) {
        return Left;
    }

    // @Incomplete maintain parent stack for binary expressions !!!
    while (Prec < Parser->op.precedence) {
        if (Parser->op.kind == OPERATOR_INFIX) {
            ConsumeToken(Parser);

            Precedence NextPrecedence = Parser->op.precedence;
            if (Parser->op.associativity == ASSOCIATIVITY_RIGHT) {
                NextPrecedence = Parser->lexer->getOperatorPrecedenceBefore(NextPrecedence);
            }

            ASTBinaryExpr* Right = new (Context) ASTBinaryExpr;
            Right->declContext = Parser->declContext;
            Right->parent = Parser->parent;
            Right->op = Parser->op;
            Right->left = Left;
            Left->parent = Right;

            Right->right = ParseExpr(Parser, Context, Diag, NextPrecedence);
            if (Right->right == nullptr) {
                return nullptr;
            }
            Right->right->parent = Right;

            Left = Right;
        } else if (Parser->op.text.equals(".")) {
            ConsumeToken(Parser);

            ASTMemberAccessExpr* Right = new (Context) ASTMemberAccessExpr;
            Right->declContext = Parser->declContext;
            PushParent(Parser, Right);
            PopParent(Parser);
            Right->left = Left;
            Left->parent = Right;

            if (!Parser->token.is(TOKEN_IDENTIFIER)) {
                Parser->report(DIAG_ERROR, "Expected identifier for member name!");
                return nullptr;
            }
            Right->memberName = Context->getLexeme(Parser->token.text);
            ConsumeToken(Parser);

            Left = Right;

        }
        // @Bug postfix expressions should always be parsed as primary expressions without precedence!
        else if (Parser->op.text.equals("()")) {
            Left = ParseCallExpr(Parser, Context, Diag, Left);
        } else if (Parser->op.text.equals("[]")) {
            Left = ParseSubscriptExpr(Parser, Context, Diag, Left);
        } else {
            return nullptr;
        }

        if (!Parser->lexer->getOperator(Parser->token.text, OPERATOR_INFIX, Parser->op)
            && !Parser->lexer->getOperator(Parser->token.text, OPERATOR_POSTFIX, Parser->op)) {
            break;
        }
    }

    return Left;
}

static ASTExpr* TryParseExpr(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, Precedence Precedence) {
    auto isSilent = Parser->silentErrors;
    Parser->silentErrors = true;
    defer(Parser->silentErrors = isSilent);

    auto lexerState = Parser->lexer->state;
    auto expr = ParseExpr(Parser, Context, Diag, Precedence);
    if (!expr) {
        Parser->lexer->state = lexerState;
    }

    return expr;
}

// MARK: - Directives

/// grammar: directive := load-directive

/// grammar: load-directive := "#load" string-literal
static ASTLoadDirective* ParseLoadDirective(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_LOAD) && "Invalid token given for start of load directive!");
    ConsumeToken(Parser);

    if (Parser->token.kind != TOKEN_LITERAL_STRING) {
        Parser->report(DIAG_ERROR, "Expected string literal after load directive!");
        return nullptr;
    }

    assert(Parser->token.text.size() >= 2 && "Invalid length of string literal text, has to contain at least \"\"");

    ASTLoadDirective* loadDirective = new (Context) ASTLoadDirective;
    loadDirective->declContext = Parser->declContext;
    loadDirective->parent = Parser->parent;
    loadDirective->loadFilePath = Context->getLexeme(Parser->token.text.drop_front(1).drop_back(1));

    ConsumeToken(Parser);

    return loadDirective;
}

// MARK: - Types

/// grammar: any-type-identifier := "Any"
static ASTTypeRef* ParseAnyType(Parser* Parser, ASTContext* Context) {
    assert(Parser->token.kind == TOKEN_KEYWORD_ANY);

    ConsumeToken(Parser);

    auto TypeRef = new (Context) ASTAnyTypeRef;
    TypeRef->declContext = Parser->declContext;
    PushParent(Parser, TypeRef);
    PopParent(Parser);

    return TypeRef;
}

/// grammar: opaque-type := identifier
static ASTTypeRef* ParseOpaqueType(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_IDENTIFIER);

    auto TypeRef = new (Context) ASTOpaqueTypeRef;
    TypeRef->declContext = Parser->declContext;
    PushParent(Parser, TypeRef);
    PopParent(Parser);

    TypeRef->typeName = Context->getLexeme(Parser->token.text);
    ConsumeToken(Parser);

    return TypeRef;
}

/// grammar: type-of-type-identifier := "typeof" "(" expression ")"
static ASTTypeRef* ParseTypeOfType(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_KEYWORD_TYPEOF);

    ConsumeToken(Parser);

    if (!Parser->token.is('(')) {
        Parser->report(DIAG_ERROR, "Expected ( after typeof keyword!");
        return nullptr;
    }
    ConsumeToken(Parser);

    auto TypeRef = new (Context) ASTTypeOfTypeRef;
    TypeRef->declContext = Parser->declContext;
    PushParent(Parser, TypeRef);
    {
        TypeRef->expr = ParseExpr(Parser, Context, Diag);
        if (!TypeRef->expr) {
            Parser->report(DIAG_ERROR, "Expected expression as argument in typeof !");
            return nullptr;
        }
    }
    PopParent(Parser);

    if (!Parser->token.is(')')) {
        Parser->report(DIAG_ERROR, "Expected ) after expression of typeof keyword!");
        return nullptr;
    }
    ConsumeToken(Parser);

    return TypeRef;
}

/// grammar: pointer-type-identifier := type-identifier "*"
static ASTTypeRef* ParsePointerType(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, ASTTypeRef* PointeeTypeRef, bool* DidFail) {
    uint32_t Depth = 0;
    while (Parser->token.is(TOKEN_OPERATOR)
           && Parser->lexer->getOperator(Parser->token.text, OPERATOR_POSTFIX, Parser->op)
           && Parser->op.text.equals("*")) {
        Depth += 1;
        ConsumeToken(Parser);
    }

    if (Depth < 1) {
        *DidFail = true;
        return PointeeTypeRef;
    }

    auto TypeRef = new (Context) ASTPointerTypeRef;
    TypeRef->declContext = Parser->declContext;
    PushParent(Parser, TypeRef);
    PopParent(Parser);
    TypeRef->depth = Depth;
    TypeRef->pointeeTypeRef = PointeeTypeRef;
    TypeRef->pointeeTypeRef->parent = TypeRef;
    return TypeRef;
}

/// grammar: array-type-identifier := type-identifier "[" [ expression ] "]"
static ASTTypeRef* ParseArrayType(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag, ASTTypeRef* ElementTypeRef) {
    assert(Parser->token.kind == '[');

    ConsumeToken(Parser);

    auto TypeRef = new (Context) ASTArrayTypeRef;
    TypeRef->declContext = Parser->declContext;
    TypeRef->elementTypeRef = ElementTypeRef;
    TypeRef->elementTypeRef->parent = TypeRef;

    PushParent(Parser, TypeRef);
    {
        if (!Parser->token.is(']')) {
            TypeRef->sizeExpr = ParseExpr(Parser, Context, Diag);
            if (!TypeRef->sizeExpr) {
                Parser->report(DIAG_ERROR, "Expected expression for size of array-type-identifier after '[' !");
                return nullptr;
            }
        }
    }
    PopParent(Parser);

    if (!Parser->token.is(']')) {
        Parser->report(DIAG_ERROR, "Expected ] after expression of array-type-identifier!");
        return nullptr;
    }
    ConsumeToken(Parser);

    return TypeRef;
}

/// grammar: type-identifier := identifier | any-type-identifier | pointer-type-identifier | array-type-identifier | type-of-type-identifier
static ASTTypeRef* ParseType(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    ASTTypeRef* TypeRef = nullptr;

    switch (Parser->token.kind) {
        case TOKEN_KEYWORD_ANY:
            TypeRef = ParseAnyType(Parser, Context);
            break;

        case TOKEN_IDENTIFIER:
            TypeRef = ParseOpaqueType(Parser, Context, Diag);
            break;

        case TOKEN_KEYWORD_TYPEOF:
            TypeRef = ParseTypeOfType(Parser, Context, Diag);
            break;

        default:
            Parser->report(DIAG_ERROR, "Expected type identifier!");
            return nullptr;
    }

    bool DidFinish = false;
    while (!DidFinish && TypeRef) {
        switch (Parser->token.kind) {
            case TOKEN_OPERATOR:
                TypeRef = ParsePointerType(Parser, Context, Diag, TypeRef, &DidFinish);
                break;

            case '[':
                TypeRef = ParseArrayType(Parser, Context, Diag, TypeRef);
                break;

            default:
                DidFinish = true;
                break;
        }
    }

    return TypeRef;
}

// MARK: - Block

ASTStmt* ParseStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag);

// grammar: compound-statement := '{' { statement } '}'
static ASTCompoundStmt* ParseCompoundStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (!Parser->token.is('{')) {
        Parser->report(DIAG_ERROR, "Expected '{' at start of block!");
        return nullptr;
    }
    ConsumeToken(Parser);

    auto compoundStmt = new (Context) ASTCompoundStmt;
    compoundStmt->declContext = Parser->declContext;

    PushParent(Parser, compoundStmt);
    {
        llvm::SmallVector<ASTStmt*, 0> stmts;

        if (!Parser->token.is('}')) {
            unsigned line = Parser->token.line;
            while (true) {
                if (stmts.size() > 0 && line == Parser->token.line) {
                    Parser->report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
                    return nullptr;
                }
                line = Parser->token.line;

                ASTStmt* stmt = ParseStmt(Parser, Context, Diag);
                if (stmt == nullptr) {
                    return nullptr;
                }

                stmts.push_back(stmt);

                if (Parser->token.is('}')) {
                    break;
                }
            }
        }
        ConsumeToken(Parser);

        compoundStmt->stmts = llvm::makeArrayRef(stmts).copy(Context->nodeAllocator);
    }
    PopParent(Parser);

    return compoundStmt;
}

// MARK: - Context Declarations

/// grammar: enum-element := "case" identifier [ "=" expression ]
static ASTEnumElementDecl* ParseEnumElementDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (!Parser->token.is(TOKEN_KEYWORD_CASE)) {
        Parser->report(DIAG_ERROR, "Expected 'case' keyword at start of enum element!");
        return nullptr;
    }
    ConsumeToken(Parser);

    ASTEnumElementDecl* EnumElement = new (Context) ASTEnumElementDecl;
    EnumElement->declContext = Parser->declContext;
    PushParent(Parser, EnumElement);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier for name of enum element!");
            return nullptr;
        }
        EnumElement->name = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (Parser->token.is(TOKEN_OPERATOR)
            && Parser->lexer->getOperator(Parser->token.text, OPERATOR_INFIX, Parser->op)
            && Parser->op.text.equals("=")) {
            ConsumeToken(Parser);

            EnumElement->assignment = ParseExpr(Parser, Context, Diag);
            if (EnumElement->assignment == nullptr) {
                return nullptr;
            }
        }

    }
    PopParent(Parser);

    return EnumElement;
}

/// grammar: parameter := identifier ":" type-identifier
static ASTParamDecl* ParseParameterDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    ASTParamDecl* Parameter = new (Context) ASTParamDecl;
    Parameter->declContext = Parser->declContext;
    PushParent(Parser, Parameter);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier for name of parameter!");
            return nullptr;
        }
        Parameter->name = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (!Parser->token.is(':')) {
            Parser->report(DIAG_ERROR, "Expected ':' after name of parameter!");
            return nullptr;
        }
        ConsumeToken(Parser);

        Parameter->typeRef = ParseType(Parser, Context, Diag);
        if (Parameter->typeRef == nullptr) {
//            Parser->report(DIAG_ERROR, "Expected type of parameter!");
            return nullptr;
        }
    }
    PopParent(Parser);

    return Parameter;
}

// MARK: - Declarations

/// grammar: enum-declaration := "enum" identifier "{" [ enum-element { line-break enum-element } ] "}"
static ASTEnumDecl* ParseEnumDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_ENUM) && "Invalid token given for start of enum!");

    ConsumeToken(Parser);

    ASTEnumDecl* Enum = new (Context) ASTEnumDecl;
    Enum->declContext = Parser->declContext;

    PushDeclContext(Parser, Enum);
    PushParent(Parser, Enum);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier for name of enum declaration!");
            return nullptr;
        }
        Enum->name = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (!Parser->token.is('{')) {
            Parser->report(DIAG_ERROR, "Expected '{' after name of enum declaration!");
            return nullptr;
        }
        ConsumeToken(Parser);

        if (!Parser->token.is('}')) {
            unsigned line = Parser->token.line;
            while (true) {
                if (Enum->containsDecls() > 0 && line == Parser->token.line) {
                    Parser->report(DIAG_ERROR, "Consecutive enum elements on a line are not allowed!");
                    return nullptr;
                }

                ASTEnumElementDecl* EnumElement = ParseEnumElementDecl(Parser, Context, Diag);
                if (EnumElement == nullptr) {
                    return nullptr;
                }

                if (!Enum->lookupDecl(EnumElement->name)) {
                    Enum->addDecl(EnumElement);
                } else {
                    Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", EnumElement->name);
                }

                if (Parser->token.is('}')) {
                    break;
                } else if (!Parser->token.is(TOKEN_KEYWORD_CASE)) {
                    Parser->report(DIAG_ERROR, "Expected '}' at end of enum declaration!");
                    return nullptr;
                }
            }
        }
        ConsumeToken(Parser);
    }
    PopParent(Parser);
    PopDeclContext(Parser);

    return Enum;
}

/// grammar: func-declaration := "func" identifier "(" [ parameter { "," parameter } ] ")" "->" type-identifier block
static ASTFuncDecl* ParseFuncDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_FUNC) && "Invalid token given for start of func declaration!");
    ConsumeToken(Parser);

    ASTFuncDecl* Func = new (Context) ASTFuncDecl;
    Func->declContext = Parser->declContext;

    PushDeclContext(Parser, Func);
    PushParent(Parser, Func);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier in function declaration!");
            return nullptr;
        }
        Func->name = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (!Parser->token.is('(')) {
            Parser->report(DIAG_ERROR, "Expected '(' in parameter list of function declaration!");
            return nullptr;
        }
        ConsumeToken(Parser);

        llvm::SmallVector<ASTParamDecl*, 0> parameters;
        if (!Parser->token.is(')')) {
            while (true) {
                ASTParamDecl* parameter = ParseParameterDecl(Parser, Context, Diag);
                if (parameter == nullptr) {
                    return nullptr;
                }

                parameters.push_back(parameter);

                if (Parser->token.is(')')) {
                    break;
                } else if (!Parser->token.is(',')) {
                    Parser->report(DIAG_ERROR, "Expected ')' or ',' in parameter list of function declaration!");
                    return nullptr;
                }
                ConsumeToken(Parser);
            }
        }
        ConsumeToken(Parser);

        Func->parameters = llvm::makeArrayRef(parameters).copy(Context->nodeAllocator);

        for (auto parameter : Func->parameters) {
            if (!Func->lookupDecl(parameter->name)) {
                Func->addDecl(parameter);
            } else {
                Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", parameter->name);
            }
        }

        if (!Parser->token.is(TOKEN_ARROW)) {
            Parser->report(DIAG_ERROR, "Expected '->' in function declaration!");
            return nullptr;
        }
        ConsumeToken(Parser);

        Func->returnTypeRef = ParseType(Parser, Context, Diag);
        if (!Func->returnTypeRef) {
            return nullptr;
        }

        Func->body = ParseCompoundStmt(Parser, Context, Diag);
        if (!Func->body) {
            return nullptr;
        }

        for (auto stmt : Func->body->stmts) {
            if (stmt->isDecl()) {
                auto decl = reinterpret_cast<ASTDecl*>(stmt);
                if (decl->isNamedDecl()) {
                    auto namedDecl = reinterpret_cast<ASTNamedDecl*>(decl);
                    if (!Func->lookupDecl(namedDecl->name)) {
                        Func->addDecl(namedDecl);
                    } else {
                        Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", namedDecl->name);
                    }
                } else {
                    Func->addDecl(decl);
                }
            }
        }
    }
    PopParent(Parser);
    PopDeclContext(Parser);

    return Func;
}

/// grammar: value-declaration := var-declaration | let-declaration
static ASTValueDecl* ParseValueDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag);

/// grammar: struct-declaration := "struct" identifier "{" { value-declaration } "}"
static ASTStructDecl* ParseStructDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_STRUCT) && "Invalid token given for start of struct!");
    ConsumeToken(Parser);

    ASTStructDecl* Struct = new (Context) ASTStructDecl;
    Struct->declContext = Parser->declContext;

    PushDeclContext(Parser, Struct);
    PushParent(Parser, Struct);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier for name of struct declaration!");
            return nullptr;
        }
        Struct->name = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (!Parser->token.is('{')) {
            Parser->report(DIAG_ERROR, "Expected '{' after identifier of struct declaration!");
            return nullptr;
        }
        ConsumeToken(Parser);

        if (!Parser->token.is('}')) {
            unsigned line = Parser->token.line;
            while (true) {
                if (Struct->containsDecls() && line == Parser->token.line) {
                    Parser->report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
                    return nullptr;
                }
                line = Parser->token.line;

                auto decl = ParseValueDecl(Parser, Context, Diag);
                if (!decl) {
                    Parser->report(DIAG_ERROR, "Expected value declaration or closing '}' in struct declaration!");
                    return nullptr;
                }

                if (!Struct->lookupDecl(decl->name)) {
                    Struct->addDecl(decl);
                } else {
                    Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", decl->name);
                }

                if (Parser->token.is('}')) {
                    break;
                }
            }
        }
        ConsumeToken(Parser);
    }
    PopParent(Parser);
    PopDeclContext(Parser);

    return Struct;
}

/// grammar: value-declaration := var-declaration | let-declaration
/// grammar: var-declaration := "var" identifier ":" type-identifier [ "=" expression ]
/// grammar: let-declaration := "let" identifier ":" type-identifier [ "=" expression ]
static ASTValueDecl* ParseValueDecl(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (!Parser->token.is(TOKEN_KEYWORD_VAR, TOKEN_KEYWORD_LET)) {
        return nullptr;
    }

    bool isConstant = Parser->token.is(TOKEN_KEYWORD_LET);
    ConsumeToken(Parser);

    ASTValueDecl* decl = new (Context) ASTValueDecl(isConstant);
    decl->declContext = Parser->declContext;
    PushParent(Parser, decl);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier for name of variable declaration!");
            return nullptr;
        }
        decl->name = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (!Parser->token.is(':')) {
            Parser->report(DIAG_ERROR, "Expected ':' after variable name identifier!");
            return nullptr;
        }
        ConsumeToken(Parser);

        decl->typeRef = ParseType(Parser, Context, Diag);
        if (!decl->typeRef) {
            return nullptr;
        }

        if (Parser->token.is(TOKEN_OPERATOR)
            && Parser->lexer->getOperator(Parser->token.text, OPERATOR_INFIX, Parser->op)
            && Parser->op.text.equals("=")) {
            ConsumeToken(Parser);

            decl->initializer = ParseExpr(Parser, Context, Diag);
            if (!decl->initializer) {
                return nullptr;
            }
        }

    }
    PopParent(Parser);

    return decl;
}

// MARK: - Top Level Declarations

/// grammar: top-level-node := directive | enum-declaration | func-declaration | struct-declaration | variable-declaration
ASTNode* ParseTopLevelNode(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    switch (Parser->token.kind) {
        case TOKEN_KEYWORD_LOAD:   return ParseLoadDirective(Parser, Context, Diag);
        case TOKEN_KEYWORD_ENUM:   return ParseEnumDecl(Parser, Context, Diag);
        case TOKEN_KEYWORD_FUNC:   return ParseFuncDecl(Parser, Context, Diag);
        case TOKEN_KEYWORD_STRUCT: return ParseStructDecl(Parser, Context, Diag);

        case TOKEN_KEYWORD_VAR:
        case TOKEN_KEYWORD_LET:
            return ParseValueDecl(Parser, Context, Diag);

        case TOKEN_EOF:            return nullptr;
        default:
            Parser->report(DIAG_ERROR, "Unexpected token found expected top level declaration!");
            return nullptr;
    }
}

// MARK: - Statements

/// grammar: break-statement := "break"
static ASTBreakStmt* ParseBreakStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_KEYWORD_BREAK);

    ConsumeToken(Parser);

    ASTBreakStmt* Break = new (Context) ASTBreakStmt;
    Break->declContext = Parser->declContext;
    PushParent(Parser, Break);
    PopParent(Parser);
    return Break;
}

/// grammar: continue-statement := "continue"
static ASTContinueStmt* ParseContinueStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_KEYWORD_CONTINUE);

    ConsumeToken(Parser);

    ASTContinueStmt* Continue = new (Context) ASTContinueStmt;
    Continue->declContext = Parser->declContext;
    PushParent(Parser, Continue);
    PopParent(Parser);
    return Continue;
}

/// grammar: fallthrough-statement := "fallthrough"
static ASTFallthroughStmt* ParseFallthroughStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_KEYWORD_FALLTHROUGH);

    ConsumeToken(Parser);

    ASTFallthroughStmt* Fallthrough = new (Context) ASTFallthroughStmt;
    Fallthrough->declContext = Parser->declContext;
    PushParent(Parser, Fallthrough);
    PopParent(Parser);
    return Fallthrough;
}

/// grammar: return-statement := "return" [ expression ]
static ASTReturnStmt* ParseReturnStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.kind == TOKEN_KEYWORD_RETURN);

    ConsumeToken(Parser);

    ASTReturnStmt* Return = new (Context) ASTReturnStmt;
    Return->declContext = Parser->declContext;
    PushParent(Parser, Return);
    {
        Return->expr = TryParseExpr(Parser, Context, Diag);
    }
    PopParent(Parser);

    return Return;
}

/// grammar: defer-statement := "defer" expression
static ASTDeferStmt* ParseDeferStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_DEFER) && "Invalid token given for start of defer-statement!");
    ConsumeToken(Parser);

    ASTDeferStmt* Defer = new (Context) ASTDeferStmt;
    Defer->declContext = Parser->declContext;
    PushParent(Parser, Defer);
    {
        Defer->expr = ParseExpr(Parser, Context, Diag);
        if (!Defer->expr) {
            return nullptr;
        }
    }
    PopParent(Parser);

    return Defer;
}

/// grammar: do-statement := "do" block "while" expression
static ASTDoStmt* ParseDoStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_DO) && "Invalid token given for start of do-statement");

    ConsumeToken(Parser);

    ASTDoStmt* Do = new (Context) ASTDoStmt;
    Do->declContext = Parser->declContext;
    PushParent(Parser, Do);
    {
        Do->body = ParseCompoundStmt(Parser, Context, Diag);
        if (!Do->body) {
            return nullptr;
        }

        if (!Parser->token.is(TOKEN_KEYWORD_WHILE)) {
            Parser->report(DIAG_ERROR, "Expected keyword 'while' after do block!");
            return nullptr;
        }
        ConsumeToken(Parser);

        do {

            ASTExpr* expr = ParseExpr(Parser, Context, Diag);
            if (expr == nullptr) {
                return nullptr;
            }

            if (!Do->condition) {
                Do->condition = expr;
            } else {
                auto andExpr = new (Context) ASTBinaryExpr;
                andExpr->parent = Parser->parent;
                andExpr->declContext = Parser->declContext;
                andExpr->left = Do->condition;
                andExpr->left->parent = andExpr;
                andExpr->right = expr;
                andExpr->right->parent = andExpr;

                if (!Parser->lexer->getOperator("&&", OPERATOR_INFIX, andExpr->op)) {
                    llvm::report_fatal_error("Internal compiler error!");
                }

                Do->condition = andExpr;
            }

            if (!Parser->token.is(',')) {
                break;
            }
            ConsumeToken(Parser);

        } while (true);
    }
    PopParent(Parser);

    return Do;
}

/// grammar: for-statement := "for" identifier "in" expression block
static ASTForStmt* ParseForStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_FOR) && "Invalid token given for start of for-statement");

    ConsumeToken(Parser);

    ASTForStmt* For = new (Context) ASTForStmt;
    For->declContext = Parser->declContext;
    PushParent(Parser, For);
    {
        if (!Parser->token.is(TOKEN_IDENTIFIER)) {
            Parser->report(DIAG_ERROR, "Expected identifier for iterator in for-statement!");
            return nullptr;
        }
        For->elementName = Context->getLexeme(Parser->token.text);
        ConsumeToken(Parser);

        if (!Parser->token.is(TOKEN_KEYWORD_IN)) {
            Parser->report(DIAG_ERROR, "Expected keyword 'in' after for iterator");
            return nullptr;
        }
        ConsumeToken(Parser);

        For->sequenceExpr = ParseExpr(Parser, Context, Diag);
        if (!For->sequenceExpr) {
            return nullptr;
        }

        For->body = ParseCompoundStmt(Parser, Context, Diag);
        if (!For->body) {
            return nullptr;
        }
    }
    PopParent(Parser);

    return For;
}

/// grammar: guard-statement := "guard" expression { "," expression } else block
static ASTGuardStmt* ParseGuardStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_GUARD) && "Invalid token given for start of guard-statement");

    ConsumeToken(Parser);

    ASTGuardStmt* Guard = new (Context) ASTGuardStmt;
    Guard->declContext = Parser->declContext;
    PushParent(Parser, Guard);
    {
        do {

            ASTExpr* expr = ParseExpr(Parser, Context, Diag);
            if (!expr) {
                return nullptr;
            }

            if (!Guard->condition) {
                Guard->condition = expr;
            } else {
                auto andExpr = new (Context) ASTBinaryExpr;
                andExpr->parent = Parser->parent;
                andExpr->declContext = Parser->declContext;
                andExpr->left = Guard->condition;
                andExpr->left->parent = andExpr;
                andExpr->right = expr;
                andExpr->right->parent = andExpr;

                if (!Parser->lexer->getOperator("&&", OPERATOR_INFIX, andExpr->op)) {
                    llvm::report_fatal_error("Internal compiler error!");
                }

                Guard->condition = andExpr;
            }

            if (!Parser->token.is(',')) {
                break;
            }
            ConsumeToken(Parser);

        } while (true);

        if (!Parser->token.is(TOKEN_KEYWORD_ELSE)) {
            Parser->report(DIAG_ERROR, "Expected keyword 'else' in guard-statement");
            return nullptr;
        }
        ConsumeToken(Parser);

        Guard->elseStmt = ParseCompoundStmt(Parser, Context, Diag);
        if (!Guard->elseStmt) {
            return nullptr;
        }
    }
    PopParent(Parser);

    return Guard;
}

/// grammar: if-statement := "if" expression { "," expression } block [ "else" ( if-statement | block ) ]
static ASTIfStmt* ParseIfStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_IF) && "Invalid token given for start of if-statement!");

    ConsumeToken(Parser);

    ASTIfStmt* If = new (Context) ASTIfStmt;
    If->declContext = Parser->declContext;
    PushParent(Parser, If);
    {
        do {
            ASTExpr* expr = ParseExpr(Parser, Context, Diag);
            if (!expr) {
                return nullptr;
            }

            if (!If->condition) {
                If->condition = expr;
            } else {
                auto andExpr = new (Context) ASTBinaryExpr;
                andExpr->parent = Parser->parent;
                andExpr->declContext = Parser->declContext;
                andExpr->left = If->condition;
                andExpr->left->parent = andExpr;
                andExpr->right = expr;
                andExpr->right->parent = andExpr;

                if (!Parser->lexer->getOperator("&&", OPERATOR_INFIX, andExpr->op)) {
                    llvm::report_fatal_error("Internal compiler error!");
                }

                If->condition = andExpr;
            }

            if (!Parser->token.is(',')) {
                break;
            }
            ConsumeToken(Parser);

        } while (true);

        If->thenStmt = ParseCompoundStmt(Parser, Context, Diag);
        if (!If->thenStmt) {
            return nullptr;
        }

        if (Parser->token.is(TOKEN_KEYWORD_ELSE)) {
            ConsumeToken(Parser);

            if (Parser->token.is(TOKEN_KEYWORD_IF)) {
                If->chainKind = AST_CHAIN_IF;
                If->elseIf = ParseIfStmt(Parser, Context, Diag);
                if (!If->elseIf) {
                    return nullptr;
                }
            } else {
                If->chainKind = AST_CHAIN_ELSE;
                If->elseStmt = ParseCompoundStmt(Parser, Context, Diag);
                if (!If->elseStmt) {
                    return nullptr;
                }
            }
        }
    }
    PopParent(Parser);

    return If;
}

/// grammar: switch-case-statement := ( "case" expression | "else" ) ":" statement { line-break statement }
static ASTCaseStmt* ParseSwitchCaseStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    if (!Parser->token.is(TOKEN_KEYWORD_CASE, TOKEN_KEYWORD_ELSE)) {
        Diag->report(DIAG_ERROR, "Expected 'case' or 'else' keyword, found '{0}'", Parser->token.text);
        return nullptr;
    }

    ASTCaseStmt* Case = new (Context) ASTCaseStmt;
    Case->declContext = Parser->declContext;

    PushDeclContext(Parser, Case);
    PushParent(Parser, Case);
    {
        if (Parser->token.is(TOKEN_KEYWORD_CASE)) {
            ConsumeToken(Parser);
            Case->caseKind = AST_CASE_CONDITION;
            Case->condition = ParseExpr(Parser, Context, Diag);
            if (Case->condition == nullptr) {
                return nullptr;
            }
        } else {
            ConsumeToken(Parser);
            Case->caseKind = AST_CASE_ELSE;
        }

        if (!Parser->token.is(':')) {
            Parser->report(DIAG_ERROR, "Expected ':' in switch-case statement!");
            return nullptr;
        }
        ConsumeToken(Parser);

        Case->body = new (Context) ASTCompoundStmt;
        Case->body->declContext = Parser->declContext;

        PushParent(Parser, Case->body);
        {
            llvm::SmallVector<ASTStmt*, 0> stmts;
            unsigned line = Parser->token.line;
            do {
                if (Parser->token.is(TOKEN_KEYWORD_CASE, TOKEN_KEYWORD_ELSE, '}')) {
                    break;
                }

                if (stmts.size() > 0 && line == Parser->token.line) {
                    Parser->report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
                    return nullptr;
                }
                line = Parser->token.line;

                ASTStmt* stmt = ParseStmt(Parser, Context, Diag);
                if (!stmt) {
                    Parser->report(DIAG_ERROR, "Expected statement in switch-case!");
                    return nullptr;
                }

                // @Refactor if this is a decl and is added to the decl context is it still required to be part of the statements?
                stmts.push_back(stmt);

                if (stmt->isDecl()) {
                    auto Decl = reinterpret_cast<ASTDecl*>(stmt);
                    if (Decl->isNamedDecl()) {
                        auto namedDecl = reinterpret_cast<ASTNamedDecl*>(Decl);
                        if (!Case->lookupDecl(namedDecl->name)) {
                            Case->addDecl(namedDecl);
                        } else {
                            Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", namedDecl->name);
                        }
                    } else {
                        Case->addDecl(Decl);
                    }
                }

            } while (true);

            Case->body->stmts = llvm::makeArrayRef(stmts).copy(Context->nodeAllocator);
        }
        PopParent(Parser);
    }
    PopParent(Parser);
    PopDeclContext(Parser);

    return Case;
}

/// grammar: switch-statement := "switch" expression "{" [ switch-case { line-break switch-case } ] "}"
static ASTSwitchStmt* ParseSwitchStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_SWITCH) && "Invalid token given for start of switch-statement!");
    ConsumeToken(Parser);

    ASTSwitchStmt* Switch = new (Context) ASTSwitchStmt;
    Switch->declContext = Parser->declContext;

    PushParent(Parser, Switch);
    {
        Switch->expr = ParseExpr(Parser, Context, Diag);
        if (!Switch->expr) {
            return nullptr;
        }

        if (!Parser->token.is('{')) {
            Parser->report(DIAG_ERROR, "Expected '{' after expression in switch-statement!");
            return nullptr;
        }
        ConsumeToken(Parser);

        llvm::SmallVector<ASTCaseStmt*, 0> cases;
        unsigned line = Parser->token.line;
        do {
            if (cases.size() > 0 && line == Parser->token.line) {
                Parser->report(DIAG_ERROR, "Consecutive statements on a line are not allowed!");
                return nullptr;
            }
            line = Parser->token.line;

            ASTCaseStmt* caseStmt = ParseSwitchCaseStmt(Parser, Context, Diag);
            if (!caseStmt) {
                return nullptr;
            }

            cases.push_back(caseStmt);

            if (Parser->token.is('}')) {
                break;
            }

        } while (true);

        ConsumeToken(Parser);

        Switch->cases = llvm::makeArrayRef(cases).copy(Context->nodeAllocator);
    }
    PopParent(Parser);

    return Switch;
}

/// grammar: while-statement := "while" expression { "," expression } block
static ASTWhileStmt* ParseWhileStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    assert(Parser->token.is(TOKEN_KEYWORD_WHILE) && "Invalid token given for start of while-statement!");
    ConsumeToken(Parser);

    ASTWhileStmt* While = new (Context) ASTWhileStmt;
    While->declContext = Parser->declContext;
    PushParent(Parser, While);
    {
        do {

            ASTExpr* expr = ParseExpr(Parser, Context, Diag);
            if (!expr) {
                return nullptr;
            }

            if (!While->condition) {
                While->condition = expr;
            } else {
                auto andExpr = new (Context) ASTBinaryExpr;
                andExpr->parent = Parser->parent;
                andExpr->declContext = Parser->declContext;
                andExpr->left = While->condition;
                andExpr->left->parent = andExpr;
                andExpr->right = expr;
                andExpr->right->parent = andExpr;

                if (!Parser->lexer->getOperator("&&", OPERATOR_INFIX, andExpr->op)) {
                    llvm::report_fatal_error("Internal compiler error!");
                }

                While->condition = andExpr;
            }

            if (!Parser->token.is(',')) {
                break;
            }
            ConsumeToken(Parser);

        } while (true);

        While->body = ParseCompoundStmt(Parser, Context, Diag);
        if (!While->body) {
            return nullptr;
        }
    }
    PopParent(Parser);

    return While;
}

/// grammar: statement := variable-declaration | control-statement | defer-statement | do-statement | for-statement | guard-statement | if-statement | switch-statement | while-statement | expression
ASTStmt* ParseStmt(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    switch (Parser->token.kind) {
        // @Incomplete allow load directives to be parsed as statements in blocks
        //             after adding support for compile-time evaluated functions
        //             which indeed can contain load directives ...
//        case TOKEN_KEYWORD_LOAD:        return ParseLoadDirective(Parser, Context, Diag);
        case TOKEN_KEYWORD_ENUM:        return ParseEnumDecl(Parser, Context, Diag);
        case TOKEN_KEYWORD_FUNC:        return ParseFuncDecl(Parser, Context, Diag);
        case TOKEN_KEYWORD_STRUCT:      return ParseStructDecl(Parser, Context, Diag);

        case TOKEN_KEYWORD_VAR:
        case TOKEN_KEYWORD_LET:
            return ParseValueDecl(Parser, Context, Diag);

        case TOKEN_KEYWORD_BREAK:       return ParseBreakStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_CONTINUE:    return ParseContinueStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_FALLTHROUGH: return ParseFallthroughStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_RETURN:      return ParseReturnStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_DEFER:       return ParseDeferStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_DO:          return ParseDoStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_FOR:         return ParseForStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_GUARD:       return ParseGuardStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_IF:          return ParseIfStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_SWITCH:      return ParseSwitchStmt(Parser, Context, Diag);
        case TOKEN_KEYWORD_WHILE:       return ParseWhileStmt(Parser, Context, Diag);
        default: {
            auto expr = TryParseExpr(Parser, Context, Diag);
            if (!expr) {
                Parser->report(DIAG_ERROR, "Expected statement, found '{0}'", Parser->token.text);
            }
            return expr;
        }
    }
}

void ParseAllTopLevelNodes(Parser* Parser, ASTContext* Context, DiagnosticEngine* Diag) {
    Parser->token = Parser->lexer->peekNextToken();

    auto module = Context->getModule();
    PushParent(Parser, module);
    PushDeclContext(Parser, module);
    {
        bool checkConsecutiveTopLevelNodes = false;
        unsigned line = Parser->token.line;
        do {
            unsigned nodeLine = Parser->token.line;
            ASTNode* Node = ParseTopLevelNode(Parser, Context, Diag);
            if (!Node) {
                break;
            }

            if (checkConsecutiveTopLevelNodes && line == nodeLine) {
                Parser->report(DIAG_ERROR, "Consecutive top level nodes on a line are not allowed!");
                break;
            }
            checkConsecutiveTopLevelNodes = true;
            line = nodeLine;

            assert(Node->isDecl());
            auto decl = reinterpret_cast<ASTDecl*>(Node);
            if (decl->isNamedDecl()) {
                auto namedDecl = reinterpret_cast<ASTNamedDecl*>(decl);
                if (module->lookupDecl(namedDecl->name)) {
                    Parser->report(DIAG_ERROR, "Invalid redeclaration of '{0}'", namedDecl->name);
                } else {
                    module->addDecl(namedDecl);
                }
            } else {
                module->addDecl(decl);
            }

        } while (true);
    }
    PopParent(Parser);
    PopDeclContext(Parser);
}
