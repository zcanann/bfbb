#include <stdio.h>
#include <string.h>
#include <rwsdk/rwcore.h>

/* Not declared by our MSL string.h */
extern char* strtok(char* string, const char* delimit);

typedef union RwPtrChar RwPtrChar;
union RwPtrChar
{
    RwChar* ptrChar;
    const RwChar* constptrChar;
};

static int StrICmp(const RwChar* s1, const RwChar* s2)
{
    RwChar c1;
    RwChar c2;

    if (s1 && s2)
    {
        do
        {
            c1 = *s1;
            c2 = *s2;

            if (c1 >= 'A' && c1 <= 'Z')
            {
                c1 += 'a' - 'A';
            }

            if (c2 >= 'A' && c2 <= 'Z')
            {
                c2 += 'a' - 'A';
            }

            if (c1 != c2)
            {
                return c1 - c2;
            }

            s1++;
            s2++;
        } while (c1 && c2);

        if (c1 != c2)
        {
            return c1 - c2;
        }
    }

    return 0;
}

static RwChar* StrUpr(RwChar* s)
{
    RwChar c;
    RwChar* p;

    if (s)
    {
        for (p = s; (c = *p) != 0; p++)
        {
            if (c >= 'a' && c <= 'z')
            {
                *p = c - ('a' - 'A');
            }
        }
    }

    return s;
}

static RwChar* StrLwr(RwChar* s)
{
    RwChar c;
    RwChar* p;

    if (s)
    {
        for (p = s; (c = *p) != 0; p++)
        {
            if (c >= 'A' && c <= 'Z')
            {
                *p = c + ('a' - 'A');
            }
        }
    }

    return s;
}

static RwChar* StrChr(const RwChar* s, int c)
{
    RwPtrChar result;
    RwChar match = (RwChar)c;
    RwChar ch;

    result.ptrChar = NULL;

    do
    {
        ch = *s;
        if (ch == match)
        {
            result.constptrChar = s;
            break;
        }
        s++;
    } while (ch);

    return result.ptrChar;
}

static RwChar* StrRChr(const RwChar* s, int c)
{
    RwPtrChar result;
    RwChar match = (RwChar)c;
    RwChar ch;

    result.ptrChar = NULL;

    do
    {
        ch = *s;
        if (ch == match)
        {
            result.constptrChar = s;
        }
        s++;
    } while (ch);

    return result.ptrChar;
}

RwBool _rwStringOpen(void)
{
    RWSRCGLOBAL(stringFuncs).vecSprintf = (vecSprintfFunc)sprintf;
    RWSRCGLOBAL(stringFuncs).vecVsprintf = (vecVsprintfFunc)vsprintf;
    RWSRCGLOBAL(stringFuncs).vecStrcpy = (vecStrcpyFunc)strcpy;
    RWSRCGLOBAL(stringFuncs).vecStrncpy = (vecStrncpyFunc)strncpy;
    RWSRCGLOBAL(stringFuncs).vecStrcat = (vecStrcatFunc)strcat;
    RWSRCGLOBAL(stringFuncs).vecStrncat = (vecStrncatFunc)strncat;
    RWSRCGLOBAL(stringFuncs).vecStrrchr = (vecStrrchrFunc)StrRChr;
    RWSRCGLOBAL(stringFuncs).vecStrchr = (vecStrchrFunc)StrChr;
    RWSRCGLOBAL(stringFuncs).vecStrstr = (vecStrstrFunc)strstr;
    RWSRCGLOBAL(stringFuncs).vecStrcmp = (vecStrcmpFunc)strcmp;
    RWSRCGLOBAL(stringFuncs).vecStrncmp = (vecStrncmpFunc)strncmp;
    RWSRCGLOBAL(stringFuncs).vecStricmp = (vecStricmpFunc)StrICmp;
    RWSRCGLOBAL(stringFuncs).vecStrlen = (vecStrlenFunc)strlen;
    RWSRCGLOBAL(stringFuncs).vecStrupr = (vecStruprFunc)StrUpr;
    RWSRCGLOBAL(stringFuncs).vecStrlwr = (vecStrlwrFunc)StrLwr;
    RWSRCGLOBAL(stringFuncs).vecStrtok = (vecStrtokFunc)strtok;
    RWSRCGLOBAL(stringFuncs).vecSscanf = (vecSscanfFunc)sscanf;

    return TRUE;
}

void _rwStringClose(void)
{
}
