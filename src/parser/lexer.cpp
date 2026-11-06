#include "parser/lexer.hpp"

using namespace parser;

parser::Lexer::Lexer(Source_Id id, Sv content, Alloc alloc)
    : source_id(id), content(content),
      line(1),
      col(1), prev_col(1),
      off(0), prev_off(0),
      allocator(alloc)
{
}

bool parser::Lexer::is_done() const
{
    return off >= content.length();
}

using Codepoint = uint32;

struct Token_Spec {
    const char *text;
    Token_Kind kind;
};

using Token_List = std::initializer_list<Token_Spec>;

static const Token_List KEYWORDS = {
    {"func", KW_Func},         {"function", KW_Func},
    {"const", KW_Const},       {"var", KW_Var},
    {"if", KW_If},             {"then", KW_Then},
    {"else", KW_Else},         {"for", KW_For},
    {"break", KW_Break},       {"continue", KW_Continue},
    {"cast", KW_Cast},         {"in", KW_In},
    {"ns", KW_Namespace},      {"namespace", KW_Namespace},
    {"struct", KW_Struct},     {"type", KW_Type},
    {"enum", KW_Enum},         {"using", KW_Using},
    {"return", KW_Return},     {"move", KW_Move},

    {"int", TK_Int},           {"int8", TK_Int8},
    {"uint8", TK_Uint8},       {"int16", TK_Int16},
    {"int32", TK_Int},         {"int64", TK_Int64},
    {"uint", TK_Uint},         {"uint64", TK_Uint64},
    {"usize", TK_Usize},       {"bool", TK_Bool},
    {"char", TK_Char},         {"void", TK_Void},
    {"float", TK_Float32},     {"float32", TK_Float32},
    {"float64", TK_Float64},

    {"true", LIT_True},        {"false", LIT_False},
};

static const Token_List PUNCTUATIONS = {
    {"...", SEP_Vararg},

    {"++", OP_Inc},      {"--", OP_Dec},
    {"==", OP_Equals},   {"!=", OP_Not_Equals},
    {"->", OP_Arrow},    {">=", OP_Greater_Equals},
    {"<=", OP_Less_Equals},   {"![", SEP_Turbo_Fish},

    {"+", OP_Add},       {"-", OP_Sub},
    {"*", OP_Star},      {"/", OP_Slash},
    {"=", OP_Assign},    {">", OP_Greater},
    {"<", OP_Less},      {"!", OP_Bang},
    {"^", OP_Caret},

    {"{", SEP_Lbrace},   {"[", SEP_Lbracket},
    {"(", SEP_Lparen},   {")", SEP_Rparen},
    {"}", SEP_Rbrace},   {"]", SEP_Rbracket},
    {":", SEP_Colon},    {";", SEP_Semi_Colon},
    {",", SEP_Comma},    {".", SEP_Dot},
};

static size_t decode_utf8_char(const char *str, Codepoint *codepoint)
{
    const uint8_t *bytes = (const uint8_t *)str;
    if (!bytes || !*bytes)
        return 0;

    uint8_t b0 = bytes[0];

    // 1-byte sequence (ASCII: 0xxxxxxx)
    if ((b0 & 0x80) == 0x00) {
        *codepoint = b0;
        return 1;
    }

    // 2-byte sequence (110xxxxx 10xxxxxx)
    if ((b0 & 0xE0) == 0xC0) {
        if ((bytes[1] & 0xC0) != 0x80)
            return 0; // Invalid continuation byte
        *codepoint = ((b0 & 0x1F) << 6) | (bytes[1] & 0x3F);
        return (*codepoint >= 0x80) ? 2 : 0; // Reject overlong encodings
    }

    // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
    if ((b0 & 0xF0) == 0xE0) {
        if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80)
            return 0;
        *codepoint =
            ((b0 & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);

        // Reject overlongs and UTF-16 surrogates (0xD800 - 0xDFFF)
        if (*codepoint < 0x800 ||
            (*codepoint >= 0xD800 && *codepoint <= 0xDFFF))
            return 0;
        return 3;
    }

    // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
    if ((b0 & 0xF8) == 0xF0) {
        if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 ||
            (bytes[3] & 0xC0) != 0x80)
            return 0;
        *codepoint = ((b0 & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) |
                     ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);

        // Reject overlongs and code points above the Unicode limit (0x10FFFF)
        if (*codepoint < 0x10000 || *codepoint > 0x10FFFF)
            return 0;
        return 4;
    }

    return 0; // Invalid lead byte
}

static Codepoint current(const Lexer *lexer)
{
    if (lexer->is_done())
        return 0;
    Codepoint cp;
    usize length = decode_utf8_char(&lexer->content[lexer->off], &cp);
    if (length == 0)
        return 0;
    return cp;
}

static Codepoint next(Lexer *lexer)
{
    if (lexer->is_done())
        return 0;
    Codepoint cp;
    usize length = decode_utf8_char(&lexer->content[lexer->off], &cp);
    if (cp == '\n') {
        lexer->line += 1;
        lexer->col = 1;
    } else {
        lexer->col += 1;
    }
    lexer->off += length;
    return cp;
}

static bool codepoint_isalpha(Codepoint cp)
{
    return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
           (cp == '_') || (cp > 127);
}

static bool codepoint_isdigit(Codepoint cp, int base)
{
    if (base == 10) {
        return (cp >= '0' && cp <= '9');
    } else if (base == 8) {
        return (cp >= '0' && cp <= '7');
    } else if (base == 2) {
        return (cp == '0' || cp == '1');
    } else if (base == 16) {
        return (cp >= '0' && cp <= '7') || (cp >= 'a' && cp <= 'f') ||
               (cp >= 'A' && cp <= 'F');
    }
    MINI_UNREACHABLE();
}

static bool codepoint_isspace(Codepoint cp)
{
    return (cp == ' ') || (cp == '\t') || (cp == '\r') || (cp == '\n');
}

static bool is_prefix(const Lexer *lexer, const char *prefix)
{
    if (lexer->is_done()) {
        return false;
    }

    Mini_StringView head =
        mini_sv_init(&lexer->content[lexer->off],
                     lexer->content.length() - lexer->off);
    if (mini_sv_begins_with_str(head, prefix)) {
        return true;
    }

    return false;
}

static bool eat_prefix(Lexer *lexer, const char *prefix)
{
    if (is_prefix(lexer, prefix)) {
        usize n = strlen(prefix);
        for (usize c = 0; c < n; c++) {
            next(lexer);
        }
        return true;
    }
    return false;
}

static Locus lexer_get_locus(const Lexer *lexer)
{
    return Locus(lexer->line, lexer->prev_col, lexer->col,
                lexer->prev_off, lexer->off, lexer->source_id);
}


auto parser::Lexer::next_token() -> Expect<Token, Diagnostic>
{
    if (is_done()) {
        return Token(End_Of_File, lexer_get_locus(this));
    }

    while (true) {
        if (codepoint_isspace(current(this))) {
            next(this);
            continue;
        }

        /* skip single line comments. */
        if (is_prefix(this, "//")) {
            while (!is_done() &&
                   (current(this) != '\n' && current(this) != '\r')) {
                next(this);
            }
            continue;
        }

        /* skip multi line comments */

        if (eat_prefix(this, "/*")) {
            usize count = 1;
            while (!is_done() and count > 0) {
                if (eat_prefix(this, "/*")) {
                    count += 1;
                } else if (eat_prefix(this, "*/")) {
                    count -= 1;
                } else {
                    next(this);
                }
            }
            MINI_ASSERT(count == 0, "unbalanced comment");
            continue;
        }

        break;
    }

    if (is_done()) {
        return Token(End_Of_File, lexer_get_locus(this));
    }

    prev_off = off;
    prev_col = col;

    if (is_prefix(this, "c\"")) {
        next(this);
        next(this);
        while (!is_done() and current(this) != '"') {
            next(this);
        }
        next(this);
        return Token(LIT_Cstring, lexer_get_locus(this));
    }

    if (current(this) == '"') {
        next(this);
        while (!is_done()) {
            next(this);
        }
        next(this);
        return Token(LIT_String, lexer_get_locus(this));
    }

    if (current(this) == '#') {
        if (eat_prefix(this, "#canon_name"))
            return Token(DR_Canon_Name, lexer_get_locus(this));
        MINI_UNREACHABLE("TODO: report invalid directive");
    }

    if (current(this) == '\'') {
        next(this);
        next(this);
        if (current(this) == '\'') {
            next(this);

            return Unexpect(Diagnostic::Error(allocator, lexer_get_locus(this), "In Jeni, character literals do not have a closing `'`")
                   .with_code(INVALID_CHAR_LITERAL));
        }
        return Token(LIT_Char, lexer_get_locus(this));
    }

    /* lex an identifier or keyword */
    if (codepoint_isalpha(current(this))) {
        while (!is_done() &&
               (codepoint_isalpha(current(this)) ||
                codepoint_isdigit(current(this), 10))) {
            next(this);
        }
        usize length = off - prev_off;
        Sv word = content.substr(prev_off, length);
        for (auto spec : KEYWORDS) {
            if (word == spec.text) {
                return Token(spec.kind, lexer_get_locus(this));
            }
        }
        return Token(Identifier, lexer_get_locus(this));
    }

    /* lex integers:
     *
     * if it starts with a base 10 digit that isn't 0, then it is fully base 10.
     * else if it does start with 0, then we are looking a base prefix
     * i.e 0x, 0o, 0d, 0b.
     */
    if (codepoint_isdigit(current(this), 10)) {
        int continue_base = 10;

        if (eat_prefix(this, "0x"))
            continue_base = 16;
        else if (eat_prefix(this, "0b"))
            continue_base = 2;
        else if (eat_prefix(this, "0o"))
            continue_base = 8;

        /* lex decimal part */
        while (!is_done() &&
               codepoint_isdigit(current(this), continue_base))
            next(this);

        /* check for (and lex) floating point part */
        if (current(this) == '.') {
            next(this);
            while (!is_done() &&
                   codepoint_isdigit(current(this), continue_base))
                next(this);
            return Token(LIT_Float, lexer_get_locus(this), continue_base);
        }

        /* todo: lex integer suffixes i.e u8, i8, i16, uz */

        return Token(LIT_Integer, lexer_get_locus(this), continue_base);
    }

    /* lex punctuations (operators and separators) */
    {
        for (auto spec : PUNCTUATIONS) {
            if (eat_prefix(this, spec.text)) {
                return Token(spec.kind, lexer_get_locus(this));
            }
        }
    }

    printf("%zu:%zu: %d - '%c'\n", line, prev_off,
           current(this), current(this));
    MINI_UNREACHABLE();
}
