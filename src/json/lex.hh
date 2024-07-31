#pragma once

#include "String.hh"
#include "Allocator.hh"

namespace json
{

struct Token
{
    enum TYPE : u8
    {
        LBRACE = '{',
        RBRACE = '}',
        LBRACKET = '[',
        RBRACKET = ']',
        QUOTE = '"',
        IDENT = 'I',
        NUMBER = 'N',
        TRUE_ = 'T',
        FALSE_ = 'F',
        NULL_ = 'n',
        ASSIGN = ':',
        COMMA = ',',
        DOT = '.',
        UNHANDLED = 'X',
        EOF_ = '\0',
    } type;
    adt::String svLiteral;
};

struct Lexer
{
    adt::Allocator* _pArena {};
    adt::String _sFile;
    u32 _pos = 0;

    Lexer(adt::Allocator* p) : _pArena(p) {}

    void loadFile(adt::String path);
    void skipWhiteSpace();
    Token number();
    Token stringNoQuotes();
    Token string();
    Token character(enum Token::TYPE type);
    Token next();
};

} /* namespace json */

