#ifndef XFONTHELPERS_H
#define XFONTHELPERS_H

// Private implementation include, owned only by xFont.cpp. Its placement keeps
// the helper code group and aggregate initializer order consistent with retail.
// These are out-of-line definitions; do not include this from another TU.

substr substr::create(const char* text, size_t size)
{
    substr s = { text, size };
    return s;
}

size_t rskip_ws(substr& s)
{
    return rskip_ws(s.text, s.size);
}

size_t rskip_ws(const char*& text, size_t& size)
{
    while (size && is_ws(text[size - 1]))
    {
        size--;
    }

    return size;
}

bool is_ws(char c)
{
    return (c == ' ' || c == '\t' || c == '\n');
}

const char* find_char(const substr& s, char c)
{
    if (!s.text)
    {
        return NULL;
    }

    const char* text = s.text;
    S32 size = s.size;

    while (size > 0 && *text != '\0')
    {
        if (*text == c)
        {
            return text;
        }

        size--;
        text++;
    }

    return NULL;
}

const char* skip_ws(substr& s)
{
    return skip_ws(s.text, s.size);
}

const char* skip_ws(const char*& text, size_t& size)
{
    size_t i = 0;

    while (i < size && *text != '\0')
    {
        if (!is_ws(*text))
        {
            size -= i;
            break;
        }

        text++;
        i++;
    }

    return text;
}

size_t atox(const substr& s)
{
    size_t read_size;
    return atox(s, read_size);
}

size_t trim_ws(substr& s)
{
    return trim_ws(s.text, s.size);
}

size_t trim_ws(const char*& text, size_t& size)
{
    skip_ws(text, size);
    return rskip_ws(text, size);
}

xtextbox::tag_type* xtextbox::find_format_tag(const substr& s)
{
    S32 index;
    return find_format_tag(s, index);
}

size_t xtextbox::layout::jots_size() const
{
    return _jots_size;
}

xtextbox xtextbox::create()
{
    return create(xfont::create(), screen_bounds, 0, 0.0f, 0.0f, 0.0f, 0.0f);
}

xfont xfont::create()
{
    return create(0, 0.0f, 0.0f, 0.0f, g_WHITE, screen_bounds);
}

void xtextbox::jot::intersect_flags(const jot& other)
{
    *(U16*)&flag &= *(U16*)&other.flag;
}

void xtextbox::jot::reset_flags()
{
    *(U16*)&flag = 0;
}

xSphere* xModelGetLocalSBound(xModelInstance* model)
{
    return (xSphere*)RpAtomicGetBoundingSphere(model->Data);
}

#endif
