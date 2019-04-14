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

#pragma once

#include "AST/Node.h"
#include <ostream>
#include <Basic/Basic.h>

namespace jelly {
namespace AST {

    class BlockStatement;
    class GuardStatement;
    class IfStatement;
    class DoStatement;
    class WhileStatement;
    class ConditionalCaseStatement;
    class ElseCaseStatement;
    class BreakStatement;
    class ContinueStatement;
    class FallthroughStatement;
    class ReturnStatement;
    class DeferStatement;
    class LoadDirective;
    class ModuleDeclaration;
    class EnumerationElementDeclaration;
    class ParameterDeclaration;
    class EnumerationDeclaration;
    class FunctionDeclaration;
    class StructureDeclaration;
    class ConstantDeclaration;
    class VariableDeclaration;
    class UnaryExpression;
    class BinaryExpression;
    class IdentifierExpression;
    class MemberAccessExpression;
    class CallExpression;
    class SubscriptExpression;
    class NilLiteral;
    class BoolLiteral;
    class IntLiteral;
    class FloatLiteral;
    class StringLiteral;
    class SwitchStatement;
    class OpaqueTypeRef;
    class TypeOfTypeRef;
    class PointerTypeRef;
    class ArrayTypeRef;

    class Dumper {
        std::ostream& stream;
        uint32_t indentation;

        void printIndentation();
        void printString(StringRef text);

        template <typename ...Ts>
        void printFormatString(const char* format, Ts &&... Vals) {
            std::string string = formatv(format, Vals...);
            printString(string);
        }

        void printProperty(StringRef key, StringRef value);

        template<typename Element>
        void dumpChild(Element* child) {
            indentation += 1;
            dump(child);
            indentation -= 1;
        }

        template<typename Element>
        void dumpChildren(ArrayRef<Element*> children) {
            for (auto child : children) {
                dumpChild(child);
            }
        }

        void dumpKind(Node::Kind kind);
        void dumpBlockStatement(BlockStatement* statement);
        void dumpGuardStatement(GuardStatement* statement);
        void dumpIfStatement(IfStatement* statement);
        void dumpDoStatement(DoStatement* statement);
        void dumpWhileStatement(WhileStatement* statement);
        void dumpConditionalCaseStatement(ConditionalCaseStatement* statement);
        void dumpElseCaseStatement(ElseCaseStatement* statement);
        void dumpBreakStatement(BreakStatement* statement);
        void dumpContinueStatement(ContinueStatement* statement);
        void dumpFallthroughStatement(FallthroughStatement* statement);
        void dumpReturnStatement(ReturnStatement* statement);
        void dumpDeferStatement(DeferStatement* statement);
        void dumpLoadDirective(LoadDirective* directive);
        void dumpModuleDeclaration(ModuleDeclaration* declaration);
        void dumpEnumerationElementDeclaration(EnumerationElementDeclaration* declaration);
        void dumpParameterDeclaration(ParameterDeclaration* declaration);
        void dumpEnumerationDeclaration(EnumerationDeclaration* declaration);
        void dumpFunctionDeclaration(FunctionDeclaration* declaration);
        void dumpStructureDeclaration(StructureDeclaration* declaration);
        void dumpConstantDeclaration(ConstantDeclaration* declaration);
        void dumpVariableDeclaration(VariableDeclaration* declaration);
        void dumpUnaryExpression(UnaryExpression* expression);
        void dumpBinaryExpression(BinaryExpression* expression);
        void dumpIdentifierExpression(IdentifierExpression* expression);
        void dumpMemberAccessExpression(MemberAccessExpression* expression);
        void dumpCallExpression(CallExpression* expression);
        void dumpSubscriptExpression(SubscriptExpression* expression);
        void dumpNilLiteral(NilLiteral* literal);
        void dumpBoolLiteral(BoolLiteral* literal);
        void dumpIntLiteral(IntLiteral* literal);
        void dumpFloatLiteral(FloatLiteral* literal);
        void dumpStringLiteral(StringLiteral* literal);
        void dumpSwitchStatement(SwitchStatement* statement);
        void dumpOpaqueTypeRef(OpaqueTypeRef* typeRef);
        void dumpTypeOfTypeRef(TypeOfTypeRef* typeRef);
        void dumpPointerTypeRef(PointerTypeRef* typeRef);
        void dumpArrayTypeRef(ArrayTypeRef* typeRef);

    public:

        Dumper(std::ostream& stream);

        void dump(Node* node);
    };
}
}
