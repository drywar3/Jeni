#include "parser/token.hpp"

using namespace parser;

const char *parser::token_to_string(Token_Kind kind)
{
    switch (kind) {
    case KW_Var: return "var";
    case KW_Const: return "const";
    case KW_Func: return "func";

    case TK_Int: return "int";

    case SEP_Lbrace: return "{";
    case SEP_Rbrace: return "}";
    case SEP_Lparen: return "(";
    case SEP_Rparen: return ")";
    case SEP_Lbracket: return "[";
    case SEP_Rbracket: return "]";
    case SEP_Semi_Colon: return ";";
    case SEP_Colon: return ":";
    case SEP_Comma: return ",";
    case SEP_Vararg: return "...";

    case OP_Star: return "*";
    case OP_Assign: return "=";

    case LIT_True: return "true";
    case LIT_False: return "false";
    case LIT_Integer: return "integer";

    case End_Of_File: return "**eof**";
    case Identifier:  return "identifier";
    default: MINI_UNREACHABLE("%d", kind);
    }
}
