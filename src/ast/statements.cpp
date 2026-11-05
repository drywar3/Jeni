#include "ast/statements.hpp"

using namespace ast;
using namespace ast::stmt;

Function_Declaration::Function_Declaration(
    Locus locus,
    Name name,
    Function_Prototype proto,
    Statement *body
) : Statement(locus, Statement_Kind::Function),
    name(name),
    prototype(proto),
    body(body)
{
}

Block::Block(Locus locus, Body body)
    : Statement(locus, Statement_Kind::Block),
      body(body)
{
}

If_Statement::If_Statement(Locus locus, If_Header then_branch, Branches branches, Statement *else_branch)
    : Statement(locus, Statement_Kind::If_Statement),
      then_branch(then_branch),
      branches(branches),
      else_branch(else_branch)
{
}

Return::Return(Locus locus, Expression *value)
    : Statement(locus, Statement_Kind::Return), value(value)
{
}
