#pragma once

#include "vixen/allocator/allocator.hpp"

namespace vixen {

struct uninitialized_tag_t {};
constexpr uninitialized_tag_t uninitialized_tag{};

/// @ingroup vixen_data_structures
/// @brief Managed pointer that destroys its allocation at the end of its lifetime.
template <typename T>
struct Unique {
    Unique() = default;

    template <typename... Args>
    Unique(uninitialized_tag_t, Allocator &alloc)
        : data(heap::createUninit<T>(alloc)), alloc(&alloc) {}

    template <typename... Args>
    Unique(Allocator &alloc, Args &&...args)
        : data(heap::createInit<T>(alloc, std::forward<Args>(args)...)), alloc(&alloc) {}

    Unique(copy_tag_t, Allocator &alloc, const Unique &other)
        : data(heap::createInit<T>(alloc, copyAllocate(alloc, *other.data))), alloc(&alloc) {}

    VIXEN_DEFINE_CLONE_METHOD(Unique);

    Unique(Unique const &other) = delete;
    Unique &operator=(Unique const &other) = delete;

    template <typename U, require<std::is_convertible<U *, T *>> = true>
    Unique(Unique<U> &&other)
        : data(util::exchange(other.data, nullptr)), alloc(util::exchange(other.alloc, nullptr)) {}

    template <typename U, require<std::is_convertible<U *, T *>> = true>
    Unique &operator=(Unique<U> &&other) {
        if (static_cast<void *>(this) == static_cast<void *>(&other)) return *this;
        if (data) {
            heap::destroyInit(alloc, data);
        }
        data = util::exchange(other.data, nullptr);
        alloc = util::exchange(other.alloc, nullptr);
        return *this;
    }

    ~Unique() {
        if (data) {
            heap::destroyInit(alloc, data);
        }
    }

    // clang-format off
    const T &operator*() const { return *data; }
    const T *operator->() const { return data; }
    T &operator*() { return *data; }
    T *operator->() { return data; }

    explicit operator bool() const { return data != nullptr; }
    operator T const *() const { return data; }
    operator T *() { return data; }

    T const &get() const { return *data; }
    T &get() { return *data; }
    // clang-format on

    void clear() {
        if (data) {
            heap::destroyInit(alloc, data);
            data = nullptr;
        }
    }

    template <typename S>
    S &operator<<(S &s) const {
        s << **this;
        return s;
    }

private:
    T *data = nullptr;
    Allocator *alloc = nullptr;

    // this is asinine.
    template <typename U>
    friend struct Unique;
};

template <typename T, typename H>
inline void hash(const Unique<T> &unique, H &hasher) {
    hash(unique.get(), hasher);
}

template <typename T, typename... Args>
Unique<T> makeUnique(Allocator &alloc, Args &&...args) {
    return Unique<T>(alloc, std::forward<Args>(args)...);
}

template <typename T>
Unique<T> makeUniqueUninitialized(Allocator &alloc) {
    return Unique<T>(uninitialized_tag, alloc);
}

} // namespace vixen
