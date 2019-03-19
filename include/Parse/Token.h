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

#pragma once

#include <Basic/Basic.h>

namespace jelly {
namespace Parse {

    class Token {
    public:

        enum class Kind : uint8_t {
            Unknown,
            Colon = ':',
            Comma = ',',
            Dot = '.',
            Arrow,
            LeftParenthesis = '(',
            RightParenthesis = ')',
            LeftBracket = '[',
            RightBracket = ']',
            LeftBrace = '{',
            RightBrace = '}',
            Identifier,
            Operator,
            KeywordLoad,
            KeywordFunc,
            KeywordEnum,
            KeywordStruct,
            KeywordVar,
            KeywordLet,
            KeywordBreak,
            KeywordCase,
            KeywordContinue,
            KeywordDefer,
            KeywordDo,
            KeywordElse,
            KeywordFallthrough,
            KeywordGuard,
            KeywordIf,
            KeywordReturn,
            KeywordSwitch,
            KeywordWhile,
            KeywordFalse,
            KeywordNil,
            KeywordTrue,
            KeywordTypeof,
            LiteralInt,
            LiteralFloat,
            LiteralString,
            EndOfFile
        };

    private:

        Kind kind;
        uint32_t line;
        uint32_t column;
        jelly::StringRef text;

    public:

        Token();

        Kind getKind() const;
        void setKind(Kind kind);

        uint32_t getLine() const;
        void setLine(uint32_t line);

        uint32_t getColumn() const;
        void setColumn(uint32_t column);

        jelly::StringRef getText() const;
        void setText(jelly::StringRef text);

        bool is(Kind kind) const;

        template <typename ...T>
        bool is(Kind kind1, Kind kind2, T... kinds) const {
            if (is(kind1)) { return true; }
            return is(kind2, kinds...);
        }
    };
}
}

namespace llvm {
    template<> struct format_provider<jelly::Parse::Token::Kind> {
        static void format(const jelly::Parse::Token::Kind &value, raw_ostream &Stream, StringRef Style) {
            switch (value) {
                case jelly::Parse::Token::Kind::Unknown:
                    Stream << "Unknown";
                    break;

                case jelly::Parse::Token::Kind::Colon:
                    Stream << "':'";
                    break;

                case jelly::Parse::Token::Kind::Comma:
                    Stream << "','";
                    break;

                case jelly::Parse::Token::Kind::Dot:
                    Stream << "'.'";
                    break;

                case jelly::Parse::Token::Kind::Arrow:
                    Stream << "'->'";
                    break;

                case jelly::Parse::Token::Kind::LeftParenthesis:
                    Stream << "'('";
                    break;

                case jelly::Parse::Token::Kind::RightParenthesis:
                    Stream << "')'";
                    break;

                case jelly::Parse::Token::Kind::LeftBracket:
                    Stream << "'['";
                    break;

                case jelly::Parse::Token::Kind::RightBracket:
                    Stream << "']'";
                    break;

                case jelly::Parse::Token::Kind::LeftBrace:
                    Stream << "'{'";
                    break;

                case jelly::Parse::Token::Kind::RightBrace:
                    Stream << "'}'";
                    break;

                case jelly::Parse::Token::Kind::Identifier:
                    Stream << "Identifier";
                    break;

                case jelly::Parse::Token::Kind::Operator:
                    Stream << "Operator";
                    break;

                case jelly::Parse::Token::Kind::KeywordLoad:
                    Stream << "'#load'";
                    break;

                case jelly::Parse::Token::Kind::KeywordFunc:
                    Stream << "'func'";
                    break;

                case jelly::Parse::Token::Kind::KeywordEnum:
                    Stream << "'enum'";
                    break;

                case jelly::Parse::Token::Kind::KeywordStruct:
                    Stream << "'struct'";
                    break;

                case jelly::Parse::Token::Kind::KeywordVar:
                    Stream << "'var'";
                    break;

                case jelly::Parse::Token::Kind::KeywordLet:
                    Stream << "'let'";
                    break;

                case jelly::Parse::Token::Kind::KeywordBreak:
                    Stream << "'break'";
                    break;

                case jelly::Parse::Token::Kind::KeywordCase:
                    Stream << "'case'";
                    break;

                case jelly::Parse::Token::Kind::KeywordContinue:
                    Stream << "'continue'";
                    break;

                case jelly::Parse::Token::Kind::KeywordDefer:
                    Stream << "'defer'";
                    break;

                case jelly::Parse::Token::Kind::KeywordDo:
                    Stream << "'do'";
                    break;

                case jelly::Parse::Token::Kind::KeywordElse:
                    Stream << "'else'";
                    break;

                case jelly::Parse::Token::Kind::KeywordFallthrough:
                    Stream << "'fallthrough'";
                    break;

                case jelly::Parse::Token::Kind::KeywordGuard:
                    Stream << "'guard'";
                    break;

                case jelly::Parse::Token::Kind::KeywordIf:
                    Stream << "'if'";
                    break;

                case jelly::Parse::Token::Kind::KeywordReturn:
                    Stream << "'return'";
                    break;

                case jelly::Parse::Token::Kind::KeywordSwitch:
                    Stream << "'switch'";
                    break;

                case jelly::Parse::Token::Kind::KeywordWhile:
                    Stream << "'while'";
                    break;

                case jelly::Parse::Token::Kind::KeywordFalse:
                    Stream << "'false'";
                    break;

                case jelly::Parse::Token::Kind::KeywordNil:
                    Stream << "'nil'";
                    break;

                case jelly::Parse::Token::Kind::KeywordTrue:
                    Stream << "'true'";
                    break;

                case jelly::Parse::Token::Kind::KeywordTypeof:
                    Stream << "'typeof'";
                    break;

                case jelly::Parse::Token::Kind::LiteralInt:
                    Stream << "Integer";
                    break;

                case jelly::Parse::Token::Kind::LiteralFloat:
                    Stream << "Float";
                    break;

                case jelly::Parse::Token::Kind::LiteralString:
                    Stream << "String";
                    break;

                case jelly::Parse::Token::Kind::EndOfFile:
                    Stream << "'EOF'";
                    break;

            }
        }
    };

    template<> struct format_provider<jelly::Parse::Token> {
        static void format(const jelly::Parse::Token &value, raw_ostream &Stream, StringRef Style) {
            switch (value.getKind()) {
                case jelly::Parse::Token::Kind::Identifier:
                    Stream << "Identifier = '";
                    Stream << value.getText();
                    Stream << "'";
                    break;

                case jelly::Parse::Token::Kind::Operator:
                    Stream << "Operator = '";
                    Stream << value.getText();
                    Stream << "'";
                    break;

                case jelly::Parse::Token::Kind::LiteralInt:
                    Stream << "Integer = '";
                    Stream << value.getText();
                    Stream << "'";
                    break;

                case jelly::Parse::Token::Kind::LiteralFloat:
                    Stream << "Float = '";
                    Stream << value.getText();
                    Stream << "'";
                    break;

                case jelly::Parse::Token::Kind::LiteralString:
                    Stream << "String = '";
                    Stream << value.getText();
                    Stream << "'";
                    break;

                default:
                    format_provider<jelly::Parse::Token::Kind>::format(value.getKind(), Stream, Style);
                    break;
            }
        }
    };
}
