#pragma once

#include "traits.hpp"
#include "vixen/allocator/allocator.hpp"

namespace vixen {

struct uninitialized_tag_t {};
constexpr uninitialized_tag_t uninitialized_tag{};

/// @ingroup vixen_data_structures
/// @brief Managed pointer that destroys its allocation at the end of its lifetime.
template <typename T>
struct Unique {
    static_assert(std::is_nothrow_destructible_v<T>);
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);

    Unique() = default;

    template <typename... Args>
    Unique(Allocator &alloc, Args &&...args)
        : mData(heap::createInit<T>(alloc, std::forward<Args>(args)...)), mAlloc(&alloc) {}

    Unique(copy_tag_t, Allocator &alloc, Unique const &other)
        : mData(heap::createInit<T>(alloc, copyAllocate(alloc, *other.mData))), mAlloc(&alloc) {}
    VIXEN_DEFINE_CLONE_METHOD(Unique)

    Unique(Unique const &other) = delete;
    Unique &operator=(Unique const &other) = delete;

    template <typename U>
    Unique(Unique<U> &&other) noexcept requires IsConvertibleFrom<T *, U *>
        : mData(util::exchange(other.mData, nullptr))
        , mAlloc(util::exchange(other.mAlloc, nullptr)) {}

    template <typename U>
    Unique &operator=(Unique<U> &&other) noexcept requires IsConvertibleFrom<T *, U *> {
        if (static_cast<void *>(this) == static_cast<void *>(&other)) return *this;

        if (mData) heap::destroyInit(mAlloc, mData);

        mData = util::exchange(other.mData, nullptr);
        mAlloc = util::exchange(other.mAlloc, nullptr);
        return *this;
    }

    ~Unique() {
        if (mData) heap::destroyInit(*mAlloc, mData);
    }

    // clang-format off
    const T &operator*() const noexcept { return *mData; }
    const T *operator->() const noexcept { return mData; }
    T &operator*() noexcept { return *mData; }
    T *operator->() noexcept { return mData; }

    explicit operator bool() const noexcept { return mData != nullptr; }
    operator T const *() const noexcept { return mData; }
    operator T *() noexcept { return mData; }

    T const &get() const noexcept { return *mData; }
    T &get() noexcept { return *mData; }
    // clang-format on

    void release() noexcept {
        if (mData) {
            heap::destroyInit(*mAlloc, mData);
            mData = nullptr;
        }
    }

    template <typename S>
    S &operator<<(S &s) const {
        s << **this;
        return s;
    }

private:
    T *mData = nullptr;
    Allocator *mAlloc = nullptr;

    // this is asinine.
    template <typename U>
    friend struct Unique;
};

template <typename T, typename H>
inline void hash(const Unique<T> &unique, H &hasher) noexcept {
    hash(unique.get(), hasher);
}

template <typename T, typename... Args>
Unique<T> makeUnique(Allocator &alloc, Args &&...args) {
    return Unique<T>(alloc, std::forward<Args>(args)...);
}

} // namespace vixen
