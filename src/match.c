////////////////////////////////////////////////////////////////////////////////
// match.c
// Super lightweight pattern matching
////////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

static unsigned char to_lower(unsigned char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';

    return c;
}

static bool is_alpha_ascii(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z');
}

static bool is_alnum_ascii(unsigned char c)
{
    return (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        c == '_';
}

static bool is_digit_ascii(unsigned char c)
{
    return (c >= '0' && c <= '9');
}

static bool is_hex_ascii(unsigned char c)
{
    return (c >= '0' && c <= '9') ||
        (c >= 'A' && c <= 'F') ||
        (c >= 'a' && c <= 'f');
}

static bool is_ws(unsigned char c)
{
    return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

static bool match_from(const char* pat, const char* s, char sigil);

static size_t span_class(const char* s, bool (*pred)(unsigned char))
{
    size_t n = 0;
    while (s[n] && pred((unsigned char)s[n])) n++;
    return n;
}

static bool match_token(const char* pat, const char* s, char sigil,
    bool (*pred)(unsigned char))
{
    // Need at least one char
    size_t max = span_class(s, pred);
    if (max == 0) return false;

    // Greedy, with backtracking
    for (size_t take = max; take >= 1; --take) {
        if (match_from(pat, s + take, sigil))
            return true;
        if (take == 1) break;
    }
    return false;
}

static bool match_word(const char* pat, const char* s, char sigil)
{
    // Must start with alpha
    if (!s[0] || !is_alpha_ascii(s[0])) return false;
    return match_token(pat, s, sigil, is_alnum_ascii);
}

static bool match_from(const char* pat, const char* s, char sigil)
{
    for (;;) {
        // Different lengths of words;
        // TODO: Be gracious with shortened command forms
        if (is_ws(*pat) != is_ws(*s))
            return false;

        while (is_ws(*s)) s++;
        while (is_ws(*pat)) pat++;

        unsigned char pc = (unsigned char)*pat;
        unsigned char sc = (unsigned char)*s;

        if (pc == '\0') return sc == '\0';

        if (pc == (unsigned char)sigil) {
            // Escape?
            if (pat[1] == sigil) {
                if (sc != sigil) return false;
                pat += 2; s += 1;
                continue;
            }
            // Token switch on next char
            char t = pat[1];
            if (t == 'A') {
                pat += 2;
                return match_token(pat, s, sigil, is_alpha_ascii);
            }
            else if (t == 'N') {
                pat += 2;
                return match_token(pat, s, sigil, is_digit_ascii);
            }
            else if (t == 'W') {
                pat += 2;
                return match_word(pat, s, sigil); // word = alnum or '_'
            }
            else if (t == 'X') {
                pat += 2;
                return match_token(pat, s, sigil, is_hex_ascii);
            }
            else {
                // Unknown token -> treat as literal next char
                if (to_lower(sc) != to_lower((unsigned char)t)) return false;
                pat += 2; 
                s += 1;
                continue;
            }
        }
        else {
            // Literal must match
            if (sc == '\0' || sc != pc) return false;
            pat += 1; s += 1;
        }
    }
}

bool mini_match(const char* pattern, const char* s, char sigil)
{
    // Skip introductory whitespace.
    while (is_ws(*s)) s++;
    while (is_ws(*pattern)) pattern++;

    return match_from(pattern, s, sigil);
}
