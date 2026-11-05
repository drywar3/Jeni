#pragma once

#include <mini.cc/array.h>

template<typename T, typename Id = usize>
struct Id_Allocator {
    mini::Array<T> pool;

    Id_Allocator() : pool() {}
    Id_Allocator(Mini_Allocator allocator) : pool(allocator) {}

    Id allocate(T value)
    {
        usize current = pool.count();
        pool.append(value);
        return Id(current);
    }

    T &allocate_and_get(T value)
    {
        auto id = allocate(value);
        return get(id);
    }

    T *allocate_and_get_ptr(T value)
    {
        auto id = allocate(value);
        return get_ptr(id);
    }

    T &get(Id id)
    {
        MINI_ASSERT(usize(id) < pool.count(), "invalid index of size [%zu]. count = [%zu]", id, pool.count());
        return pool[usize(id)];
    }

    const T &get(Id id) const
    {
        MINI_ASSERT(usize(id) < pool.count(), "invalid index of size [%zu]. count = [%zu]", id, pool.count());
        return pool[usize(id)];
    }

    T *get_ptr(Id id)
    {
        MINI_ASSERT(usize(id) < pool.count(), "invalid index of size [%zu]. count = [%zu]", id, pool.count());
        return &pool[usize(id)];
    }

    const T *get_ptr(Id id) const
    {
        MINI_ASSERT(usize(id) < pool.count(), "invalid index of size [%zu]. count = [%zu]", id, pool.count());
        return &pool[usize(id)];
    }

    Id current_id() const
    {
        return Id(pool.count());
    }
};
