#include "sema/worker.hpp"
#include "sema/sema.hpp"

using namespace sema;

Worker_Status Worker::resume(Semantic_Storage *store)
{
    auto &sema = store->semas.get(semantic_id);
    auto save = sema.current_scope;
    sema.current_scope = scope_id;
    auto status = function(sema,payload,true);
    sema.current_scope = save;
    return status;
}
