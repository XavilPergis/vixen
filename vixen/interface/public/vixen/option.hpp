#pragma once

#include "vixen/assert.hpp"
#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include "erased_storage.hpp"

namespace vixen {

struct empty_opt_type {};
constexpr empty_opt_type empty_opt = empty_opt_type{};

template <typename T>
using remove_cvref_t = std::remove_reference_t<std::remove_cv_t<T>>;

template <typename... Conds>
using require = std::enable_if_t<std::conjunction_v<Conds...>, bool>;

template <typename T>
struct option_storage {
protected:
    option_storage() = default;
    option_storage(bool occupied) : occupied(occupied) {}

    void copy_assign(option_storage const &other) {
        if (this->occupied && other.occupied) {
            this->storage.get() = other.storage.get();
        } else if (this->occupied) {
            this->storage.destruct();
        } else if (other.occupied) {
            this->storage.construct_in_place(other.storage.get());
        }
    }

    void move_assign(option_storage &&other) {
        if (this->occupied && other.occupied) {
            this->storage.get() = mv(other.storage.get());
        } else if (this->occupied) {
            this->storage.destruct();
        } else if (other.occupied) {
            this->storage.construct_in_place(mv(other.storage.get()));
        }
    }

    void destruct() {
        if (occupied)
            storage.destruct();
    }

    bool occupied;
    UninitializedStorage<T> storage;
};

template <typename T,
    bool = has_trivial_destructor<T>,
    bool = has_trivial_copy_ops<T>,
    bool = has_trivial_move_ops<T>>
struct option_base;

template <typename T, bool C, bool M>
struct option_base<T, false, C, M> : option_base<T, true, false, false> {
    // Default constructor, initializes to unoccupied option.
    option_base() : option_base<T, true, false, false>() {}
    option_base(empty_opt_type) : option_base<T, true, false, false>() {}

    option_base(option_base const &other) = default;
    option_base &operator=(option_base const &other) = default;
    option_base(option_base &&other) = default;
    option_base &operator=(option_base &&other) = default;

    ~option_base() {
        this->destruct();
    }
};

template <typename T>
struct option_base<T, true, true, true> : option_storage<T> {
    // Default constructor, initializes to unoccupied option.
    option_base() : option_storage<T>(false) {}
    option_base(empty_opt_type) : option_storage<T>(false) {}

    option_base(option_base const &other) = default;
    option_base &operator=(option_base const &other) = default;

    option_base(option_base &&other) = default;
    option_base &operator=(option_base &&other) = default;
};

template <typename T>
struct option_base<T, true, true, false> : option_storage<T> {
    // Default constructor, initializes to unoccupied option.
    option_base() : option_storage<T>(false) {}
    option_base(empty_opt_type) : option_storage<T>(false) {}

    option_base(option_base const &other) = default;
    option_base &operator=(option_base const &other) = default;

    option_base(option_base &&other) : option_storage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.construct_in_place(mv(other.storage.get()));
        }
    }
    option_base &operator=(option_base &&other) {
        this->move_assign(mv(other));
        return *this;
    }
};

// why would anyone do this? trivially movable but not trivially copyable??
template <typename T>
struct option_base<T, true, false, true> : option_storage<T> {
    // Default constructor, initializes to unoccupied option.
    option_base() : option_storage<T>(false) {}
    option_base(empty_opt_type) : option_storage<T>(false) {}

    option_base(option_base const &other) : option_storage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.construct_in_place(other.storage.get());
        }
    }
    option_base &operator=(option_base const &other) {
        this->copy_assign(other);
        return *this;
    }

    option_base(option_base &&other) = default;
    option_base &operator=(option_base &&other) = default;
};

template <typename T>
struct option_base<T, true, false, false> : option_storage<T> {
    // Default constructor, initializes to unoccupied option.
    option_base() : option_storage<T>(false) {}
    option_base(empty_opt_type) : option_storage<T>(false) {}

    option_base(option_base const &other) : option_storage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.construct_in_place(other.storage.get());
        }
    }
    option_base &operator=(option_base const &other) {
        this->copy_assign(other);
        return *this;
    }

    // @NOTE: moved-from options are not set to empty, since any movable inner type will already
    // have to have a state that signifies being moved-from, and we'd have to trade off either
    // consistency with trivially movable inner types, which wouldn't set the old option to empty,
    // or we'd sacrifice the ability to have trivial move operators entirely for the sake of being
    // consistent. The least bad option is to be consistent and fast!
    option_base(option_base &&other) : option_storage<T>(other.occupied) {
        if (other.occupied) {
            this->storage.construct_in_place(mv(other.storage.get()));
        }
    }
    option_base &operator=(option_base &&other) {
        this->move_assign(mv(other));
        return *this;
    }
};

// @NOTE/Unintuitive: because of C++ weirdness, if T is both copy constructible and NOT move
// constructible, option<T> WILL be move constructible via T's copy constructor, I mean, i don't
// think this is going to be much of an issue, since when the hell are you going to ever have a
// class that's copy constructible but NOT move constructible?? that doesn't even make any sense.
// Also, std::optional suffers from the same problem, so I don't feel so bad about leaving this one
// be.
template <typename T>
struct Option
    : option_base<T>
    , mimic_copy_ctor<T>
    , mimic_move_ctor<T> {
    using pointer = std::remove_reference_t<T> *;
    using const_pointer = const std::remove_reference_t<T> *;

    static_assert(!std::is_same_v<T, empty_opt_type>);

    // Forwarding constructor/assignment operator from inner type
    template <typename U>
    using is_not_self = std::negation<std::is_same<Option<T>, remove_cvref_t<U>>>;
    template <typename U>
    using is_not_empty_tag = std::negation<std::is_same<T, remove_cvref_t<U>>>;

    using option_base<T>::option_base;

    // Don't declare these for other `option<T>`s
    template <typename U, require<is_not_self<U>, std::is_constructible<T, U &&>> = true>
    Option(U &&value) {
        this->occupied = true;
        this->storage.construct_in_place(std::forward<U>(value));
    }

    template <typename U, require<is_not_self<U>, std::is_constructible<T, U &&>> = true>
    Option &operator=(U &&other) {
        if (this->occupied) {
            this->storage.get() = std::forward<U>(other);
        } else {
            this->occupied = true;
            this->storage.construct_in_place(std::forward<U>(other));
        }
        return *this;
    }

    template <typename... Args>
    void construct_in_place(Args &&...args) {
        if (this->occupied) {
            this->storage.destruct();
        }
        this->storage.construct_in_place(std::forward<Args...>(args)...);
        this->occupied = true;
    }

    // clang-format off
    const T &operator*() const { return get(); }
    T &operator*() { return get(); }
    const_pointer operator->() const { return &get(); }
    pointer operator->() { return &get(); }

    explicit operator bool() const { return this->occupied; }
    bool is_some() const { return this->occupied; }
    bool is_none() const { return !this->occupied; }
    // clang-format on

    void clear() {
        if (this->occupied) {
            this->storage.destruct();
        }
        this->occupied = false;
    }

    constexpr T &get() {
        VIXEN_DEBUG_ASSERT(this->occupied);
        return this->storage.get();
    }
    constexpr T const &get() const {
        VIXEN_DEBUG_ASSERT(this->occupied);
        return this->storage.get();
    }
};

template <typename T>
struct Option<T &> {
    constexpr Option() : storage(nullptr) {}
    constexpr Option(empty_opt_type) : Option() {}

    constexpr Option(T &value) : storage(std::addressof(value)) {}

    constexpr Option &operator=(T &value) {
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
    constexpr T const &operator*() const { return get(); }
    constexpr T const *operator->() const { return &get(); }
    constexpr T &operator*() { return get(); }
    constexpr T *operator->() { return &get(); }

    constexpr explicit operator bool() const { return storage != nullptr; }
    constexpr bool is_some() const { return storage != nullptr; }
    constexpr bool is_none() const { return storage == nullptr; }
    // clang-format on

    constexpr void clear() {
        storage = nullptr;
    }

    constexpr T &get() {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

    constexpr T const &get() const {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

private:
    T *storage;
};

template <typename T>
struct Option<T const &> {
    constexpr Option() : storage(nullptr) {}
    constexpr Option(empty_opt_type) : Option() {}

    constexpr Option(T const &value) : storage(std::addressof(value)) {}
    constexpr Option(T &value) : storage(std::addressof(value)) {}

    constexpr Option &operator=(T const &value) {
        storage = std::addressof(value);
        return *this;
    }
    constexpr Option &operator=(T &value) {
        storage = std::addressof(value);
        return *this;
    }

    constexpr Option(Option const &other) = default;
    constexpr Option &operator=(Option const &other) = default;
    constexpr Option(Option &&other) = default;
    constexpr Option &operator=(Option &&other) = default;

    // clang-format off
    constexpr T const &operator*() const { return get(); }
    constexpr T const *operator->() const { return &get(); }

    constexpr explicit operator bool() const { return storage != nullptr; }
    constexpr bool is_some() const { return storage != nullptr; }
    constexpr bool is_none() const { return storage == nullptr; }
    // clang-format on

    constexpr void clear() {
        storage = nullptr;
    }

    constexpr T const &get() const {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

private:
    T const *storage;
};

template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator==(const Option<T> &lhs, const Option<U> &rhs) {
    return lhs.is_some() ? rhs.is_some() && lhs.get() == rhs.get() : rhs.is_none();
}
template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator==(const Option<T> &lhs, const U &rhs) {
    return lhs.is_some() && lhs.get() == rhs;
}
template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator==(const U &lhs, const Option<T> &rhs) {
    return rhs.is_some() && lhs == rhs.get();
}

template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator!=(const Option<T> &lhs, const Option<U> &rhs) {
    return !(lhs == rhs);
}
template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator!=(const Option<T> &lhs, const U &rhs) {
    return !(lhs == rhs);
}
template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator!=(const U &lhs, const Option<T> &rhs) {
    return !(lhs == rhs);
}

template <typename T, typename H>
inline void hash(const Option<T> &option, H &hasher) {
    // TODO: idk what we should hash in the None branch
    if (option) {
        hash(*option, hasher);
    } else {
        hash(-1337, hasher);
    }
}

} // namespace vixen

// #include "option.inl"
