//
// MIT License
//
// Copyright (c) 2019 Murat Yilmaz
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

#include "AST/ArrayTypeRef.h"
#include "AST/BinaryExpression.h"
#include "AST/BlockStatement.h"
#include "AST/BoolLiteral.h"
#include "AST/BreakStatement.h"
#include "AST/CallExpression.h"
#include "AST/ConditionalCaseStatement.h"
#include "AST/ConstantDeclaration.h"
#include "AST/ContinueStatement.h"
#include "AST/DeferStatement.h"
#include "AST/DoStatement.h"
#include "AST/Dumper.h"
#include "AST/ElseCaseStatement.h"
#include "AST/EnumerationDeclaration.h"
#include "AST/EnumerationElementDeclaration.h"
#include "AST/FallthroughStatement.h"
#include "AST/FloatLiteral.h"
#include "AST/FunctionDeclaration.h"
#include "AST/GuardStatement.h"
#include "AST/IdentifierExpression.h"
#include "AST/IfStatement.h"
#include "AST/IntLiteral.h"
#include "AST/LoadDeclaration.h"
#include "AST/MemberAccessExpression.h"
#include "AST/ModuleDeclaration.h"
#include "AST/NilLiteral.h"
#include "AST/Node.h"
#include "AST/OpaqueTypeRef.h"
#include "AST/ParameterDeclaration.h"
#include "AST/PointerTypeRef.h"
#include "AST/ReturnStatement.h"
#include "AST/StringLiteral.h"
#include "AST/StructureDeclaration.h"
#include "AST/SwitchStatement.h"
#include "AST/TypeOfTypeRef.h"
#include "AST/UnaryExpression.h"
#include "AST/VariableDeclaration.h"
#include "AST/WhileStatement.h"

// @Incomplete Add runtime informations like memory address and also other members like types
//             for now this implementation will be helpful to run tests for the Parser...

using namespace jelly::AST;

Dumper::Dumper(std::ostream& stream) :
stream(stream),
indentation(0) {
}

void Dumper::printIndentation() {
    for (auto i = 0; i < indentation; i++) {
        stream << "  ";
    }
}

void Dumper::printString(StringRef text) {
    stream << text.str();
}

void Dumper::printProperty(StringRef key, StringRef value) {
    indentation += 1;
    printIndentation();
    printString("@");
    printString(key);
    printString(" = '");
    printString(value);
    printString("'\n");
    indentation -= 1;
}

void Dumper::dumpKind(Node::Kind kind) {
    switch (kind) {
        case Node::Kind::Block:                 return printString("BlockStatement");
        case Node::Kind::GuardStmt:             return printString("GuardStatement");
        case Node::Kind::IfStmt:                return printString("IfStatement");
        case Node::Kind::DoStmt:                return printString("DoStatement");
        case Node::Kind::WhileStmt:             return printString("WhileStatement");
        case Node::Kind::ConditionalCaseStmt:   return printString("ConditionalCaseStatement");
        case Node::Kind::ElseCaseStmt:          return printString("ElseCaseStatement");
        case Node::Kind::BreakStmt:             return printString("BreakStatement");
        case Node::Kind::ContinueStmt:          return printString("ContinueStatement");
        case Node::Kind::FallthroughStmt:       return printString("FallthroughStatement");
        case Node::Kind::ReturnStmt:            return printString("ReturnStatement");
        case Node::Kind::Defer:                 return printString("DeferStatement");
        case Node::Kind::LoadDecl:              return printString("LoadDeclaration");
        case Node::Kind::Module:                return printString("ModuleDeclaration");
        case Node::Kind::EnumerationElement:    return printString("EnumerationElementDeclaration");
        case Node::Kind::Parameter:             return printString("ParameterDeclaration");
        case Node::Kind::Enumeration:           return printString("EnumerationDeclaration");
        case Node::Kind::Function:              return printString("FunctionDeclaration");
        case Node::Kind::Structure:             return printString("StructureDeclaration");
        case Node::Kind::Constant:              return printString("ConstantDeclaration");
        case Node::Kind::Variable:              return printString("VariableDeclaration");
        case Node::Kind::UnaryExpr:             return printString("UnaryExpression");
        case Node::Kind::BinaryExpr:            return printString("BinaryExpression");
        case Node::Kind::IdentifierExpr:        return printString("IdentifierExpression");
        case Node::Kind::MemberAccessExpr:      return printString("MemberAccessExpression");
        case Node::Kind::CallExpr:              return printString("CallExpression");
        case Node::Kind::NilLit:                return printString("NilLiteral");
        case Node::Kind::BoolLit:               return printString("BoolLiteral");
        case Node::Kind::IntLit:                return printString("IntLiteral");
        case Node::Kind::FloatLit:              return printString("FloatLiteral");
        case Node::Kind::StringLit:             return printString("StringLiteral");
        case Node::Kind::SwitchStmt:            return printString("SwitchStatement");
        case Node::Kind::OpaqueTypeRef:         return printString("OpaqueTypeRef");
        case Node::Kind::TypeOfTypeRef:         return printString("TypeOfTypeRef");
        case Node::Kind::PointerTypeRef:        return printString("PointerTypeRef");
        case Node::Kind::ArrayTypeRef:          return printString("ArrayTypeRef");
        default:                                jelly_unreachable("Invalid kind given for node!");
    }
}

void Dumper::dump(Node* node) {
    assert(node);

    printIndentation();
    dumpKind(node->getKind());
    printString("\n");

    switch (node->getKind()) {
        case Node::Kind::Block:                 return dumpBlockStatement(reinterpret_cast<BlockStatement*>(node));
        case Node::Kind::GuardStmt:             return dumpGuardStatement(reinterpret_cast<GuardStatement*>(node));
        case Node::Kind::IfStmt:                return dumpIfStatement(reinterpret_cast<IfStatement*>(node));
        case Node::Kind::DoStmt:                return dumpDoStatement(reinterpret_cast<DoStatement*>(node));
        case Node::Kind::WhileStmt:             return dumpWhileStatement(reinterpret_cast<WhileStatement*>(node));
        case Node::Kind::ConditionalCaseStmt:   return dumpConditionalCaseStatement(reinterpret_cast<ConditionalCaseStatement*>(node));
        case Node::Kind::ElseCaseStmt:          return dumpElseCaseStatement(reinterpret_cast<ElseCaseStatement*>(node));
        case Node::Kind::BreakStmt:             return dumpBreakStatement(reinterpret_cast<BreakStatement*>(node));
        case Node::Kind::ContinueStmt:          return dumpContinueStatement(reinterpret_cast<ContinueStatement*>(node));
        case Node::Kind::FallthroughStmt:       return dumpFallthroughStatement(reinterpret_cast<FallthroughStatement*>(node));
        case Node::Kind::ReturnStmt:            return dumpReturnStatement(reinterpret_cast<ReturnStatement*>(node));
        case Node::Kind::Defer:                 return dumpDeferStatement(reinterpret_cast<DeferStatement*>(node));
        case Node::Kind::LoadDecl:              return dumpLoadDeclaration(reinterpret_cast<LoadDeclaration*>(node));
        case Node::Kind::Module:                return dumpModuleDeclaration(reinterpret_cast<ModuleDeclaration*>(node));
        case Node::Kind::EnumerationElement:    return dumpEnumerationElementDeclaration(reinterpret_cast<EnumerationElementDeclaration*>(node));
        case Node::Kind::Parameter:             return dumpParameterDeclaration(reinterpret_cast<ParameterDeclaration*>(node));
        case Node::Kind::Enumeration:           return dumpEnumerationDeclaration(reinterpret_cast<EnumerationDeclaration*>(node));
        case Node::Kind::Function:              return dumpFunctionDeclaration(reinterpret_cast<FunctionDeclaration*>(node));
        case Node::Kind::Structure:             return dumpStructureDeclaration(reinterpret_cast<StructureDeclaration*>(node));
        case Node::Kind::Constant:              return dumpConstantDeclaration(reinterpret_cast<ConstantDeclaration*>(node));
        case Node::Kind::Variable:              return dumpVariableDeclaration(reinterpret_cast<VariableDeclaration*>(node));
        case Node::Kind::UnaryExpr:             return dumpUnaryExpression(reinterpret_cast<UnaryExpression*>(node));
        case Node::Kind::BinaryExpr:            return dumpBinaryExpression(reinterpret_cast<BinaryExpression*>(node));
        case Node::Kind::IdentifierExpr:        return dumpIdentifierExpression(reinterpret_cast<IdentifierExpression*>(node));
        case Node::Kind::MemberAccessExpr:      return dumpMemberAccessExpression(reinterpret_cast<MemberAccessExpression*>(node));
        case Node::Kind::CallExpr:              return dumpCallExpression(reinterpret_cast<CallExpression*>(node));
        case Node::Kind::NilLit:                return dumpNilLiteral(reinterpret_cast<NilLiteral*>(node));
        case Node::Kind::BoolLit:               return dumpBoolLiteral(reinterpret_cast<BoolLiteral*>(node));
        case Node::Kind::IntLit:                return dumpIntLiteral(reinterpret_cast<IntLiteral*>(node));
        case Node::Kind::FloatLit:              return dumpFloatLiteral(reinterpret_cast<FloatLiteral*>(node));
        case Node::Kind::StringLit:             return dumpStringLiteral(reinterpret_cast<StringLiteral*>(node));
        case Node::Kind::SwitchStmt:            return dumpSwitchStatement(reinterpret_cast<SwitchStatement*>(node));
        case Node::Kind::OpaqueTypeRef:         return dumpOpaqueTypeRef(reinterpret_cast<OpaqueTypeRef*>(node));
        case Node::Kind::TypeOfTypeRef:         return dumpTypeOfTypeRef(reinterpret_cast<TypeOfTypeRef*>(node));
        case Node::Kind::PointerTypeRef:        return dumpPointerTypeRef(reinterpret_cast<PointerTypeRef*>(node));
        case Node::Kind::ArrayTypeRef:          return dumpArrayTypeRef(reinterpret_cast<ArrayTypeRef*>(node));
        default:                                jelly_unreachable("Invalid kind given for node!");
    }
}

void Dumper::dumpBlockStatement(BlockStatement* statement) {
    dumpChildren(statement->getChildren());
}

void Dumper::dumpGuardStatement(GuardStatement* statement) {
    dumpChild(statement->getCondition());
    dumpChild(statement->getElseBlock());
}

void Dumper::dumpIfStatement(IfStatement* statement) {
    dumpChild(statement->getCondition());
    dumpChild(statement->getThenBlock());

    if (statement->getElseBlock()) {
        dumpChild(statement->getElseBlock());
    }
}

void Dumper::dumpDoStatement(DoStatement* statement) {
    dumpChild(statement->getCondition());
    dumpChild(statement->getBody());
}

void Dumper::dumpWhileStatement(WhileStatement* statement) {
    dumpChild(statement->getCondition());
    dumpChild(statement->getBody());
}

void Dumper::dumpConditionalCaseStatement(ConditionalCaseStatement* statement) {
    dumpChild(statement->getCondition());
    dumpChild(statement->getBody());
}

void Dumper::dumpElseCaseStatement(ElseCaseStatement* statement) {
    dumpChild(statement->getBody());
}

void Dumper::dumpBreakStatement(BreakStatement* statement) { }

void Dumper::dumpContinueStatement(ContinueStatement* statement) { }

void Dumper::dumpFallthroughStatement(FallthroughStatement* statement) { }

void Dumper::dumpReturnStatement(ReturnStatement* statement) {
    if (statement->getValue()) {
        dumpChild(statement->getValue());
    }
}

void Dumper::dumpDeferStatement(DeferStatement* statement) {
    dumpChild(statement->getExpression());
}

void Dumper::dumpLoadDeclaration(LoadDeclaration* declaration) {
    printProperty("sourceFilePath", declaration->getSourceFilePath());
}

void Dumper::dumpModuleDeclaration(ModuleDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChildren(declaration->getChildren());
}

void Dumper::dumpEnumerationElementDeclaration(EnumerationElementDeclaration* declaration) {
    printProperty("name", declaration->getName());

    if (declaration->getValue()) {
        dumpChild(declaration->getValue());
    }
}

void Dumper::dumpParameterDeclaration(ParameterDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChild(declaration->getTypeRef());
}

void Dumper::dumpEnumerationDeclaration(EnumerationDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChildren(declaration->getChildren());
}

void Dumper::dumpFunctionDeclaration(FunctionDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChildren(declaration->getParameters());
    dumpChild(declaration->getReturnTypeRef());

    if (declaration->getBody()) {
        dumpChild(declaration->getBody());
    }
}

void Dumper::dumpStructureDeclaration(StructureDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChildren(declaration->getChildren());
}

void Dumper::dumpConstantDeclaration(ConstantDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChild(declaration->getTypeRef());
    dumpChild(declaration->getInitializer());
}

void Dumper::dumpVariableDeclaration(VariableDeclaration* declaration) {
    printProperty("name", declaration->getName());
    dumpChild(declaration->getTypeRef());

    if (declaration->getInitializer()) {
        dumpChild(declaration->getInitializer());
    }
}

void Dumper::dumpUnaryExpression(UnaryExpression* expression) {
    // @Todo print fixity and symbol of operator!
    printProperty("operator", expression->getOperator().getSymbol());
    dumpChild(expression->getRight());
}

void Dumper::dumpBinaryExpression(BinaryExpression* expression) {
    // @Todo print fixity and symbol of operator!
    printProperty("operator", expression->getOperator().getSymbol());
    dumpChild(expression->getLeft());
    dumpChild(expression->getRight());
}

void Dumper::dumpIdentifierExpression(IdentifierExpression* expression) {
    printProperty("identifier", expression->getIdentifier());
}

void Dumper::dumpMemberAccessExpression(MemberAccessExpression* expression) {
    printProperty("memberName", expression->getMemberName());
    dumpChild(expression->getLeft());
}

void Dumper::dumpCallExpression(CallExpression* expression) {
    dumpChild(expression->getCallee());
    dumpChildren(expression->getArguments());
}

void Dumper::dumpNilLiteral(NilLiteral* literal) { }

void Dumper::dumpBoolLiteral(BoolLiteral* literal) {
    if (literal->getValue()) {
        printProperty("value", "true");
    } else {
        printProperty("value", "false");
    }
}

void Dumper::dumpIntLiteral(IntLiteral* literal) {
    printProperty("value", std::to_string(literal->getValue()));
}

void Dumper::dumpFloatLiteral(FloatLiteral* literal) {
    printProperty("value", std::to_string(literal->getValue()));
}

void Dumper::dumpStringLiteral(StringLiteral* literal) {
    printProperty("value", literal->getValue());
}

void Dumper::dumpSwitchStatement(SwitchStatement* statement) {
    dumpChild(statement->getArgument());
    dumpChildren(statement->getChildren());
}

void Dumper::dumpOpaqueTypeRef(OpaqueTypeRef* typeRef) {
    printProperty("name", typeRef->getName());
}

void Dumper::dumpTypeOfTypeRef(TypeOfTypeRef* typeRef) {
    dumpChild(typeRef->getExpression());
}

void Dumper::dumpPointerTypeRef(PointerTypeRef* typeRef) {
    printProperty("depth", std::to_string(typeRef->getDepth()));
    dumpChild(typeRef->getPointeeTypeRef());
}

void Dumper::dumpArrayTypeRef(ArrayTypeRef* typeRef) {
    dumpChild(typeRef->getElementTypeRef());

    if (typeRef->getValue()) {
        dumpChild(typeRef->getValue());
    }
}
