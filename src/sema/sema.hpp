#pragma once

#include "misc/map.hpp"
#include "sema/entities/id.hpp"
#include "sema/entities/scope.hpp"
#include "sema/store.hpp"
#include "sema/worker.hpp"
#include "source.hpp"
#include "compiler.hpp"
#include "sema/sema_id.hpp"
#include "ast/statement.hpp"
#include "sema/symbol_proxy.hpp"

namespace sema
{
    using Waiting_Map = HashMap<Symbol_Id, Arr<Worker>>;

    struct Semantic_Context {
        Semantic_Id       id;
        Source_Id         source_id;
        Compiler_Context *compiler;
        Semantic_Storage *store;
        Alloc             allocator;
        Waiting_Map       workers;

        Scope_Id global_scope;
        Scope_Id current_scope;

        Opt<Symbol_Id> current_namespace = None;

        Semantic_Context(Semantic_Id id, Source_Id source_id, Compiler_Context *compiler);

        bool analyze();
        void add_worker(Symbol_Id symbol_id, Worker worker);

        //----------------------scope utilities----------------------
        Scope_Id enter_scope(Scope::Kind kind = Scope::Block);
        void exit_scope();
        auto find_last_scope_of_kind(Scope::Kind kind, auto &&fn)
            -> Opt<decltype(fn(this, (Scope*)nullptr))>
        {
            Opt<Scope_Id> current = current_scope;
            while (current.has_value()) {
                auto &scope = store->scopes.at_index(usize(*current));
                if (scope.kind == kind) return fn(this, &scope);
                current = scope.parent;
            }
            return None;
        }
        //----------------------================----------------------

        //----------------------symbol utilities----------------------
        Symbol_Id register_symbol(Scope_Id scope_id, Symbol_Ptr symbol_ptr);
        Opt<Symbol_Proxy> get_symbol_at_locus(Locus locus);
        Symbol_Proxy get_symbol_at_locus_unchecked(Locus locus); /* panics */
        void set_symbol_at_locus(Locus locus, Symbol_Id symbol_id);
        //----------------------================----------------------

        //----------------------type utilities----------------------
        Type_Id get_type_at_locus(Locus locus);
        void set_type_at_locus(Locus locus, Type_Id type_Id);
        Type_Id register_or_insert_type(Type *type);
        //----------------------==============----------------------

        Opt<Symbol_Proxy> find_symbol(Sv name);
        Opt<Symbol_Proxy> find_symbol_in(Sv name, Scope_Id scope_id);

        Opt<Symbol_Proxy> eagerly_find_symbol(Sv name);
        Opt<Symbol_Proxy> eagerly_find_symbol_in(Sv name, Scope_Id scope_id);

        bool discover_statement(ast::Statement *stmt);
        bool discover_variable_assignment(ast::Statement *stmt);
        bool discover_function_decl(ast::Statement *stmt);

        //----------------------------checkers----------------------------
        Worker_Status check_statement (Worker_Payload payload, bool is_resumption);
        Worker_Status check_expression(Worker_Payload payload, bool is_resumption);

        Worker_Status check_variable_declaration(ast::Statement *stmt, bool is_resumption);
        Worker_Status check_function_declaration(ast::Statement *stmt, bool is_resumption);
        Worker_Status check_block_statement(ast::Statement *stmt, bool is_resumption);
        Worker_Status check_if_statement(ast::Statement *stmt, bool is_resumption);
        Worker_Status check_return_statement(ast::Statement *stmt, bool is_resumption);
        //----------------------------========----------------------------

    };

} // namespace sema
