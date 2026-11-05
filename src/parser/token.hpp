#pragma once

#include "parser/locus.hpp"

namespace parser
{
    enum Token_Kind {
        Operator_Start,
            OP_Add,
            OP_Sub,
            OP_Star,
            OP_Slash,
            OP_Inc,
            OP_Dec,
            OP_Assign,
            OP_Equals,
            OP_Not_Equals,
            OP_Arrow,
            OP_Bang,
            OP_Caret,
            OP_Mod,
            OP_Less,                // <
            OP_Less_Equals,         // <=
            OP_Greater,             // >
            OP_Greater_Equals,      // >=
        Operator_End,

        Type_Keyword_Start,
            TK_Int,
            TK_Uint,
            TK_Char,
            TK_Uchar,
            TK_Void,
            TK_Bool,
            TK_Int8,
            TK_Int16,
            TK_Int32,
            TK_Int64,
            TK_Isize,
            TK_Uint8,
            TK_Uint16,
            TK_Uint32,
            TK_Uint64,
            TK_Usize,
            TK_Float32,
            TK_Float64,
        Type_Keyword_End,

        Keyword_Start,
            KW_Func,
            KW_Import,
            KW_Use,
            KW_Using,
            KW_Move,
            KW_Cast,
            KW_Return,
            KW_Continue,
            KW_Break,
            KW_Defer,
            KW_Var,
            KW_Const,
            KW_Eval,
            KW_If,
            KW_For,
            KW_Else,
            KW_Then,
            KW_Struct,
            KW_Enum,
            KW_In,
            KW_Namespace,
            KW_Type,
        Keyword_End,

        Separator_Start,
            SEP_Lparen,
            SEP_Lbrace,
            SEP_Lbracket,
            SEP_Rparen,
            SEP_Rbrace,
            SEP_Rbracket,
            SEP_Comma,
            SEP_Semi_Colon,
            SEP_Colon,
            SEP_Vararg,
            SEP_Turbo_Fish,
            SEP_Dot,
        Separator_End,

        Directive_Start,
            DR_Extern,
            DR_Canon_Name,
            DR_Inline,
            DR_No_Inline,
        Directive_End,

        Literal_Start,
            LIT_String,
            LIT_Cstring,
            LIT_True,
            LIT_False,
            LIT_Null,
            LIT_Integer,
            LIT_Float,
            LIT_Char,
        Literal_End,

        Identifier,
        End_Of_File,
        Error,
    };

    const char *token_to_string(Token_Kind kind);

    struct Token {
        Token_Kind kind;
        Locus      locus;
        usize      base;

        Token() : kind(Error), locus(0, 0, 0, 0, 0, Source_Id(0)) {}
        Token(Token_Kind kind, Locus locus, usize base = 10) : kind(kind), locus(locus), base(base) {}

        bool is_operator() const;
        bool is_type_keyword() const;
        bool is_regular_keyword() const;
        bool is_separator() const;
        bool is_directive() const;
    };
} // namespace parser
