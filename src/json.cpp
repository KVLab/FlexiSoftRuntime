#include "json.h"
#include <sstream>
#include <cctype>
#include <cstring>

static void StripUtf8Bom(std::string& data)
{
    if (data.size() < 3)
        return;

    if ((unsigned char)data[0] == 0xEF &&
        (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF)
    {
        data.erase(0, 3);
    }
}

static int HexDigitValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');

    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');

    return -1;
}

static bool AppendUtf8(std::string& out, unsigned long cp)
{
    if (cp <= 0x7FUL)
    {
        out += (char)cp;
        return true;
    }

    if (cp <= 0x7FFUL)
    {
        out += (char)(0xC0 | ((cp >> 6) & 0x1F));
        out += (char)(0x80 | (cp & 0x3F));
        return true;
    }

    if (cp <= 0xFFFFUL)
    {
        /*
            UTF-16 surrogate code units are not valid Unicode scalar values.
            Valid surrogate pairs are combined before this function is called.
        */
        if (cp >= 0xD800UL && cp <= 0xDFFFUL)
            return false;

        out += (char)(0xE0 | ((cp >> 12) & 0x0F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
        return true;
    }

    if (cp <= 0x10FFFFUL)
    {
        out += (char)(0xF0 | ((cp >> 18) & 0x07));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
        return true;
    }

    return false;
}

std::string JsonReadFile(const char* path)
{
    if (!path || !path[0])
        return "";

    std::wstring wpath = s2ws(path);

    FILE* f = _wfopen(wpath.c_str(), L"rb");

    if (!f)
        return "";

    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return "";
    }

    long size = ftell(f);

    if (size <= 0)
    {
        fclose(f);
        return "";
    }

    if (fseek(f, 0, SEEK_SET) != 0)
    {
        fclose(f);
        return "";
    }

    std::string data;
    data.resize((size_t)size);

    size_t readBytes = fread(&data[0], 1, (size_t)size, f);

    fclose(f);

    if (readBytes != (size_t)size)
        return "";

    StripUtf8Bom(data);

    return data;
}

class Parser
{
public:
    Parser(const std::string& x)
        : b(x.c_str()), p(x.c_str()), e(x.c_str() + x.size())
    {
    }

    bool parse(JVal& out, std::string& err)
    {
        skip();

        if (!val(out))
        {
            err = "invalid JSON near offset " + std::to_string((long long)(p0()));
            return false;
        }

        skip();

        if (p != e)
        {
            err = "trailing data in JSON";
            return false;
        }

        return true;
    }

private:
    const char* b;
    const char* p;
    const char* e;

    long long p0() const
    {
        return (long long)(p - b);
    }

    void skip()
    {
        while (p < e && isspace((unsigned char)*p))
            p++;
    }

    bool eat(char c)
    {
        skip();

        if (p < e && *p == c)
        {
            p++;
            return true;
        }

        return false;
    }

    bool val(JVal& v)
    {
        skip();

        if (p >= e)
            return false;

        if (*p == '{')
            return obj(v);

        if (*p == '[')
            return arr(v);

        if (*p == '"')
        {
            v.t = JVal::STR;
            return str(v.s);
        }

        if (*p == 't' && e - p >= 4 && strncmp(p, "true", 4) == 0)
        {
            p += 4;
            v.t = JVal::BOOL;
            v.b = true;
            return true;
        }

        if (*p == 'f' && e - p >= 5 && strncmp(p, "false", 5) == 0)
        {
            p += 5;
            v.t = JVal::BOOL;
            v.b = false;
            return true;
        }

        if (*p == 'n' && e - p >= 4 && strncmp(p, "null", 4) == 0)
        {
            p += 4;
            v.t = JVal::NIL;
            return true;
        }

        return num(v);
    }

    bool hex4(unsigned long& cp)
    {
        if (e - p < 4)
            return false;

        cp = 0;

        for (int i = 0; i < 4; i++)
        {
            int v = HexDigitValue(*p++);

            if (v < 0)
                return false;

            cp = (cp << 4) | (unsigned long)v;
        }

        return true;
    }

    bool unicodeEscape(std::string& out)
    {
        unsigned long cp = 0;

        if (!hex4(cp))
            return false;

        /*
            High surrogate: require a following low surrogate encoded as \uXXXX.
            Example:
                \uD83D\uDE00 -> U+1F600
        */
        if (cp >= 0xD800UL && cp <= 0xDBFFUL)
        {
            if (e - p < 6)
                return false;

            if (p[0] != '\\' || p[1] != 'u')
                return false;

            p += 2;

            unsigned long lo = 0;

            if (!hex4(lo))
                return false;

            if (lo < 0xDC00UL || lo > 0xDFFFUL)
                return false;

            cp =
                0x10000UL +
                (((cp - 0xD800UL) << 10) |
                    (lo - 0xDC00UL));
        }
        else if (cp >= 0xDC00UL && cp <= 0xDFFFUL)
        {
            /*
                Isolated low surrogate is invalid.
            */
            return false;
        }

        return AppendUtf8(out, cp);
    }

    bool str(std::string& out)
    {
        if (!eat('"'))
            return false;

        out.clear();

        while (p < e)
        {
            char c = *p++;

            if (c == '"')
                return true;

            if (c == '\\')
            {
                if (p >= e)
                    return false;

                char q = *p++;

                switch (q)
                {
                case '"':  out += '"';  break;
                case '\\': out += '\\'; break;
                case '/':  out += '/';  break;
                case 'b':  out += '\b'; break;
                case 'f':  out += '\f'; break;
                case 'n':  out += '\n'; break;
                case 'r':  out += '\r'; break;
                case 't':  out += '\t'; break;

                case 'u':
                    if (!unicodeEscape(out))
                        return false;
                    break;

                default:
                    return false;
                }
            }
            else
            {
                out += c;
            }
        }

        return false;
    }

    bool num(JVal& v)
    {
        skip();

        const char* s = p;

        if (p < e && (*p == '-' || *p == '+'))
            p++;

        while (p < e && isdigit((unsigned char)*p))
            p++;

        if (p < e && *p == '.')
        {
            p++;

            while (p < e && isdigit((unsigned char)*p))
                p++;
        }

        if (s == p)
            return false;

        char* end = NULL;
        v.n = strtod(s, &end);

        if (end != p)
            return false;

        v.t = JVal::NUM;
        return true;
    }

    bool obj(JVal& v)
    {
        if (!eat('{'))
            return false;

        v.t = JVal::OBJ;
        skip();

        if (eat('}'))
            return true;

        for (;;)
        {
            std::string k;

            if (!str(k))
                return false;

            if (!eat(':'))
                return false;

            JVal x;

            if (!val(x))
                return false;

            v.o[k] = x;

            if (eat('}'))
                return true;

            if (!eat(','))
                return false;
        }
    }

    bool arr(JVal& v)
    {
        if (!eat('['))
            return false;

        v.t = JVal::ARR;
        skip();

        if (eat(']'))
            return true;

        for (;;)
        {
            JVal x;

            if (!val(x))
                return false;

            v.a.push_back(x);

            if (eat(']'))
                return true;

            if (!eat(','))
                return false;
        }
    }
};

bool JsonParseText(const std::string& text, JVal& root, std::string& err)
{
    Parser p(text);
    return p.parse(root, err);
}

bool JsonParseFile(const char* path, JVal& root, std::string& err)
{
    std::string text = JsonReadFile(path);

    if (text.empty())
    {
        err = "JSON file not found or empty";
        return false;
    }

    return JsonParseText(text, root, err);
}

const JVal* JsonGet(const JVal& o, const char* k)
{
    if (o.t != JVal::OBJ)
        return NULL;

    std::map<std::string, JVal>::const_iterator it = o.o.find(k);

    return it == o.o.end() ? NULL : &it->second;
}

std::string JsonString(const JVal& o, const char* k, const std::string& d)
{
    const JVal* v = JsonGet(o, k);

    if (!v)
        return d;

    if (v->t == JVal::STR)
        return v->s;

    if (v->t == JVal::NUM)
    {
        char buf[32];

        _snprintf(
            buf,
            sizeof(buf) - 1,
            "%ld",
            (long)v->n
        );

        buf[sizeof(buf) - 1] = 0;

        return buf;
    }

    return d;
}

int JsonInt(const JVal& o, const char* k, int d)
{
    const JVal* v = JsonGet(o, k);

    if (!v)
        return d;

    if (v->t == JVal::NUM)
        return (int)v->n;

    if (v->t == JVal::STR)
        return (int)strtol(v->s.c_str(), NULL, 0);

    return d;
}

bool JsonBool(const JVal& o, const char* k, bool d)
{
    const JVal* v = JsonGet(o, k);

    return (v && v->t == JVal::BOOL) ? v->b : d;
}
