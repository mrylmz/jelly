#ifndef __JELLY_LEXER__
#define __JELLY_LEXER__

#include <JellyCore/Allocator.h>
#include <JellyCore/Base.h>
#include <JellyCore/SourceRange.h>
#include <JellyCore/String.h>

JELLY_EXTERN_C_BEGIN

enum _TokenKind {
    TokenKindUnknown,
    TokenKindError,
    TokenKindSlash,
    TokenKindSlashEquals,
    TokenKindEqualsSign,
    TokenKindEqualsEqualsSign,
    TokenKindMinusSign,
    TokenKindMinusEqualsSign,
    TokenKindPlusSign,
    TokenKindPlusEquals,
    TokenKindExclamationMark,
    TokenKindExclamationMarkEqualsSign,
    TokenKindAsterisk,
    TokenKindAsteriskEquals,
    TokenKindPercentSign,
    TokenKindPercentEquals,
    TokenKindDot,
    TokenKindLessThan,
    TokenKindLessThanLessThan,
    TokenKindLessThanLessThanEquals,
    TokenKindLessThanEqualsSign,
    TokenKindGreaterThan,
    TokenKindGreaterThanGreaterThan,
    TokenKindGreaterThanGreaterThanEquals,
    TokenKindGreaterThanEqualsSign,
    TokenKindAmpersand,
    TokenKindAmpersandAmpersand,
    TokenKindAmpersandEquals,
    TokenKindPipe,
    TokenKindPipePipe,
    TokenKindPipeEquals,
    TokenKindCircumflex,
    TokenKindCircumflexEquals,
    TokenKindLeftParenthesis,
    TokenKindRightParenthesis,
    TokenKindColon,
    TokenKindLeftBracket,
    TokenKindRightBracket,
    TokenKindLeftCurlyBracket,
    TokenKindRightCurlyBracket,
    TokenKindComma,
    TokenKindTilde,
    TokenKindArrow,
    TokenKindIdentifier,
    TokenKindKeywordIs,
    TokenKindKeywordAs,
    TokenKindKeywordIf,
    TokenKindKeywordElse,
    TokenKindKeywordWhile,
    TokenKindKeywordDo,
    TokenKindKeywordCase,
    TokenKindKeywordSwitch,
    TokenKindKeywordBreak,
    TokenKindKeywordContinue,
    TokenKindKeywordFallthrough,
    TokenKindKeywordReturn,
    TokenKindKeywordNil,
    TokenKindKeywordTrue,
    TokenKindKeywordFalse,
    TokenKindKeywordEnum,
    TokenKindKeywordFunc,
    TokenKindKeywordStruct,
    TokenKindKeywordVar,
    TokenKindKeywordVoid,
    TokenKindKeywordBool,
    TokenKindKeywordInt8,
    TokenKindKeywordInt16,
    TokenKindKeywordInt32,
    TokenKindKeywordInt64,
    TokenKindKeywordInt128,
    TokenKindKeywordInt,
    TokenKindKeywordUInt8,
    TokenKindKeywordUInt16,
    TokenKindKeywordUInt32,
    TokenKindKeywordUInt64,
    TokenKindKeywordUInt128,
    TokenKindKeywordUInt,
    TokenKindKeywordFloat16,
    TokenKindKeywordFloat32,
    TokenKindKeywordFloat64,
    TokenKindKeywordFloat128,
    TokenKindKeywordFloat,
    TokenKindDirectiveLoad,
    TokenKindLiteralString,
    TokenKindLiteralInt,
    TokenKindLiteralFloat,
    TokenKindEndOfFile,
};
typedef enum _TokenKind TokenKind;

struct _Token {
    TokenKind kind;
    SourceRange location;

    Index line;
    Index column;
    SourceRange leadingTrivia;
    SourceRange trailingTrivia;

    union {
        Bool boolValue;
        UInt64 intValue;
        Float64 floatValue;
        StringRef stringValue;
    };
};
typedef struct _Token Token;

typedef struct _Lexer *LexerRef;
typedef struct _LexerState *LexerStateRef;

LexerRef LexerCreate(AllocatorRef allocator, StringRef buffer);

void LexerDestroy(LexerRef lexer);

LexerStateRef LexerGetState(LexerRef lexer);
void LexerSetState(LexerRef lexer, LexerStateRef state);

void LexerPeekToken(LexerRef lexer, Token *token);
void LexerNextToken(LexerRef lexer, Token *token);

JELLY_EXTERN_C_END

#endif
