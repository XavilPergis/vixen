#pragma once

#include "vixen/assert.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include "erased_storage.hpp"

namespace vixen {

struct EmptyOptionType {};
constexpr EmptyOptionType EMPTY_OPTION = EmptyOptionType{};

template <typename T>
using remove_cvref_t = std::remove_reference_t<std::remove_cv_t<T>>;

template <typename... Conds>
using require = std::enable_if_t<std::conjunction_v<Conds...>, bool>;

// template <typename T>
// struct OptionStorage {
// protected:
//     OptionStorage() = default;
//     OptionStorage(bool occupied) noexcept : occupied(occupied) {}

//     void copyAssign(OptionStorage const &other) {
//         if (this->occupied && other.occupied) {
//             this->storage.get() = other.storage.get();
//         } else if (this->occupied) {
//             this->storage.destruct();
//         } else if (other.occupied) {
//             this->storage.constructInPlace(other.storage.get());
//         }
//     }

//     void moveAssign(OptionStorage &&other) noexcept {
//         if (this->occupied && other.occupied) {
//             this->storage.get() = mv(other.storage.get());
//         } else if (this->occupied) {
//             this->storage.destruct();
//         } else if (other.occupied) {
//             this->storage.constructInPlace(mv(other.storage.get()));
//         }
//     }

//     void destruct() noexcept {
//         if (occupied) storage.destruct();
//     }

//     bool occupied;
//     UninitializedStorage<T> storage;
// };

// template <typename T,
//     bool = has_trivial_destructor<T>,
//     bool = has_trivial_copy_ops<T>,
//     bool = has_trivial_move_ops<T>>
// struct OptionBase;

// template <typename T, bool C, bool M>
// struct OptionBase<T, false, C, M> : OptionBase<T, true, false, false> {
//     // Default constructor, initializes to unoccupied option.
//     OptionBase() noexcept : OptionBase<T, true, false, false>() {}
//     OptionBase(EmptyOptionType) noexcept : OptionBase<T, true, false, false>() {}

//     OptionBase(OptionBase const &other) = default;
//     OptionBase &operator=(OptionBase const &other) = default;
//     OptionBase(OptionBase &&other) = default;
//     OptionBase &operator=(OptionBase &&other) = default;

//     ~OptionBase() { this->destruct(); }
// };

// template <typename T>
// struct OptionBase<T, true, true, true> : OptionStorage<T> {
//     // Default constructor, initializes to unoccupied option.
//     OptionBase() noexcept : OptionStorage<T>(false) {}
//     OptionBase(EmptyOptionType) noexcept : OptionStorage<T>(false) {}

//     OptionBase(OptionBase const &other) = default;
//     OptionBase &operator=(OptionBase const &other) = default;

//     OptionBase(OptionBase &&other) = default;
//     OptionBase &operator=(OptionBase &&other) = default;
// };

// template <typename T>
// struct OptionBase<T, true, true, false> : OptionStorage<T> {
//     // Default constructor, initializes to unoccupied option.
//     OptionBase() noexcept : OptionStorage<T>(false) {}
//     OptionBase(EmptyOptionType) noexcept : OptionStorage<T>(false) {}

//     OptionBase(OptionBase const &other) = default;
//     OptionBase &operator=(OptionBase const &other) = default;

//     OptionBase(OptionBase &&other) noexcept : OptionStorage<T>(other.occupied) {
//         if (other.occupied) {
//             this->storage.constructInPlace(mv(other.storage.get()));
//         }
//     }
//     OptionBase &operator=(OptionBase &&other) noexcept {
//         this->moveAssign(mv(other));
//         return *this;
//     }
// };

// // why would anyone do this? trivially movable but not trivially copyable??
// template <typename T>
// struct OptionBase<T, true, false, true> : OptionStorage<T> {
//     // Default constructor, initializes to unoccupied option.
//     OptionBase() noexcept : OptionStorage<T>(false) {}
//     OptionBase(EmptyOptionType) noexcept : OptionStorage<T>(false) {}

//     OptionBase(OptionBase const &other) : OptionStorage<T>(other.occupied) {
//         if (other.occupied) {
//             this->storage.constructInPlace(other.storage.get());
//         }
//     }
//     OptionBase &operator=(OptionBase const &other) {
//         this->copyAssign(other);
//         return *this;
//     }

//     OptionBase(OptionBase &&other) = default;
//     OptionBase &operator=(OptionBase &&other) = default;
// };

// template <typename T>
// struct OptionBase<T, true, false, false> : OptionStorage<T> {
//     // Default constructor, initializes to unoccupied option.
//     constexpr OptionBase() noexcept : OptionStorage<T>(false) {}
//     constexpr OptionBase(EmptyOptionType) noexcept : OptionStorage<T>(false) {}

//     OptionBase(OptionBase const &other) : OptionStorage<T>(other.occupied) {
//         if (other.occupied) {
//             this->storage.constructInPlace(other.storage.get());
//         }
//     }
//     OptionBase &operator=(OptionBase const &other) {
//         this->copyAssign(other);
//         return *this;
//     }

//     // @NOTE: moved-from options are not set to empty, since any movable inner type will already
//     // have to have a state that signifies being moved-from, and we'd have to trade off either
//     // consistency with trivially movable inner types, which wouldn't set the old option to
//     empty,
//     // or we'd sacrifice the ability to have trivial move operators entirely for the sake of
//     being
//     // consistent. The least bad option is to be consistent and fast!
//     constexpr OptionBase(OptionBase &&other) noexcept : OptionStorage<T>(other.occupied) {
//         if (other.occupied) {
//             this->storage.constructInPlace(mv(other.storage.get()));
//         }
//     }
//     constexpr OptionBase &operator=(OptionBase &&other) noexcept {
//         this->moveAssign(mv(other));
//         return *this;
//     }
// };

template <typename T>
struct Option;

// clang-format off
template <typename T, typename From>
concept OptionNormalConstructibleFrom
     = IsDifferent<Option<T>, RemoveCvRef<From>>
    && IsDifferent<RemoveCvRef<From>, EmptyOptionType>
    && IsConstructibleFrom<T, From>;

template <typename T, typename From>
concept OptionNormalAssignableFrom
     = IsDifferent<Option<T>, RemoveCvRef<From>>
    && IsDifferent<RemoveCvRef<From>, EmptyOptionType>
    && IsMoveAssignableFrom<T, From>;
// clang-format on

// @NOTE/Unintuitive: because of C++ weirdness, if T is both copy constructible and NOT move
// constructible, option<T> WILL be move constructible via T's copy constructor, I mean, i don't
// think this is going to be much of an issue, since when the hell are you going to ever have a
// class that's copy constructible but NOT move constructible?? that doesn't even make any sense.
// Also, std::optional suffers from the same problem, so I don't feel so bad about leaving this one
// be.
template <typename T>
struct Option {
    using pointer = std::remove_reference_t<T> *;
    using const_pointer = const std::remove_reference_t<T> *;

    static_assert(!std::is_same_v<T, EmptyOptionType>);

    // using OptionBase<T>::OptionBase;

    Option() = default;

    // clang-format off
    template <typename U = T>
        requires OptionNormalConstructibleFrom<T, U &&>
    constexpr Option(U &&value) noexcept {
        new (&mStorage) T(std::forward<U>(value));
        mOccupied = true;
        // this->storage.constructInPlace(std::forward<U>(value));
        // this->occupied = true;
    }

    constexpr Option(EmptyOptionType) noexcept {}

    constexpr Option(const Option &other) requires IsCopyConstructible<T> {
        if (other.mOccupied) {
            new (&mStorage) T(other.get());
        }
        mOccupied = other.mOccupied;
    }

    Option(copy_tag_t, Allocator &alloc, const Option &other) requires IsAllocatorAwareCopyConstructible<T> {
        if (other.mOccupied) {
            new (&mStorage) T(copy_tag, alloc, other.get());
        }
        mOccupied = other.mOccupied;
        // if (other.occupied) {
        //     this->storage.constructInPlace(copy_tag, alloc, other.get());
        // }
        // this->occupied = other.occupied;
    }

    Option clone(Allocator &alloc) const requires IsAllocatorAwareCopyConstructible<T> {
        return Option(copy_tag, alloc, *this);
    }

    constexpr Option(Option &&other) noexcept requires IsMoveConstructible<T> {
        if (other.mOccupied) {
            new (&mStorage) T(mv(other.get()));
        }
        mOccupied = other.mOccupied;
        // this->occupied = other.occupied;
        // if (other.occupied) {
        //     this->storage.constructInPlace(mv(other.get()));
        // }
    }

    template <typename U = T>
    constexpr Option &operator=(U &&other) noexcept requires OptionNormalAssignableFrom<T, U &&> {
        if (mOccupied) {
            get() = std::forward<U>(other);
        } else {
            new (&mStorage) T(std::forward<U>(other));
            mOccupied = true;
        }
        // if (this->occupied) {
        //     this->storage.get() = mv(other);
        // } else {
        //     this->storage.constructInPlace(mv(other));
        //     this->occupied = true;
        // }
        return *this;
    }

    constexpr Option &operator=(EmptyOptionType) noexcept {
        this->clear();
        return *this;
    }

    constexpr Option &operator=(const Option &other) requires IsCopyConstructible<T> && IsCopyAssignable<T> {
        if (&other == this) return *this;

        if (mOccupied && other.mOccupied) {
            get() = other.get();
        } else if (mOccupied && !other.mOccupied) {
            clear();
        } else if (!mOccupied && other.mOccupied) {
            new (&mStorage) T(other.get());
            mOccupied = true;
        }
        // if (this->occupied && other.occupied) {
        //     this->storage.get() = other.get();
        // } else if (this->occupied && !other.occupied) {
        //     this->clear();
        // } else if (!this->occupied && other.occupied) {
        //     this->storage.constructInPlace(other.get());
        //     this->occupied = true;
        // }

        return *this;
    }

    constexpr Option &operator=(Option &&other) noexcept requires IsMoveConstructible<T> && IsMoveAssignable<T> {
        if (&other == this) return *this;

        if (mOccupied && other.mOccupied) {
            get() = mv(other.get());
        } else if (mOccupied && !other.mOccupied) {
            clear();
        } else if (!mOccupied && other.mOccupied) {
            new (&mStorage) T(mv(other.get()));
            mOccupied = true;
        }

        // if (this->occupied && other.occupied) {
        //     this->storage.get() = mv(other.get());
        // } else if (this->occupied && !other.occupied) {
        //     this->clear();
        // } else if (!this->occupied && other.occupied) {
        //     this->storage.constructInPlace(mv(other.get()));
        //     this->occupied = true;
        // }

        return *this;
    }
    // clang-format on

    template <typename... Args>
    void constructInPlace(Args &&...args) {
        if (mOccupied) get().~T();

        new (&mStorage) T(std::forward<Args>(args)...);
        mOccupied = true;
    }

    // clang-format off
    const T &operator*() const noexcept { return get(); }
    T &operator*() noexcept { return get(); }
    const_pointer operator->() const noexcept { return &get(); }
    pointer operator->() noexcept { return &get(); }

    explicit operator bool() const noexcept { return mOccupied; }
    bool isSome() const noexcept { return mOccupied; }
    bool isNone() const noexcept { return !mOccupied; }
    // clang-format on

    constexpr T *getNullablePointer() noexcept { return isSome() ? &get() : nullptr; }
    constexpr T const *getNullablePointer() const noexcept { return isSome() ? &get() : nullptr; }

    void clear() noexcept {
        if (mOccupied) get().~T();
        mOccupied = false;
    }

    constexpr T &get() noexcept {
        VIXEN_DEBUG_ASSERT(mOccupied);
        return *reinterpret_cast<T *>(mStorage);
    }
    constexpr T const &get() const noexcept {
        VIXEN_DEBUG_ASSERT(mOccupied);
        return *reinterpret_cast<T const *>(mStorage);
    }

private:
    alignas(T) opaque mStorage[sizeof(T)];
    bool mOccupied = false;
};

template <typename T>
struct Option<T &> {
    constexpr Option() noexcept : storage(nullptr) {}
    constexpr Option(EmptyOptionType) noexcept : Option() {}

    constexpr Option(T &value) noexcept : storage(std::addressof(value)) {}

    constexpr Option &operator=(T &value) noexcept {
        storage = std::addressof(value);
        return *this;
    }

    // @NOTE: copies and moves are trivial, since these are references we are dealing with. We don't
    // have to invoke any nontrivial move/copy/destruct ops since we can just copy the reference
    // around.
    constexpr Option(Option const &other) = default;
    constexpr Option &operator=(Option const &other) = default;
    constexpr Option(Option &&other) = default;
    constexpr Option &operator=(Option &&other) = default;

    // clang-format off
    constexpr T const &operator*() const noexcept { return get(); }
    constexpr T const *operator->() const noexcept { return &get(); }
    constexpr T &operator*() noexcept { return get(); }
    constexpr T *operator->() noexcept { return &get(); }

    constexpr explicit operator bool() const noexcept { return storage != nullptr; }
    constexpr bool isSome() const noexcept { return storage != nullptr; }
    constexpr bool isNone() const noexcept { return storage == nullptr; }
    // clang-format on

    constexpr void clear() noexcept { storage = nullptr; }

    constexpr T &get() noexcept {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

    constexpr T const &get() const noexcept {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

    constexpr T *getNullablePointer() noexcept { return storage; }

    constexpr T const *getNullablePointer() const noexcept { return storage; }

private:
    T *storage;
};

template <typename T>
struct Option<T const &> {
    constexpr Option() noexcept : storage(nullptr) {}
    constexpr Option(EmptyOptionType) noexcept : Option() {}

    constexpr Option(T const &value) noexcept : storage(std::addressof(value)) {}
    constexpr Option(T &value) noexcept : storage(std::addressof(value)) {}

    constexpr Option &operator=(T const &value) noexcept {
        storage = std::addressof(value);
        return *this;
    }
    constexpr Option &operator=(T &value) noexcept {
        storage = std::addressof(value);
        return *this;
    }

    constexpr Option(Option const &other) = default;
    constexpr Option &operator=(Option const &other) = default;
    constexpr Option(Option &&other) = default;
    constexpr Option &operator=(Option &&other) = default;

    // clang-format off
    constexpr T const &operator*() const noexcept { return get(); }
    constexpr T const *operator->() const noexcept { return &get(); }

    constexpr explicit operator bool() const noexcept { return storage != nullptr; }
    constexpr bool isSome() const noexcept { return storage != nullptr; }
    constexpr bool isNone() const noexcept { return storage == nullptr; }
    // clang-format on

    constexpr void clear() noexcept { storage = nullptr; }

    constexpr T const &get() const noexcept {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

    constexpr T const *getNullablePointer() const noexcept { return storage; }

private:
    T const *storage;
};

// clang-format off
template <typename T, typename U>
concept Comparable = requires(T const &t, U const &u) {
    { t == u } -> IsSame<bool>;
};

template <typename T, typename U> requires Comparable<T, U>
bool operator==(const Option<T> &lhs, const Option<U> &rhs) noexcept {
    return lhs.isSome() ? rhs.isSome() && lhs.get() == rhs.get() : rhs.isNone();
}
template <typename T, typename U> requires Comparable<T, U>
bool operator==(const Option<T> &lhs, const U &rhs) noexcept {
    return lhs.isSome() && lhs.get() == rhs;
}
template <typename T, typename U> requires Comparable<T, U>
bool operator==(const U &lhs, const Option<T> &rhs) noexcept {
    return rhs.isSome() && lhs == rhs.get();
}

template <typename T, typename U> requires Comparable<T, U>
bool operator!=(const Option<T> &lhs, const Option<U> &rhs) noexcept {
    return !(lhs == rhs);
}
template <typename T, typename U> requires Comparable<T, U>
bool operator!=(const Option<T> &lhs, const U &rhs) noexcept {
    return !(lhs == rhs);
}
template <typename T, typename U> requires Comparable<T, U>
bool operator!=(const U &lhs, const Option<T> &rhs) noexcept {
    return !(lhs == rhs);
}
// clang-format on

template <typename T, typename H>
inline void hash(const Option<T> &option, H &hasher) noexcept {
    // TODO: idk what we should hash in the None branch
    if (option) {
        hash(*option, hasher);
    } else {
        hash(u32(0xAAAAAAAA), hasher);
    }
}

} // namespace vixen

// #include "option.inl"
