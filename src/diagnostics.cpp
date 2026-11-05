#include "diag_code.hpp"
#include "diagnostics.hpp"

#include <cstdio>
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
#include <io.h>
#define MINI_ISATTY(f) _isatty(_fileno(f))
#else
#include <unistd.h>
#define MINI_ISATTY(f) isatty(fileno(f))
#endif


constexpr auto CONTEXT_LINES = 4;

Suggestion Suggestion::Insert(Locus locus, Sv text)
{
    return Suggestion{Suggestion_Kind::Insert,locus,text,None};
}

Suggestion Suggestion::Remove(Locus locus)
{
    return Suggestion{Suggestion_Kind::Remove,locus,"", None};
}

Suggestion Suggestion::Replace(Locus locus, Sv text)
{
    return Suggestion{Suggestion_Kind::Remove,locus,text,None};
}

Suggestion Suggestion::with_header(Sv header)
{
    this->header = header;
    return *this;
}

Diagnostic::Diagnostic(Alloc allocator, Severity severity, Locus locus,
                       Sv message)
    :
      message(message),
      severity(severity),
      suggestions(allocator),
      labels(allocator)
{
    labels.append(Label(locus, true));
}

Diagnostic Diagnostic::Error(Alloc a, Locus locus, Sv message)
{
    return Diagnostic(a, Severity::Error, locus, message);
}

Diagnostic Diagnostic::Warning(Alloc a, Locus locus, Sv message)
{
    return Diagnostic(a, Severity::Warning, locus, message);
}

Diagnostic Diagnostic::Note(Alloc a, Locus locus, Sv message)
{
    return Diagnostic(a, Severity::Note, locus, message);
}

Diagnostic Diagnostic::add_suggestion(Suggestion suggestion) {
    suggestions.append(suggestion);
    return *this;
}

Diagnostic Diagnostic::with_code(int code) {
    this->code = Diag_Code(code);
    return *this;
}

int sort_label(Label a, Label b) {
    if (a.locus.source_id != b.locus.source_id) {
        return a.locus.source_id < b.locus.source_id ? -1 : 1;
    }

    if (a.locus.line != b.locus.line) {
        return a.locus.line < b.locus.line ? -1 : 1;
    }

    if (a.locus.begin != b.locus.begin) {
        return a.locus.begin < b.locus.begin ? -1 : 1;
    }

    if (a.locus.end != b.locus.end) {
        return a.locus.end < b.locus.end ? -1 : 1;
    }

    if (a.is_primary != b.is_primary) {
        return a.is_primary ? -1 : 1;
    }

    return 0;
}

static Sv source_file_get_line(const Source_File *file, usize line_idx)
{
    const usize line_count = file->line_starts.count();
    if (line_idx >= line_count) {
        return "";
    }

    const usize total = file->content.count();
    const usize start = file->line_starts[line_idx];
    if (start >= total) {
        return "";
    }

    usize end =
        (line_idx + 1 < line_count) ? file->line_starts[line_idx + 1] : total;
    if (end > total) {
        end = total;
    }
    if (end > start && file->content[end - 1] == '\n') {
        --end;
    }
    if (end > start && file->content[end - 1] == '\r') {
        --end;
    }
    return Sv(&file->content[start], end - start);
}

void print_line(Sv line, Locus locus, FILE *out) {
    for (usize n = 0; n < line.length(); n++) {
        if (n >= locus.begin - 1) {
            fprintf(out, "\033[0;31m");
        }

        if (n >= locus.end - 1) {
            fprintf(out, "\033[0m");
        }

        fprintf(out, "%c", line[n]);
    }
    fprintf(out, "\033[0m");
}

void print_label(Label label, const Source_Manager *sm, FILE *out, usize gutter) {
    const Source_File *file = sm->get_source(label.locus.source_id);
    if (!label.is_primary) {
        fprintf(out, "\n%s:%zu:%zu:", file->path.data(), label.locus.line, label.locus.begin);
    }

    fprintf(out, " %*c |\n", (int)gutter, ' ');

    // print context lines
    for (usize n = CONTEXT_LINES; n > 0; n--) {
        int line_no = label.locus.line - n;
        if (line_no > 0) {
            Sv line = source_file_get_line(file, line_no-1);
            fprintf(out, " %*zu | %.*s\n", (int)gutter, line_no, SVARG(line.base()));
        }
    }

    Sv line = source_file_get_line(file, label.locus.line-1);
    fprintf(out, " %.*zu | ", (int)gutter, label.locus.line);
    print_line(line, label.locus, out); fprintf(out, "\n");
}


void Diagnostic::report(const Source_Manager *sm, FILE *out) {
    assert(labels.count() > 0 and labels[0].is_primary, "Compiler Bug: missing primary label");
    Label plabel = labels[0];
    const Source_File *file = sm->get_source(plabel.locus.source_id);
    fprintf(
        out,
         "%s:%zu:%zu: %s[%.3d]: %s\n",
         file->path.data(), plabel.locus.line, plabel.locus.begin,
         sevstr(severity),
         code,
         get_code_message(code)
    );
    fprintf(out, "\033[1mWhat happened\033[0m:\n");
    fprintf(out, "   \033[0;33m%.*s.\n\033[0m", SVARG(message.base()));

    fprintf(out, "\n\033[1mCode Snippet\033[0m:\n");

    usize max_line = 1;
    const usize label_count = labels.count();

    for (usize i = 0; i < label_count; ++i) {
        if (labels[i].locus.line > max_line) {
            max_line = labels[i].locus.line;
        }
    }

    usize gutter = 1;
    for (usize temp = max_line; temp >= 10; temp /= 10) {
        gutter++;
    }

    std::sort(labels.data(), labels.data() + label_count, sort_label);
    for (usize n = 0; n < label_count; n++) {
        Label label = labels[n];
        print_label(label, sm, out, gutter);
    }

    fprintf(out, "\n");
}

Diagnostic_Pool::Diagnostic_Pool(Alloc allocator, usize w_max, usize e_max)
    : buffer(allocator), w_max(w_max), e_max(e_max) {}

void Diagnostic_Pool::report(Severity severity, Locus locus,
                             const char *message) {
    report(Diagnostic(buffer.allocator(), severity, locus, message));
}

void Diagnostic_Pool::report(Diagnostic diagnostic) {
    switch (diagnostic.severity) {
    case Severity::Error:
        if (e_count >= e_max) {
            return;
        }
        ++e_count;
        break;
    case Severity::Warning:
        if (w_count >= w_max) {
            return;
        }
        ++w_count;
        break;
    case Severity::Note:
        break;
    }
    buffer.append(std::move(diagnostic));
}

bool Diagnostic_Pool::has_errors() const { return e_count > 0; }

bool Diagnostic_Pool::is_empty() const { return buffer.count() == 0; }

bool Diagnostic_Pool::has_reached_max_errors() const
{
    return e_count >= e_max;
}
