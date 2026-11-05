#include "ast/expressions.hpp"

using namespace ast;
using namespace ast::expr;

Call_Argument::Call_Argument(Expression *value, Opt<Name> name)
    : value(value), name(name)
{
}

Function_Call::Function_Call(Locus locus, Expression *callee, Args args)
    : Expression(locus, Expression_Kind::Function_Call),
      callee(callee),
      args(args)
{
}
