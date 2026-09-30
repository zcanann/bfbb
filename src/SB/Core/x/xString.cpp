#include "xString.h"
#include "rwplcore.h"
#include "xMath.h"

#include <types.h>

U32 xStrHash(const char* str)
{
    U32 hash = 0;
    U32 i;

    while (i = *str, i != NULL)
    {
        hash = (i - (i & (S32)i >> 1 & 0x20) & 0xff) + hash * 0x83;
        str++;
    }

    return hash;
}

U32 xStrHash(const char* str, size_t size)
{
    U32 hash = 0;
    U32 i = 0;
    U32 c;

    while (i < size && (c = *str, c != NULL))
    {
        i++;
        str++;
        hash = (c - (c & (S32)c >> 1 & 0x20) & 0xff) + hash * 0x83;
    }

    return hash;
}

U32 xStrHashCat(U32 prefix, const char* str)
{
    // The accumulator starts at the prefix -- retail keeps it in r3, the same
    // register the argument arrives in, so leaving it uninitialised happened to
    // match. It is still a live read of an indeterminate value anywhere the
    // parameter is not already in the accumulator's register.
    U32 hash = prefix;
    U32 i;

    while (i = *str, i != NULL)
    {
        str++;
        hash = (i - (i & (S32)i >> 1 & 0x20) & 0xff) + hash * 0x83;
    }

    return hash;
}

char* xStrTok(char* string, const char* control, char** nextoken)
{
    U8* str;
    U8* ctrl;
    U8 map[32];
    S32 count;
    U8 c;

    for (S32 i = 0; i < 32; i++)
    {
        map[i] = 0;
    }

    ctrl = (U8*)control;

    do
    {
        U8 bit = 1 << (*ctrl & 0x7);
        map[*ctrl >> 3] |= bit;
    } while (*ctrl++ != '\0');

    str = (string) ? (U8*)string : (U8*)*nextoken;

    while (map[(*str >> 3) & 0x1F] & (1 << (*str & 0x7)) && *str != '\0')
    {
        str++;
    }

    string = (char*)str;

    while ((c = *str) != '\0')
    {
        if (map[(c >> 3) & 0x1F] & (1 << (c & 0x7)))
        {
            *str = '\0';
            str++;
            break;
        }

        str++;
    }

    *nextoken = (char*)str;

    if (string == (char*)str)
    {
        string = NULL;
    }

    return string;
}

char* xStrTokBuffer(const char* string, const char* control, void* buffer)
{
    U8 c;
    U8* str;
    U8* ctrl;
    U8 map[32];
    char* dest = (char*)buffer;
    dest += sizeof(char*);

    for (S32 i = 0; i < 32; i++)
    {
        map[i] = 0;
    }

    ctrl = (U8*)control;

    do
    {
        U8 bit = 1 << (*ctrl & 0x7);
        map[*ctrl >> 3] |= bit;
    } while (*ctrl++ != '\0');

    str = (string) ? (U8*)string : (U8*)*(char**)buffer;

    while (map[(*str >> 3) & 0x1F] & (1 << (*str & 0x7)) && *str != '\0')
    {
        str++;
    }

    string = (char*)str;

    while ((c = *str) != '\0')
    {
        if (map[(c >> 3) & 0x1F] & (1 << (c & 0x7)))
        {
            str++;
            break;
        }

        *dest = c;
        dest++;
        str++;
    }

    *dest = '\0';
    *(char**)buffer = (char*)str;

    if (string == (char*)str)
    {
        return NULL;
    }

    return (char*)buffer + sizeof(char*);
}

#define XSTR_UPPER(c) ((c) >= 'a' && (c) <= 'z' ? (c)-32 : (c))

S32 xStricmp(const char* string1, const char* string2)
{
    S32 result = 0;

    while (XSTR_UPPER(*string1) == XSTR_UPPER(*string2) && !result)
    {
        if (*string1 == '\0' || *string2 == '\0')
        {
            result = 1;
        }
        else
        {
            string1++;
            string2++;
        }
    }

    result = 0;
    if (*string1 != *string2)
    {
        result = 1;
        if (XSTR_UPPER(*string1) < XSTR_UPPER(*string2))
        {
            result = -1;
        }
    }

    return result;
}

#undef XSTR_UPPER

char* xStrupr(char* string)
{
    char* p = string;

    while (*p != '\0')
    {
        *p = (*p >= 'a' && *p <= 'z' ? *p - 32 : *p);

        p++;
    }

    return string;
}

namespace
{
    U32 tolower(char param_1);
    U32 tolower(S32 param_1);
} // namespace

S32 xStrParseFloatList(F32* dest, const char* strbuf, S32 max)
{
    char* str;
    S32 index;
    S32 digits;
    S32 negate;
    char* numstart;
    char savech;

    if (!(str = (char*)strbuf))
    {
        return 0;
    }

    for (index = 0; *str != '\0' && index < max; index++)
    {
        while (*str == '\t' || *str == ' ' || *str == '[' || *str == ']' || *str == '{' ||
               *str == '}' || *str == '(' || *str == ')' || *str == '+' || *str == ',' ||
               *str == ':' || *str == ';')
        {
            str++;
        }

        if (*str == '\0')
        {
            return index;
        }

        if (*str == '-')
        {
            negate = TRUE;
            str++;

            while (*str == '\t' || *str == ' ')
            {
                str++;
            }
        }
        else
        {
            negate = FALSE;
        }

        numstart = str;
        digits = 0;

        while ((*str >= '0' && *str <= '9') || *str == '.' || *str == 'E' || *str == 'e' ||
               *str == 'f')
        {
            if (*str >= '0' && *str <= '9')
            {
                digits++;
            }

            str++;
        }

        if (digits == 0)
        {
            return index;
        }

        savech = *str;

        *str = '\0';
        *dest = xatof(numstart);

        if (negate)
        {
            *dest = -*dest;
        }

        *str = savech;
        dest++;
    }

    return index;
}

S32 imemcmp(void const* d1, void const* d2, size_t size)
{
    const char* s1 = (char*)d1;
    const char* s2 = (char*)d2;

    for (size_t i = 0; i < size; i++, s1++, s2++)
    {
        S32 cval1 = tolower(*s1);
        S32 cval2 = tolower(*s2);
        if (cval1 != cval2)
        {
            return cval1 - cval2;
        }
    }

    return 0;
}

namespace
{
    U32 tolower(char param_1)
    {
        return tolower((S32)param_1);
    }

    U32 tolower(S32 param_1)
    {
        return param_1 | ((param_1 >> 1) & 32);
    }
} // End anonymous namespace

S32 icompare(const substr& s1, const substr& s2)
{
    U32 len = MIN(s1.size, s2.size);
    S32 result = imemcmp(s1.text, s2.text, len);
    switch (result)
    {
    case 0:
        if (s1.size == s2.size)
        {
            result = 0;
        }
        else
        {
            result = 1;
            if (s1.size < s2.size)
            {
                result = -1;
            }
        }
        break;
    }
    return result;
}

size_t atox(const substr& s, size_t& read_size)
{
    const char* text = s.text;
    size_t size = s.size;

    if (text == NULL)
    {
        return 0;
    }

    size_t value = 0;

    if (size > 8)
    {
        size = 8;
    }

    for (read_size = 0; read_size < size; read_size++)
    {
        U32 digit;
        U8 c = *text;

        if (c >= '0' && c <= '9')
        {
            digit = c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = c - 'A' + 10;
        }
        else
        {
            break;
        }

        value = (value << 4) + digit;
        text++;
    }

    return value;
}

// Each case of the switch is the same scan with the character set fully
// unrolled, so that the common short sets never touch a second loop.
#define FIND_CHAR_SCAN(match)                                                                      \
    size = s.size;                                                                                 \
    while (size > 0 && *text != '\0')                                                              \
    {                                                                                              \
        c = *text;                                                                                 \
        if (match)                                                                                 \
        {                                                                                          \
            return text;                                                                           \
        }                                                                                          \
        size--;                                                                                    \
        text++;                                                                                    \
    }                                                                                              \
    break

const char* find_char(const substr& s, const substr& cs)
{
    if (s.text == NULL || cs.text == NULL)
    {
        return NULL;
    }

    const char* text = s.text;
    S32 size;
    U8 c;

    switch (cs.size)
    {
    case 0:
        break;
    case 1:
        FIND_CHAR_SCAN(c == cs.text[0]);
    case 2:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1]);
    case 3:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2]);
    case 4:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3]);
    case 5:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4]);
    case 6:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4] || c == cs.text[5]);
    case 7:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4] || c == cs.text[5] || c == cs.text[6]);
    case 8:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4] || c == cs.text[5] || c == cs.text[6] || c == cs.text[7]);
    case 9:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4] || c == cs.text[5] || c == cs.text[6] || c == cs.text[7] ||
                       c == cs.text[8]);
    case 10:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4] || c == cs.text[5] || c == cs.text[6] || c == cs.text[7] ||
                       c == cs.text[8] || c == cs.text[9]);
    case 11:
        FIND_CHAR_SCAN(c == cs.text[0] || c == cs.text[1] || c == cs.text[2] || c == cs.text[3] ||
                       c == cs.text[4] || c == cs.text[5] || c == cs.text[6] || c == cs.text[7] ||
                       c == cs.text[8] || c == cs.text[9] || c == cs.text[10]);
    default:
        size = s.size;

        while (size > 0 && *text != '\0')
        {
            for (const char* p = cs.text; *p != '\0'; p++)
            {
                if (*text == *p)
                {
                    return text;
                }
            }

            size--;
            text++;
        }

        break;
    }

    return NULL;
}

#undef FIND_CHAR_SCAN
