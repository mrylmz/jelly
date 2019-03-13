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

#include "AST/ArrayTypeRef.h"
#include "AST/Associativity.h"
#include "AST/BinaryExpression.h"
#include "AST/BlockStatement.h"
#include "AST/BoolLiteral.h"
#include "AST/BranchStatement.h"
#include "AST/BreakStatement.h"
#include "AST/CallExpression.h"
#include "AST/CaseStatement.h"
#include "AST/ConditionalCaseStatement.h"
#include "AST/ConstantDeclaration.h"
#include "AST/Context.h"
#include "AST/ContinueStatement.h"
#include "AST/ControlStatement.h"
#include "AST/Declaration.h"
#include "AST/DeferStatement.h"
#include "AST/DoStatement.h"
#include "AST/Dumper.h"
#include "AST/ElseCaseStatement.h"
#include "AST/EnumerationDeclaration.h"
#include "AST/EnumerationElementDeclaration.h"
#include "AST/Expression.h"
#include "AST/FallthroughStatement.h"
#include "AST/Fixity.h"
#include "AST/FloatLiteral.h"
#include "AST/FunctionDeclaration.h"
#include "AST/GuardStatement.h"
#include "AST/Identifier.h"
#include "AST/IdentifierExpression.h"
#include "AST/IfStatement.h"
#include "AST/IntLiteral.h"
#include "AST/LoadDeclaration.h"
#include "AST/LoopStatement.h"
#include "AST/MemberAccessExpression.h"
#include "AST/ModuleDeclaration.h"
#include "AST/NamedDeclaration.h"
#include "AST/NilLiteral.h"
#include "AST/Node.h"
#include "AST/OpaqueTypeRef.h"
#include "AST/ParameterDeclaration.h"
#include "AST/PointerTypeRef.h"
#include "AST/ReturnStatement.h"
#include "AST/Statement.h"
#include "AST/StringLiteral.h"
#include "AST/StructureDeclaration.h"
#include "AST/SwitchStatement.h"
#include "AST/Type.h"
#include "AST/TypeDeclaration.h"
#include "AST/TypeOfTypeRef.h"
#include "AST/TypeRef.h"
#include "AST/UnaryExpression.h"
#include "AST/ValueDeclaration.h"
#include "AST/VariableDeclaration.h"
#include "AST/Visitor.h"
#include "AST/WhileStatement.h"
