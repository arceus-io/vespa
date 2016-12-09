// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <vespa/fastos/fastos.h>
#include "traits.h"
#include "array.h"

namespace vespalib {
namespace stash {

struct Cleanup {
    Cleanup * const next;
    explicit Cleanup(Cleanup *next_in) noexcept : next(next_in) {}
    virtual void cleanup() = 0;
protected:
    virtual ~Cleanup() {}
};

// used as header for memory allocated outside the stash
struct DeleteMemory : public Cleanup {
    explicit DeleteMemory(Cleanup *next_in) noexcept : Cleanup(next_in) {}
    virtual void cleanup() override { free((void*)this); }
};

// used as prefix for objects to be destructed
template<typename T> struct DestructObject : public Cleanup {
    explicit DestructObject(Cleanup *next_in) noexcept : Cleanup(next_in) {}
    virtual void cleanup() override { reinterpret_cast<T*>(this + 1)->~T(); }
};

// used as prefix for arrays to be destructed
template<typename T> struct DestructArray : public Cleanup {
    size_t size;
    explicit DestructArray(Cleanup *next_in, size_t size_in) noexcept
        : Cleanup(next_in), size(size_in) {}
    virtual void cleanup() override {
        T *array = reinterpret_cast<T*>(this + 1);
        for (size_t i = size; i-- > 0;) {
            array[i].~T();
        }
    }
};

struct Chunk {
    Chunk  *next;
    size_t  used;
    Chunk(const Chunk &) = delete;
    Chunk(Chunk *next_in) : next(next_in), used(sizeof(Chunk)) {}
    void clear() { used = sizeof(Chunk); }
    char *alloc(size_t size, size_t chunk_size) {
        size_t aligned_size = ((size + (sizeof(char *) - 1))
                              & ~(sizeof(char *) - 1));
        if (used + aligned_size > chunk_size) {
            return nullptr;
        }
        char *ret = reinterpret_cast<char*>(this) + used;
        used += aligned_size;
        return ret;
    }
};

} // namespace vespalib::stash

/**
 * @brief A Stash stores mixed typed objects next to each other in
 * memory.
 *
 * When a stash is destructed, destruction of internal objects will be
 * performed in reverse creation order. Objects for which the
 * vespalib::can_skip_destruction trait is true are not
 * destructed. This will save both time and space.
 *
 * The minimal chunk size of a stash is 4k. Any object larger than 1/4
 * of the chunk size will be allocated separately.
 **/
class Stash
{
private:
    stash::Chunk   *_chunks;
    stash::Cleanup *_cleanup;
    size_t          _chunk_size;

    char *do_alloc(size_t size);
    bool is_small(size_t size) const { return (size < (_chunk_size / 4)); }

    template <typename T, typename ... Args>
    T *init_array(char *mem, size_t size, Args && ... args) {
        T *array = reinterpret_cast<T*>(mem);
        for (size_t i = 0; i < size; ++i) {
            new (array + i) T(std::forward<Args>(args)...);
        }
        return array;
    }

    template <typename T>
    T *copy_elements(char *mem, ConstArrayRef<T> src) {
        T *array = reinterpret_cast<T*>(mem);
        for (size_t i = 0; i < src.size(); ++i) {
            new (array + i) T(src[i]);
        }
        return array;
    }

public:
    typedef std::unique_ptr<Stash> UP;
    explicit Stash(size_t chunk_size);
    Stash() : Stash(4096) {}
    Stash(Stash &&rhs);
    Stash(const Stash &) = delete;
    Stash & operator = (const Stash &) = delete;
    ~Stash();

    Stash &operator=(Stash &&rhs);

    void clear();

    size_t count_used() const;
    size_t get_chunk_size() const { return _chunk_size; }

    char *alloc(size_t size) {
        char *ret = __builtin_expect(is_small(size) && _chunks != nullptr, true)
                    ? _chunks->alloc(size, _chunk_size) : nullptr;
        return __builtin_expect(ret != nullptr, true) ? ret : do_alloc(size);
    }

    template <typename T, typename ... Args>
    T &create(Args && ... args) {
        if (can_skip_destruction<T>::value) {
            return *(new (alloc(sizeof(T))) T(std::forward<Args>(args)...));
        }
        using DeleteHook = stash::DestructObject<T>;
        char *mem = alloc(sizeof(DeleteHook) + sizeof(T));
        T *ret = new (mem + sizeof(DeleteHook)) T(std::forward<Args>(args)...);
        _cleanup = new (mem) DeleteHook(_cleanup);
        return *ret;
    }

    template <typename T, typename ... Args>
    ArrayRef<T> create_array(size_t size, Args && ... args) {
        if (can_skip_destruction<T>::value) {
            return ArrayRef<T>(init_array<T, Args...>(alloc(size * sizeof(T)), size, std::forward<Args>(args)...), size);
        }
        using DeleteHook = stash::DestructArray<T>;
        char *mem = alloc(sizeof(DeleteHook) + (size * sizeof(T)));
        T *ret = init_array<T, Args...>(mem + sizeof(DeleteHook), size, std::forward<Args>(args)...);
        _cleanup = new (mem) DeleteHook(_cleanup, size);
        return ArrayRef<T>(ret, size);
    }

    template <typename T>
    ArrayRef<T> copy_array(ConstArrayRef<T> src) {
        if (can_skip_destruction<T>::value) {
            return ArrayRef<T>(copy_elements<T>(alloc(src.size() * sizeof(T)), src), src.size());
        }
        using DeleteHook = stash::DestructArray<T>;
        char *mem = alloc(sizeof(DeleteHook) + (src.size() * sizeof(T)));
        T *ret = copy_elements<T>(mem + sizeof(DeleteHook), src);
        _cleanup = new (mem) DeleteHook(_cleanup, src.size());
        return ArrayRef<T>(ret, src.size());
    }
};

} // namespace vespalib
