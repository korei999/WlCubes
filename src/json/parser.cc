#include "parser.hh"
#include "utils.hh"
#include "logs.hh"

namespace json
{

void
Parser::load(adt::String path)
{
    this->sName = path;
    this->l.loadFile(path);

    this->tCurr = this->l.next();
    this->tNext = this->l.next();

    if ((this->tCurr.type != Token::LBRACE) && (this->tCurr.type != Token::LBRACKET))
    {
        CERR("wrong first token\n");
        exit(2);
    }

    this->pHead = static_cast<Object*>(this->pArena->alloc(1, sizeof(Object)));
}

void
Parser::parse()
{
    this->parseNode(this->pHead);
}

void
Parser::expect(enum Token::TYPE t, adt::String svFile, int line)
{
    if (this->tCurr.type != t)
    {
        CERR("('%.*s', at %d): (%.*s): unexpected token: expected: '%c', got '%c'\n",
             (int)svFile.size, svFile.pData, line, (int)this->sName.size, this->sName.pData, static_cast<char>(t), static_cast<char>(this->tCurr.type));
        exit(2);
    }
}

void
Parser::next()
{
    this->tCurr = this->tNext;
    this->tNext = this->l.next();
}

void
Parser::parseNode(Object* pNode)
{
    switch (this->tCurr.type)
    {
        default:
            this->next();
            break;

        case Token::IDENT:
            this->parseIdent(&pNode->tagVal);
            break;

        case Token::NUMBER:
            this->parseNumber(&pNode->tagVal);
            break;

        case Token::LBRACE:
            this->next(); /* skip brace */
            parseObject(pNode);
            break;

        case Token::LBRACKET:
            this->next(); /* skip bracket */
            this->parseArray(pNode);
            break;

        case Token::NULL_:
            this->parseNull(&pNode->tagVal);
            break;

        case Token::TRUE_:
        case Token::FALSE_:
            this->parseBool(&pNode->tagVal);
            break;
    }
}

void
Parser::parseIdent(TagVal* pTV)
{
    *pTV = {.tag = TAG::STRING, .val {.sv = this->tCurr.svLiteral}};
    this->next();
}

void
Parser::parseNumber(TagVal* pTV)
{
    bool bReal = adt::findLastOf(this->tCurr.svLiteral, '.') != NPOS;

    if (bReal)
        *pTV = {.tag = TAG::DOUBLE, .val = {.d = atof(this->tCurr.svLiteral.data())}};
    else
        *pTV = TagVal{.tag = TAG::LONG, .val = {.l = atol(this->tCurr.svLiteral.data())}};

    this->next();
}

void
Parser::parseObject(Object* pNode)
{
    pNode->tagVal.tag = TAG::OBJECT;
    pNode->tagVal.val.o = adt::Array<Object>(this->pArena, 8);
    auto& aObjs = getObject(pNode);

    for (; this->tCurr.type != Token::RBRACE; this->next())
    {
        this->expect(Token::IDENT, __FILE__, __LINE__);
        aObjs.push({.svKey = this->tCurr.svLiteral, .tagVal = {}});

        /* skip identifier and ':' */
        this->next();
        this->expect(Token::ASSIGN, __FILE__, __LINE__);
        this->next();

        parseNode(&aObjs.back());

        if (this->tCurr.type != Token::COMMA)
        {
            this->next();
            break;
        }
    }

    if (aObjs.empty())
        this->next();
}

void
Parser::parseArray(Object* pNode)
{
    pNode->tagVal.tag = TAG::ARRAY;
    pNode->tagVal.val.a = adt::Array<Object>(this->pArena, 8);
    auto& aTVs = getArray(pNode);

    /* collect each key/value pair inside array */
    for (; this->tCurr.type != Token::RBRACKET; this->next())
    {
        aTVs.push({});

        switch (this->tCurr.type)
        {
            default:
            case Token::IDENT:
                this->parseIdent(&aTVs.back().tagVal);
                break;

            case Token::NULL_:
                this->parseNull(&aTVs.back().tagVal);
                break;

            case Token::TRUE_:
            case Token::FALSE_:
                this->parseBool(&aTVs.back().tagVal);
                break;

            case Token::NUMBER:
                this->parseNumber(&aTVs.back().tagVal);
                break;

            case Token::LBRACE:
                this->next();
                this->parseObject(&aTVs.back());
                break;
        }

        if (this->tCurr.type != Token::COMMA)
        {
            this->next();
            break;
        }
    }

    if (aTVs.empty())
        this->next();
}

void
Parser::parseNull(TagVal* pTV)
{
    *pTV = {.tag = TAG::NULL_, .val = {nullptr}};
    this->next();
}

void
Parser::parseBool(TagVal* pTV)
{
    bool b = this->tCurr.type == Token::TRUE_ ? true : false;
    *pTV = {.tag = TAG::BOOL, .val = {.b = b}};
    this->next();
}

void
Parser::print()
{
    this->printNode(this->pHead, "");
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

                if (key.size == 0)
                {
                    q0 = q1 = objName1 = objName0 = "";
                }
                else
                {
                    objName0 = key;
                    objName1 = ": ";
                    q1 = q0 = "\"";
                }

                COUT("%.*s%.*s%.*s%.*s{\n", (int)q0.size, q0.pData, (int)objName0.size, objName0.pData, (int)q1.size, q1.pData, (int)objName1.size, objName1.pData);
                for (size_t i = 0; i < obj.size; i++)
                {
                    adt::String slE = (i == obj.size - 1) ? "\n" : ",\n";
                    this->printNode(&obj[i], slE);
                }
                COUT("}%.*s", (int)svEnd.size, svEnd.pData);
            }
            break;

        case TAG::ARRAY:
            {
                auto& arr = getArray(pNode);
                adt::String q0, q1, arrName0, arrName1;

                if (key.size == 0)
                {
                    q0 =  q1 = arrName1 = arrName0 = "";
                }
                else
                {
                    arrName0 = key;
                    arrName1 = ": ";
                    q1 = q0 = "\"";
                }

                COUT("%.*s%.*s%.*s%.*s[", (int)q0.size, q0.pData, (int)arrName0.size, arrName0.pData, (int)q1.size, q1.pData, (int)arrName1.size, arrName1.pData);
                for (size_t i = 0; i < arr.size; i++)
                {
                    adt::String slE = (i == arr.size - 1) ? "\n" : ",\n";

                    switch (arr[i].tagVal.tag)
                    {
                        default:
                        case TAG::STRING:
                            {
                                adt::String sl = getString(&arr[i]);
                                COUT("\"%.*s\"%.*s", (int)sl.size, sl.pData, (int)slE.size, slE.pData);
                            }
                            break;

                        case TAG::NULL_:
                                COUT("%s%.*s", "null", (int)slE.size, slE.pData);
                            break;

                        case TAG::LONG:
                            {
                                long num = getLong(&arr[i]);
                                COUT("%ld%.*s", num, (int)slE.size, slE.pData);
                            }
                            break;

                        case TAG::DOUBLE:
                            {
                                double dnum = getDouble(&arr[i]);
                                COUT("%lf%.*s", dnum, (int)slE.size, slE.pData);
                            }
                            break;

                        case TAG::BOOL:
                            {
                                bool b = getBool(&arr[i]);
                                COUT("%s%.*s", b ? "true" : "false", (int)slE.size, slE.pData);
                            }
                            break;

                        case TAG::OBJECT:
                                this->printNode(&arr[i], slE);
                            break;
                    }
                }
                COUT("]%.*s", (int)svEnd.size, svEnd.pData);
            }
            break;

        case TAG::DOUBLE:
            {
                /* TODO: add some sort formatting for floats */
                double f = getDouble(pNode);
                COUT("\"%.*s\": %lf%.*s", (int)key.size, key.pData, f, (int)svEnd.size, svEnd.pData);
            }
            break;

        case TAG::LONG:
            {
                long i = getLong(pNode);
                COUT("\"%.*s\": %ld%.*s", (int)key.size, key.pData, i, (int)svEnd.size, svEnd.pData);
            }
            break;

        case TAG::NULL_:
                COUT("\"%.*s\": %s%.*s", (int)key.size, key.pData, "null", (int)svEnd.size, svEnd.pData);
            break;

        case TAG::STRING:
            {
                adt::String sl = getString(pNode);
                COUT("\"%.*s\": \"%.*s\"%.*s", (int)key.size, key.pData, (int)sl.size, sl.pData, (int)svEnd.size, svEnd.pData);
            }
            break;

        case TAG::BOOL:
            {
                bool b = getBool(pNode);
                COUT("\"%.*s\": %s%.*s", (int)key.size, key.pData, b ? "true" : "false", (int)svEnd.size, svEnd.pData);
            }
            break;
    }
}

} /* namespace json */
