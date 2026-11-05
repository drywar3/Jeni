#pragma once

#include <optional>
#include <utility>
#include <mini.cc/array.h>

#include "misc/map.hpp"

template <typename Key, typename Value> using Map = HashMap<Key, Value>;

template <typename Key, typename Value> struct DenseMap {
  private:
    Map<Key, std::size_t> key_to_index_;
    mini::Array<Value> values_;
    Mini_Allocator allocator_;

  public:
    DenseMap(Mini_Allocator allocator = mini_default_allocator())
        : values_(allocator), allocator_(allocator)
    {
    }

    ~DenseMap() {  }

    DenseMap(const DenseMap &) = delete;
    DenseMap &operator=(const DenseMap &) = delete;

    DenseMap(DenseMap &&other) noexcept
        : key_to_index_(std::move(other.key_to_index_)),
          values_(std::move(other.values_)),
          allocator_(other.allocator_)
    {
    }

    DenseMap &operator=(DenseMap &&other) noexcept
    {
        if (this == &other) return *this;
        key_to_index_ = std::move(other.key_to_index_);
        values_ = std::move(other.values_);
        allocator_ = other.allocator_;
        return *this;
    }
    /* --- insert / lookup --- */

    std::size_t insert(const Key &key, Value value)
    {
        if (auto index = get_id(key)) {
            values_[*index] = std::move(value);
            return *index;
        }
        std::size_t new_index = values_.count();
        key_to_index_[key] = new_index;
        values_.append(std::move(value));
        return new_index;
    }

    bool contains(const Key &key) const
    {
        return key_to_index_.find(key) != nullptr;
    }

    std::optional<std::size_t> get_id(const Key &key) const
    {
        auto it = key_to_index_.find(key);
        if (it != nullptr) {
            return *it;
        }
        return std::nullopt;
    }

    Value *find(const Key &key)
    {
        auto it = key_to_index_.find(key);
        return (it != nullptr) ? &values_[*it] : nullptr;
    }

    const Value *find(const Key &key) const
    {
        auto it = key_to_index_.find(key);
        return (it != nullptr) ? &values_[*it] : nullptr;
    }

    std::size_t add_value(Value value)
    {
        std::size_t current_index = values_.count();
        values_.append(std::move(value));
        return current_index;
    }

    void link(const Key &key, std::size_t index) { key_to_index_[key] = index; }

    /* --- direct index access --- */

    Value &operator[](std::size_t index) { return values_[index]; }
    const Value &operator[](std::size_t index) const { return values_[index]; }

    std::size_t operator[](const Key &key) const { return *key_to_index_.find(key); }

    Value &at_index(std::size_t index) { return values_[index]; }
    const Value &at_index(std::size_t index) const { return values_[index]; }

    Value *at_index_ptr(std::size_t index) { return &values_[index]; }
    const Value *at_index_ptr(std::size_t index) const
    {
        return &values_[index];
    }

    /* --- capacity & iteration --- */

    std::size_t size() const { return values_.size(); }
    bool empty() const { return size() == 0; }
    void clear()
    {
        key_to_index_.clear();
        values_.clear();
    }

    auto begin()       { return values_.iter().begin(); }
    auto end()         { return values_.iter().end(); }
    auto begin() const { return values_.iter().begin(); }
    auto end() const   { return values_.iter().end(); }

};
