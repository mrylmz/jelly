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

        bool is(uint16_t kind) const;

        bool is(Kind kind) const;

        template <typename ...T>
        bool is(uint16_t kind1, uint16_t kind2, T... kinds) const {
            if (is(kind1)) { return true; }
            return is(kind2, kinds...);
        }

        template <typename ...T>
        bool is(Kind kind1, Kind kind2, T... kinds) const {
            if (is(kind1)) { return true; }
            return is(kind2, kinds...);
        }
    };
}
}
