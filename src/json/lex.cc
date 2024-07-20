#include <ctype.h>

#include "lex.hh"
#include "file.hh"
#include "logs.hh"

namespace json
{

void
Lexer::loadFile(adt::String path)
{
    this->sFile = adt::loadFile(this->pArena, path);
}

void
Lexer::skipWhiteSpace()
{
    auto oneOf = [](char c) -> bool {
        const char skipChars[] = " \t\n\r";
        for (auto& s : skipChars)
            if (c == s) return true;

        return false;
    };

    while (this->pos < this->sFile.size && oneOf(this->sFile[this->pos]))
        this->pos++;
}

Token
Lexer::number()
{
    Token r {};
    u32 start = this->pos;
    u32 i = start;

    while (isxdigit(this->sFile[i])       ||
                    this->sFile[i] == '.' ||
                    this->sFile[i] == '-' ||
                    this->sFile[i] == '+')
    {
        i++;
    }

    r.type = Token::NUMBER;
    r.svLiteral = {&this->sFile[start], i - start};
    
    this->pos = i - 1;
    return r;
}

Token
Lexer::stringNoQuotes()
{
    Token r {};

    u32 start = this->pos;
    u32 i = start;

    while (isalpha(this->sFile[i]))
        i++;

    r.svLiteral = {&this->sFile[start], i - start};

    if ("null" == r.svLiteral)
        r.type = Token::NULL_;
    else if ("false" == r.svLiteral)
        r.type = Token::FALSE;
    else if ("true" == r.svLiteral)
        r.type = Token::TRUE;
    else
        r.type = Token::IDENT;

    this->pos = i - 1;
    return r;
}

Token
Lexer::string()
{
    Token r {};

    u32 start = this->pos;
    u32 i = start + 1;
    bool bEsc = false;

    while (this->sFile[i])
    {
        switch (this->sFile[i])
        {
            default:
                if (bEsc)
                    bEsc = false;
                break;

            case Token::EOF_:
                CERR("unterminated string\n");
                exit(1);

            case '\n':
                CERR("Unexpected newline within string");
                exit(1);

            case '\\':
                bEsc = !bEsc;
                break;

            case '"':
                if (!bEsc)
                    goto done;
                else
                    bEsc = false;
                break;
        }

        i++;
    }

done:

    r.type = Token::IDENT;
    r.svLiteral = {&this->sFile[start + 1], (i - start) - 1};

    this->pos = i;
    return r;
}

Token
Lexer::character(enum Token::TYPE type)
{
    return {
        .type = type,
        .svLiteral = {&this->sFile[pos], 1}
    };
}

Token
Lexer::next()
{
    Token r {};

    if (this->pos >= this->sFile.size)
            return r;

    skipWhiteSpace();

    switch (this->sFile[this->pos])
    {
        default:
            /* solves bools and nulls */
            r = stringNoQuotes();
            break;

        case '-':
        case '+':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            r = number();
            break;

        case Token::QUOTE:
            r = string();
            break;

        case Token::COMMA:
            r = character(Token::COMMA);
            break;

        case Token::ASSIGN:
            r = character(Token::ASSIGN);
            break;

        case Token::LBRACE:
            r = character(Token::LBRACE);
            break;

        case Token::RBRACE:
            r = character(Token::RBRACE);
            break;

        case Token::RBRACKET:
            r = character(Token::RBRACKET);
            break;

        case Token::LBRACKET:
            r = character(Token::LBRACKET);
            break;

        case Token::EOF_:
            r.type = Token::EOF_;
            break;
    }

    this->pos++;
    return r;
}

} /* namespace json */
