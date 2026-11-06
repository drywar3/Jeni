#pragma once

#include <cstddef>          // std::size_t, std::nullptr_t
#include <cstdint>          // std::uintptr_t, std::uint64_t
#include <cstring>          // std::memset
#include <initializer_list> // std::initializer_list
#include <new>              // placement new
#include <type_traits>      // std::conditional
#include <utility>          // std::move, std::forward, std::swap, std::pair

#include <mini.c/allocator.h>
#include <mini.c/default_allocator.h>
#include <mini.c/map_defs.h>
#include <mini.c/mini_def.h>
#include <mini.cc/string_view.h>
#include <mini.cc/string.h>

// ============================================================================
// General Purpose FNV-1a Hash Functor Template
// ============================================================================

template <typename T> struct Hash {
    std::size_t operator()(const T &key) const
    {
        const unsigned char *bytes =
            reinterpret_cast<const unsigned char *>(&key);
        std::size_t hash = 14695981039346656037ULL;
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            hash ^= bytes[i];
            hash *= 1099511628211ULL;
        }
        return hash;
    }
};

template <typename T> struct Hash<T *> {
    std::size_t operator()(T *ptr) const
    {
        std::uintptr_t val = reinterpret_cast<std::uintptr_t>(ptr);
        return Hash<std::uintptr_t>{}(val);
    }
};

template <> struct Hash<const char *> {
    std::size_t operator()(const char *str) const
    {
        if (!str)
            return 0;
        std::size_t hash = 14695981039346656037ULL;
        while (*str) {
            hash ^= static_cast<unsigned char>(*str++);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
};

template <> struct Hash<mini::String> {
    std::size_t operator()(const mini::String& s) const
    {
        return Hash<const char*>()(s.data());
    }
};


/* hash helper function combining field hashes using bit-mixing */
inline void hash_combine(std::size_t &seed, std::size_t value)
{
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

static inline usize mini_strview_hash(const Mini_StringView *s)
{
    if (!s || !s->data) return 0;
    std::size_t hash = 14695981039346656037ULL;
    for (std::size_t i = 0; i < s->length; ++i) {
        hash ^= static_cast<unsigned char>(s->data[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

template <> struct Hash<Mini_StringView> {
    std::size_t operator()(const Mini_StringView &s) const noexcept
    {
        return mini_strview_hash(&s);
    }
};

// template <> struct std::hash<mini::StringView> {
//     std::size_t operator()(const mini::StringView &s) const noexcept
//     {
//         return mini_strview_hash(&s.base());
//     }
// };

inline bool operator==(const Mini_StringView &a, const Mini_StringView &b)
{
    return mini_sv_equals(a, b);
}

template <> struct Hash<mini::StringView> {
    std::size_t operator()(const mini::StringView &s) const noexcept
    {
        return mini_strview_hash(&s.base());
    }
};

// ============================================================================
// HashMap Class
// ============================================================================

template <typename Key, typename Value, typename HashFunc = Hash<Key>>
class HashMap {
  public:
    struct Node {
        Key key;
        Value value;
        Node *next;

        template <typename K, typename V>
        Node(K &&k, V &&v, Node *n = nullptr)
            : key(std::forward<K>(k)), value(std::forward<V>(v)), next(n)
        {
        }
    };

    template <bool IsConst> class IteratorImpl {
      private:
        using MapPtr = typename std::conditional<IsConst, const HashMap *,
                                                 HashMap *>::type;
        using NodePtr =
            typename std::conditional<IsConst, const Node *, Node *>::type;
        using NodeRef =
            typename std::conditional<IsConst, const Node &, Node &>::type;

        MapPtr map_;
        std::size_t bucket_;
        NodePtr node_;

        void advance_to_valid()
        {
            if (!map_ || !map_->buckets_)
                return;
            while (node_ == nullptr && ++bucket_ < map_->capacity_) {
                node_ = map_->buckets_[bucket_];
            }
        }

      public:
        IteratorImpl(MapPtr map, std::size_t bucket, NodePtr node)
            : map_(map), bucket_(bucket), node_(node)
        {
            if (node_ == nullptr && map_ && map_->buckets_ &&
                bucket_ < map_->capacity_) {
                advance_to_valid();
            }
        }

        // Allow implicit conversion from non-const Iterator to ConstIterator
        template <bool OtherIsConst,
                  typename = std::enable_if_t<IsConst && !OtherIsConst>>
        IteratorImpl(const IteratorImpl<OtherIsConst> &other)
            : map_(other.map_), bucket_(other.bucket_), node_(other.node_)
        {
        }

        NodeRef operator*() const { return *node_; }
        NodePtr operator->() const { return node_; }

        IteratorImpl &operator++()
        {
            if (node_) {
                node_ = node_->next;
                if (!node_) {
                    advance_to_valid();
                }
            }
            return *this;
        }

        template <bool OtherIsConst>
        bool operator==(const IteratorImpl<OtherIsConst> &other) const
        {
            return node_ == other.node_ && bucket_ == other.bucket_ &&
                   map_ == other.map_;
        }

        template <bool OtherIsConst>
        bool operator!=(const IteratorImpl<OtherIsConst> &other) const
        {
            return !(*this == other);
        }

        friend class IteratorImpl<true>;
    };

    using Iterator      = IteratorImpl<false>;
    using ConstIterator = IteratorImpl<true>;

  private:
    Node **buckets_;
    std::size_t capacity_;
    std::size_t size_;
    float max_load_factor_;
    HashFunc hasher_;
    Mini_Allocator allocator_;

    std::size_t get_bucket_index(const Key &key, std::size_t cap) const
    {
        return hasher_(key) % cap;
    }

    void check_and_rehash()
    {
        if (capacity_ == 0 || !buckets_) {
            rehash(16);
            return;
        }
        if (static_cast<float>(size_ + 1) / capacity_ > max_load_factor_) {
            rehash(capacity_ * 2);
        }
    }

  public:
    explicit HashMap(std::size_t initial_capacity = 16,
                     float max_load_factor        = 0.75f,
                     Mini_Allocator allocator     = mini_default_allocator())
        : capacity_(initial_capacity < 4 ? 4 : initial_capacity), size_(0),
          max_load_factor_(max_load_factor), hasher_(HashFunc()),
          allocator_(allocator)
    {
        buckets_ = MINI_ALLOC_MANY(allocator_, Node *, capacity_);
        std::memset(buckets_, 0, sizeof(Node *) * capacity_);
    }

    explicit HashMap(Mini_Allocator allocator,
                     std::size_t initial_capacity = 16,
                     float max_load_factor        = 0.75f)
        : HashMap(initial_capacity, max_load_factor, allocator)
    {
    }

    HashMap(std::initializer_list<std::pair<Key, Value>> list,
            Mini_Allocator allocator = mini_default_allocator())
        : HashMap(list.size() * 2, 0.75f, allocator)
    {
        for (const auto &item : list) {
            insert(item.first, item.second);
        }
    }

    ~HashMap()
    {
        clear();
        if (buckets_) {
            MINI_FREE(allocator_, buckets_);
        }
    }

    HashMap(const HashMap &other)
        : capacity_(other.capacity_ < 4 ? 4 : other.capacity_), size_(0),
          max_load_factor_(other.max_load_factor_), hasher_(other.hasher_),
          allocator_(other.allocator_)
    {
        buckets_ = MINI_ALLOC_MANY(allocator_, Node *, capacity_);
        std::memset(buckets_, 0, sizeof(Node *) * capacity_);
        for (std::size_t i = 0; i < other.capacity_; ++i) {
            if (!other.buckets_)
                break;
            Node *current = other.buckets_[i];
            while (current) {
                insert(current->key, current->value);
                current = current->next;
            }
        }
    }

    HashMap &operator=(const HashMap &other)
    {
        if (this != &other) {
            HashMap temp(other);
            swap(temp);
        }
        return *this;
    }

    HashMap(HashMap &&other) noexcept
        : buckets_(other.buckets_), capacity_(other.capacity_),
          size_(other.size_), max_load_factor_(other.max_load_factor_),
          hasher_(std::move(other.hasher_)), allocator_(other.allocator_)
    {
        other.buckets_  = nullptr;
        other.capacity_ = 0;
        other.size_     = 0;
    }

    HashMap &operator=(HashMap &&other) noexcept
    {
        if (this != &other) {
            clear();
            if (buckets_) {
                MINI_FREE(allocator_, buckets_);
            }

            buckets_         = other.buckets_;
            capacity_        = other.capacity_;
            size_            = other.size_;
            max_load_factor_ = other.max_load_factor_;
            hasher_          = std::move(other.hasher_);
            allocator_       = other.allocator_;

            other.buckets_  = nullptr;
            other.capacity_ = 0;
            other.size_     = 0;
        }
        return *this;
    }

    void swap(HashMap &other) noexcept
    {
        std::swap(buckets_, other.buckets_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
        std::swap(max_load_factor_, other.max_load_factor_);
        std::swap(hasher_, other.hasher_);
        std::swap(allocator_, other.allocator_);
    }

    Value *find(const Key &key)
    {
        if (capacity_ == 0 || !buckets_)
            return nullptr;
        std::size_t idx = get_bucket_index(key, capacity_);
        Node *current   = buckets_[idx];

        while (current) {
            if (current->key == key) {
                return &current->value;
            }
            current = current->next;
        }

        return nullptr;
    }

    const Value *find(const Key &key) const
    {
        if (capacity_ == 0 || !buckets_)
            return nullptr;
        std::size_t idx = get_bucket_index(key, capacity_);
        Node *current   = buckets_[idx];

        while (current) {
            if (current->key == key) {
                return &current->value;
            }
            current = current->next;
        }
        return nullptr;
    }

    bool contains(const Key &key) const { return find(key) != nullptr; }

    Value &operator[](const Key &key)
    {
        check_and_rehash();
        std::size_t idx = get_bucket_index(key, capacity_);
        Node *current   = buckets_[idx];

        while (current) {
            if (current->key == key) {
                return current->value;
            }
            current = current->next;
        }

        Node *new_node_mem = MINI_ALLOC(allocator_, Node);
        Node *new_node = new (new_node_mem) Node(key, Value(), buckets_[idx]);
        buckets_[idx]  = new_node;
        ++size_;
        return new_node->value;
    }

    Value &operator[](Key &&key)
    {
        check_and_rehash();
        std::size_t idx = get_bucket_index(key, capacity_);
        Node *current   = buckets_[idx];

        while (current) {
            if (current->key == key) {
                return current->value;
            }
            current = current->next;
        }

        Node *new_node_mem = MINI_ALLOC(allocator_, Node);
        Node *new_node =
            new (new_node_mem) Node(std::move(key), Value(), buckets_[idx]);
        buckets_[idx] = new_node;
        ++size_;
        return new_node->value;
    }

    template <typename K, typename V> bool emplace(K &&key, V &&value)
    {
        check_and_rehash();
        std::size_t idx = get_bucket_index(key, capacity_);
        Node *current   = buckets_[idx];

        while (current) {
            if (current->key == key) {
                current->value = std::forward<V>(value);
                return false;
            }
            current = current->next;
        }

        Node *new_node_mem = MINI_ALLOC(allocator_, Node);
        Node *new_node     = new (new_node_mem)
            Node(std::forward<K>(key), std::forward<V>(value), buckets_[idx]);
        buckets_[idx] = new_node;
        ++size_;
        return true;
    }

    bool insert(const Key &key, const Value &value)
    {
        return emplace(key, value);
    }

    bool insert(Key &&key, Value &&value)
    {
        return emplace(std::move(key), std::move(value));
    }

    bool erase(const Key &key)
    {
        if (capacity_ == 0 || !buckets_)
            return false;
        std::size_t idx = get_bucket_index(key, capacity_);
        Node *current   = buckets_[idx];
        Node *prev      = nullptr;
        while (current) {
            if (current->key == key) {
                if (prev) {
                    prev->next = current->next;
                } else {
                    buckets_[idx] = current->next;
                }
                current->~Node();
                MINI_FREE(allocator_, current);
                --size_;
                return true;
            }
            prev    = current;
            current = current->next;
        }
        return false;
    }

    void clear()
    {
        if (!buckets_)
            return;

        for (std::size_t i = 0; i < capacity_; ++i) {
            Node *current = buckets_[i];

            while (current) {
                Node *next = current->next;

                current->~Node();
                MINI_FREE(allocator_, current);

                current = next;
            }

            buckets_[i] = nullptr;
        }

        size_ = 0;
    }

    void rehash(std::size_t new_capacity)
    {
        if (new_capacity < size_)
            new_capacity = size_;
        if (new_capacity < 4)
            new_capacity = 4;

        Node **new_buckets = MINI_ALLOC_MANY(allocator_, Node *, new_capacity);
        std::memset(new_buckets, 0, sizeof(Node *) * new_capacity);

        for (std::size_t i = 0; i < capacity_; ++i) {
            if (!buckets_)
                break;
            Node *current = buckets_[i];
            while (current) {
                Node *next          = current->next;
                std::size_t new_idx = hasher_(current->key) % new_capacity;

                current->next        = new_buckets[new_idx];
                new_buckets[new_idx] = current;

                current = next;
            }
        }

        if (buckets_) {
            MINI_FREE(allocator_, buckets_);
        }
        buckets_  = new_buckets;
        capacity_ = new_capacity;
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    Iterator begin()
    {
        if (size_ == 0 || !buckets_)
            return end();
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (buckets_[i])
                return Iterator(this, i, buckets_[i]);
        }
        return end();
    }

    Iterator end() { return Iterator(this, capacity_, nullptr); }

    ConstIterator begin() const
    {
        if (size_ == 0 || !buckets_)
            return end();
        for (std::size_t i = 0; i < capacity_; ++i) {
            if (buckets_[i])
                return ConstIterator(this, i, buckets_[i]);
        }
        return end();
    }

    ConstIterator end() const
    {
        return ConstIterator(this, capacity_, nullptr);
    }
    ConstIterator cbegin() const { return begin(); }
    ConstIterator cend() const { return end(); }
};
