////////////////////////////////////////////////////////////////////////////////
// match.c
// Super lightweight pattern matching
////////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

static inline unsigned char to_lower(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';

    return c;
}

// CHARACTER SCAN //////////////////////////////////////////////////////////////

static inline bool is_alpha_ascii(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z');
}

static inline bool is_digit_ascii(unsigned char c)
{
    return (c >= '0' && c <= '9');
}

static inline bool is_alnum_ascii(unsigned char c)
{
    return is_digit_ascii(c) ||
        is_alpha_ascii(c) ||
        c == '_';
}

static inline bool is_hex_ascii(unsigned char c)
{
    return is_digit_ascii(c) ||
        (c >= 'A' && c <= 'F') ||
        (c >= 'a' && c <= 'f');
}

static inline bool is_ws(unsigned char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

// TOKEN SCAN //////////////////////////////////////////////////////////////////

// One or more alpha chars
static const char* consume_A(const char* s)
{
    const unsigned char* p = (const unsigned char*)s;
    if (!is_alpha_ascii(*p))
        return NULL;
    do {
        p++;
    } while (is_alpha_ascii(*p));
    return (const char*)p;
}

// One or more digit chars
static const char* consume_N(const char* s)
{
    const unsigned char* p = (const unsigned char*)s;
    if (!is_digit_ascii(*p))
        return NULL;
    do {
        p++;
    } while (is_digit_ascii(*p));
    return (const char*)p;
}

// One or more hex chars
static const char* consume_X(const char* s)
{
    const unsigned char* p = (const unsigned char*)s;
    if (!is_hex_ascii(*p))
        return NULL;
    do {
        p++;
    } while (is_hex_ascii(*p));
    return (const char*)p;
}

// One or more alphanumeric chars (plus underscore) for identifiers
static const char* consume_W(const char* s)
{
    const unsigned char* p = (const unsigned char*)s;

    if (!is_alpha_ascii(*p))
        return NULL;

    p++;
    while (is_alnum_ascii(*p))
        p++;

    while (is_ws(*p))
        p++;

    return (const char*)p;
}

typedef const char* (*consume_fn)(const char* s);

static bool match_from(const char* pat, const char* s, char sigil);

static bool match_quantified(const char* rest_pat,
    const char* s,
    char sigil,
    consume_fn consume,
    size_t min_count)
{
    const char* start = s;
    size_t count = 0;
    const char* p = s;

    // Greedy: consume as many units as possible
    for (;;) {
        const char* next = consume(p);
        if (!next)
            break;
        count++;
        p = next;
    }

    if (count < min_count)
        return false;

    // Backtrack: try with max units down to min_count
    for (;;) {
        if (match_from(rest_pat, p, sigil))
            return true;

        if (count == min_count)
            break;

        count--;

        // Recompute p as end after 'count' units from start
        const char* q = start;
        for (size_t i = 0; i < count; ++i) {
            q = consume(q); // must succeed; we already know layout
        }
        p = q;
    }

    return false;
}

static bool match_from(const char* pat, const char* s, char sigil)
{
    for (;;) {
        while (is_ws(*s)) s++;
        while (is_ws(*pat)) pat++;

        unsigned char pc = (unsigned char)*pat;
        unsigned char sc = (unsigned char)*s;

        if (pc == '\0') return sc == '\0';

        if (pc == (unsigned char)sigil) {
            // Escaped sigil
            if (pat[1] == sigil) {
                if (sc != sigil)
                    return false;
                pat += 2;
                s += 1;
                continue;
            }

            char t = pat[1];
            char q = pat[2]; // potential quantifier
            bool has_quant = (q == '+' || q == '*');
            size_t min_count = (has_quant && q == '*') ? 0u : 1u;
            const char* rest_pat = pat + 2 + (has_quant ? 1 : 0);

            consume_fn consume = NULL;

            switch (t) {
            case 'A': consume = consume_A; break;
            case 'N': consume = consume_N; break;
            case 'X': consume = consume_X; break;
            case 'W': consume = consume_W; break;
            // add more tokens here: case 'X': consume = consume_X; etc.
            default:
                // Unknown token: treat next char literally
                if (sc != (unsigned char)t)
                    return false;
                pat += 2;
                s += 1;
                continue;
            }

            if (!has_quant) {
                // Single occurrence
                const char* next = consume(s);
                if (!next)
                    return false;
                pat = rest_pat;
                s = next;
                continue;
            }

            // With + or * quantifier
            return match_quantified(rest_pat, s, sigil, consume, min_count);
        }

        // Literal byte match
        if (sc == '\0' || sc != pc)
            return false;

        pat += 1;
        s += 1;
    }
}

bool mini_match(const char* pattern, const char* s, char sigil)
{
    // Skip introductory whitespace.
    while (is_ws(*s)) s++;
    while (is_ws(*pattern)) pattern++;

    return match_from(pattern, s, sigil);
}
