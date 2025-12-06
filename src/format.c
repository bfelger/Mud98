////////////////////////////////////////////////////////////////////////////////
// format.c
// Screen and character formatting utilities
////////////////////////////////////////////////////////////////////////////////

// NOTE: This re-implementation focuses on correctness (color-aware line-char
// counts and sentinels) rather than raw throughput. The legacy formatter
// benefitted from glibc's highly vectorized strcpy/strcat, so GCC/Linux builds
// of the old format_string() will benchmark faster than this hand-rolled logic.
// MSVC lacks those tuned routines, which is why this version beats the legacy 
// code on Windows. 
// Keep that in mind when comparing cross-platform numbers.

#include "format.h"

#include "color.h"
#include "db.h"
#include "merc.h"
#include "recycle.h"
#include "comm.h"

#include <ctype.h>
#include <string.h>

#ifdef BENCHMARK_FMT_PROCS
#include <benchmarks/benchmarks.h>
Timer norm_timer = { 0 };
Timer wrap_timer = { 0 };

long norm_elapsed;
long wrap_elapsed;
#endif

// Sentinel (marker) to preserve intentional blank lines between paragraphs.
// It gets added by normalize_text(), then converted to a double CRLF by
// wrap_text().
#define PARA_BREAK     '\v'

// Sentinel for book-style paragraph breaks that keep the first-line indent but
// only insert a single newline (no blank spacer).
#define INDENT_BREAK   '\x0E'

static inline bool is_sentence_punct(char c)
{
    return c == '.' || c == '?' || c == '!';
}

static size_t normalize_text(const char* input, char* output, size_t capacity)
{
#ifdef BENCHMARK_FMT_PROCS
    start_timer(&norm_timer);
#endif
    const char* src = (input != NULL) ? input : "";
    size_t out_len = 0;
    bool capitalize_next = true;

    // Handle leading indentation before any text appears so book-style paragraphs
    // at the start of a string are preserved.
    if (*src == ' ' || *src == '\t') {
        size_t indent_len = 0;
        const char* indent_probe = src;
        while (*indent_probe == ' ' || *indent_probe == '\t') {
            indent_len += (*indent_probe == '\t') ? 4 : 1;
            ++indent_probe;
        }
        if (indent_len >= 4) {
            src = indent_probe;
            if (out_len + 1 < capacity)
                output[out_len++] = INDENT_BREAK;
            size_t spaces = indent_len;
            while (spaces-- > 0 && out_len + 1 < capacity)
                output[out_len++] = ' ';
        }
    }

    while (*src != '\0' && out_len + 1 < capacity) {
        char c = *src++;

        // Copy color codes as-is without affecting wrapping calculations.
        if (c == COLOR_ESC_CHAR) {
            if (*src == '\0') {
                output[out_len++] = c;
                break;
            }

            if (out_len + 2 >= capacity)
                break;

            output[out_len++] = COLOR_ESC_CHAR;
            output[out_len++] = *src++;
            continue;
        }

        // Newline handling: preserve double newlines, but otherwise elide
        // newlines within the paragraph.
        if (c == '\r' || c == '\n') {
            int newline_groups = 1;

            if ((c == '\r' && *src == '\n') || (c == '\n' && *src == '\r'))
                ++src;

            while (*src == '\r' || *src == '\n') {
                ++newline_groups;
                if (*src == '\r' && *(src + 1) == '\n') {
                    src += 2;
                }
                else if (*src == '\n' && *(src + 1) == '\r') {
                    src += 2;
                }
                else
                    ++src;
            }

            int blank_lines = newline_groups - 1;

            // Look ahead to see if the next paragraph begins with a first-line
            // indent of at least four spaces (tabs count as four).
            size_t indent_len = 0;
            const char* indent_probe = src;
            while (*indent_probe == ' ' || *indent_probe == '\t') {
                indent_len += (*indent_probe == '\t') ? 4 : 1;
                ++indent_probe;
            }
            bool has_indent = indent_len >= 4;
            if (has_indent)
                src = indent_probe;

            if (blank_lines > 0) {
                if (out_len > 0 && output[out_len - 1] == ' ')
                    --out_len;
                while (blank_lines-- > 0 && out_len + 1 < capacity)
                    output[out_len++] = PARA_BREAK;

                if (has_indent && out_len + 1 < capacity) {
                    output[out_len++] = INDENT_BREAK;
                    size_t spaces = indent_len;
                    while (spaces-- > 0 && out_len + 1 < capacity)
                        output[out_len++] = ' ';
                }

                capitalize_next = true;
                continue;
            }

            if (has_indent && out_len + 1 < capacity) {
                output[out_len++] = INDENT_BREAK;
                size_t spaces = indent_len;
                while (spaces-- > 0 && out_len + 1 < capacity)
                    output[out_len++] = ' ';
                capitalize_next = true;
                continue;
            }

            if (out_len == 0
                || output[out_len - 1] == ' '
                || output[out_len - 1] == PARA_BREAK
                || output[out_len - 1] == INDENT_BREAK)
                continue;
            output[out_len++] = ' ';
            continue;
        }

        if (c == '\t')
            c = ' ';

        if (c == ' ') {
            if (out_len == 0
                || output[out_len - 1] == ' '
                || output[out_len - 1] == PARA_BREAK
                || output[out_len - 1] == INDENT_BREAK)
                continue;
            output[out_len++] = ' ';
            continue;
        }

        // Backtrack and remove the double-space after punctuation if the next
        // character is a close paren.
        if (c == ')') {
            if (out_len >= 3
                && output[out_len - 1] == ' '
                && output[out_len - 2] == ' '
                && is_sentence_punct(output[out_len - 3])) {
                output[out_len - 2] = ')';
            }
            else {
                output[out_len++] = ')';
            }
            continue;
        }

        // Insert double spaces after sentences and mark the next character for
        // capitalization.
        if (is_sentence_punct(c)) {
            if (out_len + 3 >= capacity)
                break;

            output[out_len++] = c;

            const char* lookahead = src;
            while (*lookahead == '\r')
                ++lookahead;

            if (*lookahead == '"') {
                if (out_len + 1 < capacity) {
                    output[out_len++] = '"';
                    src = lookahead + 1;
                }
                else {
                    ++src;
                }
            }

            output[out_len++] = ' ';
            output[out_len++] = ' ';
            capitalize_next = true;
            continue;
        }

        if (capitalize_next && islower((unsigned char)c))
            c = UPPER(c);

        capitalize_next = false;
        output[out_len++] = c;
    }

    if (out_len > 0 && output[out_len - 1] == ' ')
        --out_len;

    output[out_len] = '\0';

#ifdef BENCHMARK_FMT_PROCS
    stop_timer(&norm_timer);
    norm_elapsed += elapsed(&norm_timer).tv_nsec;
#endif

    return out_len;
}

static size_t append_range(char* dest, size_t dest_len, size_t capacity, const char* src, size_t count)
{
    if (count == 0 || dest_len >= capacity - 1)
        return dest_len;

    if (dest_len + count >= capacity)
        count = capacity - dest_len - 1;

    memcpy(dest + dest_len, src, count);
    dest_len += count;
    dest[dest_len] = '\0';
    return dest_len;
}

static inline size_t append_literal(char* dest, size_t dest_len, size_t capacity, const char* literal)
{
    return append_range(dest, dest_len, capacity, literal, strlen(literal));
}

static size_t wrap_paragraph(char* text, char* out, size_t capacity, size_t out_len, bool keep_leading_spaces)
{
    if (!keep_leading_spaces)
        while (*text == ' ')
            ++text;

    char* cursor = text;

    while (*cursor != '\0') {
        size_t limit = (out_len == 0) ? 73 : 76;
        size_t visible = 0;
        size_t idx = 0;
        size_t last_space_idx = (size_t)-1;

        while (cursor[idx] != '\0') {
            char ch = cursor[idx];

            // Skip over color codes (except ^^ which shows as one char) when
            // counting visible line length.
            if (ch == COLOR_ESC_CHAR && cursor[idx + 1] != '\0') {
                if (cursor[idx + 1] == COLOR_ESC_CHAR)
                    ++visible;
                idx += 2;
                continue;
            }

            if (ch == ' ')
                last_space_idx = idx;

            ++visible;
            ++idx;

            if (visible > limit)
                break;
        }

        if (cursor[idx] == '\0' && visible <= limit)
            break;

        size_t break_idx = idx;
        bool force_hyphen = false;

        if (visible > limit) {
            if (last_space_idx != (size_t)-1)
                break_idx = last_space_idx;
            else
                force_hyphen = true;
        }

        if (force_hyphen) {
            if (break_idx > 0 && cursor[break_idx - 1] == COLOR_ESC_CHAR && cursor[break_idx] != '\0')
                --break_idx;
            out_len = append_range(out, out_len, capacity, cursor, break_idx);
            out_len = append_literal(out, out_len, capacity, "-\n\r");
            cursor += break_idx;
            continue;
        }

        char saved = cursor[break_idx];
        cursor[break_idx] = '\0';
        out_len = append_range(out, out_len, capacity, cursor, break_idx);
        out_len = append_literal(out, out_len, capacity, "\n\r");
        cursor[break_idx] = saved;

        cursor += break_idx;
        while (*cursor == ' ')
            ++cursor;
    }

    size_t tail_len = strlen(cursor);
    while (tail_len > 0) {
        char ch = cursor[tail_len - 1];
        if (ch == ' ' || ch == '\n' || ch == '\r')
            --tail_len;
        else
            break;
    }
    cursor[tail_len] = '\0';

    if (*cursor != '\0')
        out_len = append_range(out, out_len, capacity, cursor, strlen(cursor));

    return out_len;
}

static size_t wrap_text(char* text, char* out, size_t capacity)
{
#ifdef BENCHMARK_FMT_PROCS
    start_timer(&wrap_timer);
#endif
    size_t out_len = 0;
    char* segment = text;
    bool keep_leading_spaces = false;
    bool paragraph_emitted = false;

    // Split normalized text around sentinel markers so intentional blank lines
    // and indent hints survive wrapping.
    while (true) {
        char* next = NULL;
        char break_char = '\0';
        for (char* probe = segment; *probe != '\0'; ++probe) {
            if (*probe == PARA_BREAK || *probe == INDENT_BREAK) {
                next = probe;
                break_char = *probe;
                break;
            }
        }

        if (next != NULL)
            *next = '\0';

        if (segment && *segment != '\0') {
            out_len = wrap_paragraph(segment, out, capacity, out_len, keep_leading_spaces);
            keep_leading_spaces = false;
            paragraph_emitted = true;
        }

        if (next == NULL)
            break;

        if (paragraph_emitted) {
            if (out_len < 2 || out[out_len - 2] != '\n')
                out_len = append_literal(out, out_len, capacity, "\n\r");
            if (break_char == PARA_BREAK)
                out_len = append_literal(out, out_len, capacity, "\n\r");
            paragraph_emitted = false;
        }

        keep_leading_spaces = (break_char == INDENT_BREAK);
        segment = next + 1;
        continue;
    }

    if (out_len < 2 || out[out_len - 2] != '\n')
        out_len = append_literal(out, out_len, capacity, "\n\r");

#ifdef BENCHMARK_FMT_PROCS
    stop_timer(&wrap_timer);
    wrap_elapsed += elapsed(&wrap_timer).tv_nsec;
#endif
    return out_len;
}

char* format_string(const char* text)
{
    char normalized[MAX_STRING_LENGTH] = { 0 };
    normalize_text(text, normalized, sizeof(normalized));

    char wrapped[MAX_STRING_LENGTH * 2] = { 0 };
    wrap_text(normalized, wrapped, sizeof(wrapped));

    return str_dup(wrapped);
}
