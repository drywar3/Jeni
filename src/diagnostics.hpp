#pragma once

#include "source.hpp"
#include "diag_code.hpp"
#include "parser/locus.hpp"

#include "misc/misc.hpp"

enum struct Severity {
    Error,
    Warning,
    Note,
};

static inline const char *sevstr(Severity s)
{
    switch (s) {
        case Severity::Error: return "error";
        case Severity::Warning: return "warning";
        case Severity::Note: return "note";
        default: todo();
    }
}

struct Label {
    Locus       locus;
    bool        is_primary;

    Label(Locus locus, bool is_primary = false)
        : locus(locus), is_primary(is_primary)
    {

    }
};

enum Suggestion_Kind {
    Insert,
    Remove,
    Replace,
};

struct Suggestion {
    Suggestion_Kind kind;
    Locus locus;
    Sv text;
    Opt<Sv> header;

    static Suggestion Insert(Locus locus, Sv text);
    static Suggestion Remove(Locus locus);
    static Suggestion Replace(Locus locus, Sv text);

    Suggestion with_header(Sv header);
};

struct Diagnostic {
    Diag_Code code;
    Sv message;
    Severity severity;
    Arr<Suggestion> suggestions;
    Arr<Label>      labels;

    Diagnostic(Alloc allocator, Severity severity, Locus locus,
               Sv message);

    static Diagnostic Error(Alloc a, Locus locus, Sv message);
    static Diagnostic Warning(Alloc a, Locus locus, Sv message);
    static Diagnostic Note(Alloc a, Locus locus, Sv message);

    Diagnostic with_code(int code);
    Diagnostic add_suggestion(Suggestion suggestion);

    void report(const Source_Manager *sm, FILE *out);
};

struct Diagnostic_Pool {
    Arr<Diagnostic> buffer;
    usize w_count = 0, e_count = 0;
    usize w_max, e_max;

    Diagnostic_Pool(Alloc allocator, usize w_max, usize e_max);

    void report(Severity severity, Locus locus, const char *message);
    void report(Diagnostic diagnostic);

    bool has_errors() const;
    bool has_reached_max_errors() const;
    bool is_empty() const;
};
