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

template <typename T>
struct OptionStorage {
protected:
    OptionStorage() = default;
    OptionStorage(bool occupied) : occupied(occupied) {}

    void copyAssign(OptionStorage const &other) {
        if (this->occupied && other.occupied) {
            this->storage.get() = other.storage.get();
        } else if (this->occupied) {
            this->storage.destruct();
        } else if (other.occupied) {
            this->storage.constructInPlace(other.storage.get());
        }
    }

    void moveAssign(OptionStorage &&other) {
        if (this->occupied && other.occupied) {
            this->storage.get() = mv(other.storage.get());
        } else if (this->occupied) {
            this->storage.destruct();
        } else if (other.occupied) {
            this->storage.constructInPlace(mv(other.storage.get()));
        }
    }

    void destruct() {
        if (occupied) storage.destruct();
    }

    bool occupied;
    UninitializedStorage<T> storage;
};

template <typename T,
    bool = has_trivial_destructor<T>,
    bool = has_trivial_copy_ops<T>,
    bool = has_trivial_move_ops<T>>
struct OptionBase;

template <typename T, bool C, bool M>
struct OptionBase<T, false, C, M> : OptionBase<T, true, false, false> {
    // Default constructor, initializes to unoccupied option.
    OptionBase() : OptionBase<T, true, false, false>() {}
    OptionBase(EmptyOptionType) : OptionBase<T, true, false, false>() {}

    OptionBase(OptionBase const &other) = default;
    OptionBase &operator=(OptionBase const &other) = default;
    OptionBase(OptionBase &&other) = default;
    OptionBase &operator=(OptionBase &&other) = default;

    ~OptionBase() { this->destruct(); }
};

template <typename T>
struct OptionBase<T, true, true, true> : OptionStorage<T> {
    // Default constructor, initializes to unoccupied option.
    OptionBase() : OptionStorage<T>(false) {}
    OptionBase(EmptyOptionType) : OptionStorage<T>(false) {}

    OptionBase(OptionBase const &other) = default;
    OptionBase &operator=(OptionBase const &other) = default;

    OptionBase(OptionBase &&other) = default;
    OptionBase &operator=(OptionBase &&other) = default;
};

template <typename T>
struct OptionBase<T, true, true, false> : OptionStorage<T> {
    // Default constructor, initializes to unoccupied option.
    OptionBase() : OptionStorage<T>(false) {}
    OptionBase(EmptyOptionType) : OptionStorage<T>(false) {}

    OptionBase(OptionBase const &other) = default;
    OptionBase &operator=(OptionBase const &other) = default;

    OptionBase(OptionBase &&other) : OptionStorage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.constructInPlace(mv(other.storage.get()));
        }
    }
    OptionBase &operator=(OptionBase &&other) {
        this->moveAssign(mv(other));
        return *this;
    }
};

// why would anyone do this? trivially movable but not trivially copyable??
template <typename T>
struct OptionBase<T, true, false, true> : OptionStorage<T> {
    // Default constructor, initializes to unoccupied option.
    OptionBase() : OptionStorage<T>(false) {}
    OptionBase(EmptyOptionType) : OptionStorage<T>(false) {}

    OptionBase(OptionBase const &other) : OptionStorage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.constructInPlace(other.storage.get());
        }
    }
    OptionBase &operator=(OptionBase const &other) {
        this->copyAssign(other);
        return *this;
    }

    OptionBase(OptionBase &&other) = default;
    OptionBase &operator=(OptionBase &&other) = default;
};

template <typename T>
struct OptionBase<T, true, false, false> : OptionStorage<T> {
    // Default constructor, initializes to unoccupied option.
    OptionBase() : OptionStorage<T>(false) {}
    OptionBase(EmptyOptionType) : OptionStorage<T>(false) {}

    OptionBase(OptionBase const &other) : OptionStorage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.constructInPlace(other.storage.get());
        }
    }
    OptionBase &operator=(OptionBase const &other) {
        this->copyAssign(other);
        return *this;
    }

    // @NOTE: moved-from options are not set to empty, since any movable inner type will already
    // have to have a state that signifies being moved-from, and we'd have to trade off either
    // consistency with trivially movable inner types, which wouldn't set the old option to empty,
    // or we'd sacrifice the ability to have trivial move operators entirely for the sake of being
    // consistent. The least bad option is to be consistent and fast!
    OptionBase(OptionBase &&other) : OptionStorage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.constructInPlace(mv(other.storage.get()));
        }
    }
    OptionBase &operator=(OptionBase &&other) {
        this->moveAssign(mv(other));
        return *this;
    }
};

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
struct Option
    : OptionBase<T>
    , mimic_copy_ctor<T>
    , mimic_move_ctor<T> {
    using pointer = std::remove_reference_t<T> *;
    using const_pointer = const std::remove_reference_t<T> *;

    static_assert(!std::is_same_v<T, EmptyOptionType>);

    // Forwarding constructor/assignment operator from inner type
    template <typename U>
    using is_not_self = std::negation<std::is_same<Option<T>, remove_cvref_t<U>>>;
    template <typename U>
    using is_not_empty_tag = std::negation<std::is_same<T, remove_cvref_t<U>>>;

    using OptionBase<T>::OptionBase;

    // Don't declare these for other `option<T>`s
    // clang-format off
    template <typename U>
        requires OptionNormalConstructibleFrom<T, U &&>
    Option(U &&value) {
        this->storage.constructInPlace(std::forward<U>(value));
        this->occupied = true;
    }

    Option(EmptyOptionType) noexcept {
        this->occupied = false;
    }

    Option(const Option &other) requires IsCopyConstructible<T> {
        this->occupied = other.occupied;
        if (other.occupied) {
            this->storage.constructInPlace(other.get());
        }
    }

    Option(copy_tag_t, Allocator &alloc, const Option &other) requires IsAllocatorAwareCopyConstructible<T> {
        this->occupied = other.occupied;
        if (other.occupied) {
            this->storage.constructInPlace(copy_tag, alloc, other.get());
        }
    }

    Option clone(Allocator &alloc) requires IsAllocatorAwareCopyConstructible<T> {
        return Option(copy_tag, alloc, *this);
    }

    Option(Option &&other) noexcept requires IsMoveConstructible<T> {
        this->occupied = other.occupied;
        if (other.occupied) {
            this->storage.constructInPlace(mv(other.get()));
        }
    }

    template <typename U>
        requires OptionNormalAssignableFrom<T, U &&>
    Option &operator=(U &&other) noexcept {
        if (this->occupied) {
            this->storage.get() = std::forward<U>(other);
        } else {
            this->storage.constructInPlace(std::forward<U>(other));
            this->occupied = true;
        }
        return *this;
    }

    Option &operator=(EmptyOptionType) noexcept {
        this->clear();
        return *this;
    }

    Option &operator=(const Option &other) requires IsCopyConstructible<T> && IsCopyAssignable<T> {
        if (&other == this) return *this;

        if (this->occupied && other.occupied) {
            this->storage.get() = other.get();
        } else if (this->occupied && !other.occupied) {
            this->clear();
        } else if (!this->occupied && other.occupied) {
            this->storage.constructInPlace(other.get());
            this->occupied = true;
        }

        return *this;
    }

    Option &operator=(Option &&other) noexcept requires IsMoveConstructible<T> && IsMoveAssignable<T> {
        if (&other == this) return *this;

        if (this->occupied && other.occupied) {
            this->storage.get() = mv(other.get());
        } else if (this->occupied && !other.occupied) {
            this->clear();
        } else if (!this->occupied && other.occupied) {
            this->storage.constructInPlace(mv(other.get()));
            this->occupied = true;
        }

        return *this;
    }
    // clang-format on

    template <typename... Args>
    void constructInPlace(Args &&...args) {
        if (this->occupied) {
            this->storage.destruct();
        }
        this->storage.constructInPlace(std::forward<Args...>(args)...);
        this->occupied = true;
    }

    // clang-format off
    const T &operator*() const noexcept { return get(); }
    T &operator*() noexcept { return get(); }
    const_pointer operator->() const noexcept { return &get(); }
    pointer operator->() noexcept { return &get(); }

    explicit operator bool() const noexcept { return this->occupied; }
    bool isSome() const noexcept { return this->occupied; }
    bool isNone() const noexcept { return !this->occupied; }
    // clang-format on

    constexpr T *getNullablePointer() noexcept { return isSome() ? &get() : nullptr; }
    constexpr T const *getNullablePointer() const noexcept { return isSome() ? &get() : nullptr; }

    void clear() noexcept {
        if (this->occupied) {
            this->storage.destruct();
        }
        this->occupied = false;
    }

    constexpr T &get() noexcept {
        VIXEN_DEBUG_ASSERT(this->occupied);
        return this->storage.get();
    }
    constexpr T const &get() const noexcept {
        VIXEN_DEBUG_ASSERT(this->occupied);
        return this->storage.get();
    }
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
bool operator==(const Option<T> &lhs, const Option<U> &rhs) {
    return lhs.isSome() ? rhs.isSome() && lhs.get() == rhs.get() : rhs.isNone();
}
template <typename T, typename U> requires Comparable<T, U>
bool operator==(const Option<T> &lhs, const U &rhs) {
    return lhs.isSome() && lhs.get() == rhs;
}
template <typename T, typename U> requires Comparable<T, U>
bool operator==(const U &lhs, const Option<T> &rhs) {
    return rhs.isSome() && lhs == rhs.get();
}

template <typename T, typename U> requires Comparable<T, U>
bool operator!=(const Option<T> &lhs, const Option<U> &rhs) {
    return !(lhs == rhs);
}
template <typename T, typename U> requires Comparable<T, U>
bool operator!=(const Option<T> &lhs, const U &rhs) {
    return !(lhs == rhs);
}
template <typename T, typename U> requires Comparable<T, U>
bool operator!=(const U &lhs, const Option<T> &rhs) {
    return !(lhs == rhs);
}
// clang-format on

template <typename T, typename H>
inline void hash(const Option<T> &option, H &hasher) {
    // TODO: idk what we should hash in the None branch
    if (option) {
        hash(*option, hasher);
    } else {
        hash(u32(0xAAAAAAAA), hasher);
    }
}

} // namespace vixen

// #include "option.inl"
