#include "parser.hh"
#include "utils.hh"
#include "logs.hh"

namespace json
{

void
Parser::load(adt::String path)
{
    _sName = path;
    _l.loadFile(path);

    _tCurr = _l.next();
    _tNext = _l.next();

    if ((_tCurr.type != Token::LBRACE) && (_tCurr.type != Token::LBRACKET))
    {
        CERR("wrong first token\n");
        exit(2);
    }

    _pHead = (Object*)(_pArena->alloc(1, sizeof(Object)));
}

void
Parser::parse()
{
    parseNode(_pHead);
}

void
Parser::expect(enum Token::TYPE t, adt::String svFile, int line)
{
    if (_tCurr.type != t)
    {
        CERR("('%.*s', at %d): (%.*s): unexpected token: expected: '%c', got '%c'\n",
             svFile._size, svFile._pData, line, _sName._size, _sName._pData, char(t), char(_tCurr.type));
        exit(2);
    }
}

void
Parser::next()
{
    _tCurr = _tNext;
    _tNext = _l.next();
}

void
Parser::parseNode(Object* pNode)
{
    switch (_tCurr.type)
    {
        default:
            next();
            break;

        case Token::IDENT:
            parseIdent(&pNode->tagVal);
            break;

        case Token::NUMBER:
            parseNumber(&pNode->tagVal);
            break;

        case Token::LBRACE:
            next(); /* skip brace */
            parseObject(pNode);
            break;

        case Token::LBRACKET:
            next(); /* skip bracket */
            parseArray(pNode);
            break;

        case Token::NULL_:
            parseNull(&pNode->tagVal);
            break;

        case Token::TRUE_:
        case Token::FALSE_:
            parseBool(&pNode->tagVal);
            break;
    }
}

void
Parser::parseIdent(TagVal* pTV)
{
    *pTV = {.tag = TAG::STRING, .val {.sv = _tCurr.svLiteral}};
    next();
}

void
Parser::parseNumber(TagVal* pTV)
{
    bool bReal = adt::findLastOf(_tCurr.svLiteral, '.') != adt::NPOS;

    if (bReal)
        *pTV = {.tag = TAG::DOUBLE, .val = {.d = atof(_tCurr.svLiteral.data())}};
    else
        *pTV = TagVal{.tag = TAG::LONG, .val = {.l = atol(_tCurr.svLiteral.data())}};

    next();
}

void
Parser::parseObject(Object* pNode)
{
    pNode->tagVal.tag = TAG::OBJECT;
    pNode->tagVal.val.o = adt::Array<Object>(_pArena, 8);
    auto& aObjs = getObject(pNode);

    for (; _tCurr.type != Token::RBRACE; next())
    {
        expect(Token::IDENT, __FILE__, __LINE__);
        Object ob {.svKey = _tCurr.svLiteral, .tagVal = {}};
        aObjs.push(ob);

        /* skip identifier and ':' */
        next();
        expect(Token::ASSIGN, __FILE__, __LINE__);
        next();

        parseNode(&aObjs.back());

        if (_tCurr.type != Token::COMMA)
        {
            next();
            break;
        }
    }

    if (aObjs.empty())
        next();
}

void
Parser::parseArray(Object* pNode)
{
    pNode->tagVal.tag = TAG::ARRAY;
    pNode->tagVal.val.a = adt::Array<Object>(_pArena, 8);
    auto& aTVs = getArray(pNode);

    /* collect each key/value pair inside array */
    for (; _tCurr.type != Token::RBRACKET; next())
    {
        aTVs.push({});

        switch (_tCurr.type)
        {
            default:
            case Token::IDENT:
                parseIdent(&aTVs.back().tagVal);
                break;

            case Token::NULL_:
                parseNull(&aTVs.back().tagVal);
                break;

            case Token::TRUE_:
            case Token::FALSE_:
                parseBool(&aTVs.back().tagVal);
                break;

            case Token::NUMBER:
                parseNumber(&aTVs.back().tagVal);
                break;

            case Token::LBRACE:
                next();
                parseObject(&aTVs.back());
                break;
        }

        if (_tCurr.type != Token::COMMA)
        {
            next();
            break;
        }
    }

    if (aTVs.empty())
        next();
}

void
Parser::parseNull(TagVal* pTV)
{
    *pTV = {.tag = TAG::NULL_, .val = {nullptr}};
    next();
}

void
Parser::parseBool(TagVal* pTV)
{
    bool b = _tCurr.type == Token::TRUE_ ? true : false;
    *pTV = {.tag = TAG::BOOL, .val = {.b = b}};
    next();
}

void
Parser::print()
{
    printNode(_pHead, "");
    COUT("\n");
}

void
Parser::printNode(Object* pNode, adt::String svEnd)
{
    adt::String key = pNode->svKey;

    switch (pNode->tagVal.tag)
    {
        default:
            break;

        case TAG::OBJECT:
            {
                auto& obj = getObject(pNode);
                adt::String q0, q1, objName0, objName1;

                if (key._size == 0)
                {
                    q0 = q1 = objName1 = objName0 = "";
                }
                else
                {
                    objName0 = key;
                    objName1 = ": ";
                    q1 = q0 = "\"";
                }

                COUT("%.*s%.*s%.*s%.*s{\n", q0._size, q0._pData, objName0._size, objName0._pData, q1._size, q1._pData, objName1._size, objName1._pData);
                for (u32 i = 0; i < obj._size; i++)
                {
                    adt::String slE = (i == obj._size - 1) ? "\n" : ",\n";
                    printNode(&obj[i], slE);
                }
                COUT("}%.*s", svEnd._size, svEnd._pData);
            }
            break;

        case TAG::ARRAY:
            {
                auto& arr = getArray(pNode);
                adt::String q0, q1, arrName0, arrName1;

                if (key._size == 0)
                {
                    q0 =  q1 = arrName1 = arrName0 = "";
                }
                else
                {
                    arrName0 = key;
                    arrName1 = ": ";
                    q1 = q0 = "\"";
                }

                COUT("%.*s%.*s%.*s%.*s[", q0._size, q0._pData, arrName0._size, arrName0._pData, q1._size, q1._pData, arrName1._size, arrName1._pData);
                for (u32 i = 0; i < arr._size; i++)
                {
                    adt::String slE = (i == arr._size - 1) ? "\n" : ",\n";

                    switch (arr[i].tagVal.tag)
                    {
                        default:
                        case TAG::STRING:
                            {
                                adt::String sl = getString(&arr[i]);
                                COUT("\"%.*s\"%.*s", sl._size, sl._pData, slE._size, slE._pData);
                            }
                            break;

                        case TAG::NULL_:
                                COUT("%s%.*s", "null", slE._size, slE._pData);
                            break;

                        case TAG::LONG:
                            {
                                long num = getLong(&arr[i]);
                                COUT("%ld%.*s", num, slE._size, slE._pData);
                            }
                            break;

                        case TAG::DOUBLE:
                            {
                                double dnum = getDouble(&arr[i]);
                                COUT("%lf%.*s", dnum, slE._size, slE._pData);
                            }
                            break;

                        case TAG::BOOL:
                            {
                                bool b = getBool(&arr[i]);
                                COUT("%s%.*s", b ? "true" : "false", slE._size, slE._pData);
                            }
                            break;

                        case TAG::OBJECT:
                                printNode(&arr[i], slE);
                            break;
                    }
                }
                COUT("]%.*s", (int)svEnd._size, svEnd._pData);
            }
            break;

        case TAG::DOUBLE:
            {
                /* TODO: add some sort formatting for floats */
                double f = getDouble(pNode);
                COUT("\"%.*s\": %lf%.*s", key._size, key._pData, f, svEnd._size, svEnd._pData);
            }
            break;

        case TAG::LONG:
            {
                long i = getLong(pNode);
                COUT("\"%.*s\": %ld%.*s", key._size, key._pData, i, svEnd._size, svEnd._pData);
            }
            break;

        case TAG::NULL_:
                COUT("\"%.*s\": %s%.*s", key._size, key._pData, "null", svEnd._size, svEnd._pData);
            break;

        case TAG::STRING:
            {
                adt::String sl = getString(pNode);
                COUT("\"%.*s\": \"%.*s\"%.*s", key._size, key._pData, sl._size, sl._pData, svEnd._size, svEnd._pData);
            }
            break;

        case TAG::BOOL:
            {
                bool b = getBool(pNode);
                COUT("\"%.*s\": %s%.*s", key._size, key._pData, b ? "true" : "false", svEnd._size, svEnd._pData);
            }
            break;
    }
}

} /* namespace json */
