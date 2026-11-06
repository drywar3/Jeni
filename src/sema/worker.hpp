#pragma once

#include "ast/expression.hpp"
#include "ast/statement.hpp"
#include "mini.c/mini_def.h"
#include "misc/misc.hpp"
#include "sema/entities/id.hpp"
#include "sema/sema_id.hpp"
#include "sema/store.hpp"

namespace sema
{
    struct Semantic_Context;

    struct Worker_Status {
        enum Status {
            // resolution failed due to error.
            Failed,
            // resolution completed successfully.
            Done,
            // resolution could not be completed.
            Pending,
        };

        Status         status;
        Scope_Id       working_scope;
        Arr<Symbol_Id> waiting_on;

        Worker_Status(Status s) : status(s) {}

        static Worker_Status pending(Alloc a, Symbol_Id waiting_for, Scope_Id scope_id)
        {
            auto status = Worker_Status(Pending);
            status.waiting_on = Arr<Symbol_Id>(a);
            status.waiting_on.append(waiting_for);
            status.working_scope = scope_id;
            return status;
        }

        static Worker_Status pending(Alloc a)
        {
            auto status = Worker_Status(Pending);
            status.waiting_on = Arr<Symbol_Id>(a);
            return status;
        }


        operator Status() const { return status; }

        operator bool() const { return status == Done; }
    };

    // struct Worker_Payload {
    //     enum Kind {
    //         Statement,
    //         Expression,
    //     };
    //
    //     Kind kind;
    //     void *data;
    //
    //     Worker_Payload(ast::Statement **stmt)
    //         : kind(Statement), data(stmt) {}
    //
    //     Worker_Payload(ast::Expression **expr)
    //         : kind(Expression), data(expr) {}
    //
    //     ast::Expression **get_expr()
    //     {
    //         MINI_ASSERT(kind == Expression, "payload is not an expression");
    //         return static_cast<ast::Expression**>(data);
    //     }
    //
    //     ast::Statement **get_stmt()
    //     {
    //         MINI_ASSERT(kind == Statement, "payload is not an statement");
    //         return static_cast<ast::Statement**>(data);
    //     }
    // };

    using Worker_Function =
        auto (*) (Semantic_Context&, void*, bool is_resumption) -> Worker_Status;

    struct Worker {
        Semantic_Id semantic_id;
        Scope_Id    scope_id;
        void *payload;
        Worker_Function function;

        Worker(Semantic_Id sema_id,
               Scope_Id scope_id,
               void *payload,
               Worker_Function function)
            : semantic_id(sema_id), scope_id(scope_id), payload(payload),
              function(function)
        {}

        Worker_Status resume(Semantic_Storage *store);
    };
}
