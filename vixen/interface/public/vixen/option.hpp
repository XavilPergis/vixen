#pragma once

#include "vixen/traits.hpp"
#include "vixen/types.hpp"
#include "vixen/util.hpp"

#include "erased_storage.hpp"

namespace vixen {

struct empty_opt_type {};
constexpr empty_opt_type empty_opt = empty_opt_type{};

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename... Conds>
using require = std::enable_if_t<std::conjunction_v<Conds...>, bool>;

template <typename T,
    bool = has_trivial_destructor<T>,
    bool = has_trivial_copy_ops<T>,
    bool = has_trivial_move_ops<T>>
struct option_base {
    // Default constructor, initializes to unoccupied option.
    option_base() : occupied(false) {}
    option_base(empty_opt_type) : option_base() {}

    option_base(option_base const &other) : occupied(other.occupied) {
        if (other.occupied) {
            storage.construct_in_place(other.storage.get());
        }
    }
    option_base &operator=(option_base const &other) {
        if (this == std::addressof(other))
            return *this;

        if (occupied && other.occupied) {
            storage.get() = other.storage.get();
        } else if (occupied) {
            storage.destruct();
        } else if (other.occupied) {
            storage.construct_in_place(other.storage.get());
        }

        return *this;
    }

    // @TODO: should moved-from options be empty? I feel like it would be clean to do, but it isn't
    // reeally necessary, since movable items already have to have moved-from states/be trivial,
    // plus that would mean we can't ever make our move constructors trivial. So I'm leaning much
    // more towards not doing that.
    option_base(option_base &&other) : occupied(other.occupied) {
        if (other.occupied) {
            storage.construct_in_place(mv(other.storage.get()));
        }
    }
    option_base &operator=(option_base &&other) {
        if (this == std::addressof(other))
            return *this;

        if (occupied && other.occupied) {
            storage.get() = mv(other.storage.get());
        } else if (occupied) {
            storage.destruct();
        } else if (other.occupied) {
            storage.construct_in_place(mv(other.storage.get()));
        }

        return *this;
    }

protected:
    bool occupied;
    uninitialized_storage<T> storage;
};

template <typename T>
struct option
    : option_base<T>
    , mimic_copy_ctor<T>
    , mimic_move_ctor<T> {
    using pointer = std::remove_reference_t<T> *;
    using const_pointer = const std::remove_reference_t<T> *;

    static_assert(!std::is_same_v<T, empty_opt_type>);

    // Forwarding constructor/assignment operator from inner type
    template <typename U>
    using is_not_self = std::negation<std::is_same<option_base<T>, remove_cvref_t<U>>>;
    template <typename U>
    using is_not_empty_tag = std::negation<std::is_same<T, remove_cvref_t<U>>>;

    using option_base<T>::option_base;

    // Don't declare these for other `option<T>`s
    template <typename U, require<is_not_self<U>, std::is_constructible<T, U &&>> = true>
    option(U &&value) {
        this->occupied = true;
        this->storage.construct_in_place(std::forward<U>(value));
    }

    template <typename U, require<is_not_self<U>, std::is_constructible<T, U &&>> = true>
    option &operator=(U &&other) {
        if (this->occupied) {
            this->storage.get() = std::forward<U>(other);
        } else {
            this->occupied = true;
            this->storage.construct_in_place(std::forward<U>(other));
        }
        return *this;
    }

    // Forwarding constructor/assignment operator from inner type
    // template <typename U>
    // using is_not_self = std::negation<std::is_same<option<T>, remove_cvref_t<U>>>;
    // template <typename U>
    // using is_not_empty_tag = std::negation<std::is_same<T, remove_cvref_t<U>>>;

    // // Don't declare these for other `option<T>`s
    // template <typename U, require<is_not_self<U>, std::is_constructible<T, U &&>> = true>
    // option(U &&value) {
    //     storage.construct_in_place(std::forward<U>(value));
    // }

    // template <typename U, require<is_not_self<U>, std::is_constructible<T, U &&>> = true>
    // option &operator=(U &&other) {
    //     storage.construct_in_place(std::forward<U>(other));
    //     return *this;
    // }

    // option(option const &other) : occupied(other.is_some()) {
    //     if (other.is_some()) {
    //         storage.construct_in_place(other.get());
    //     }
    // }
    // option &operator=(option const &other) {
    //     if (this == std::addressof(other))
    //         return *this;

    //     if (is_some() && other.is_some()) {
    //         storage.get() = other.storage.get();
    //     } else if (is_some()) {
    //         storage.destruct();
    //     } else if (other.is_some()) {
    //         storage.construct_in_place(other.storage.get());
    //     }

    //     return *this;
    // }

    // // @TODO: should moved-from options be empty? I feel like it would be clean to do, but it
    // isn't
    // // reeally necessary, since movable items already have to have moved-from states/be trivial,
    // // plus that would mean we can't ever make our move constructors trivial. So I'm leaning much
    // // more towards not doing that.
    // option(option &&other) = default;
    // option &operator=(option &&other) = default;

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
struct option<T &> {
    constexpr option() : storage(nullptr) {}
    constexpr option(empty_opt_type) : option() {}

    constexpr option(T &value) : storage(std::addressof(value)) {}

    constexpr option &operator=(T &value) {
        storage = std::addressof(value);
        return *this;
    }

    // @NOTE: copies and moves are trivial, since these are references we are dealing with. We don't
    // have to invoke any nontrivial move/copy/destruct ops since we can just copy the reference
    // around.
    constexpr option(option const &other) = default;
    constexpr option &operator=(option const &other) = default;
    constexpr option(option &&other) = default;
    constexpr option &operator=(option &&other) = default;

    // clang-format off
    constexpr T const &operator*() const { return get(); }
    constexpr T const *operator->() const { return &get(); }
    constexpr T &operator*() { return get(); }
    constexpr T *operator->() { return &get(); }

    constexpr explicit operator bool() const { return storage != nullptr; }
    constexpr bool is_some() const { return storage == nullptr; }
    constexpr bool is_none() const { return storage != nullptr; }
    // clang-format on

    constexpr void clear() {
        storage = nullptr;
    }

    constexpr T const &get() const {
        VIXEN_DEBUG_ASSERT(storage != nullptr);
        return *storage;
    }

private:
    T *storage;
};

template <typename T>
struct option<T const &> {
    constexpr option() : storage(nullptr) {}
    constexpr option(empty_opt_type) : option() {}

    constexpr option(T const &value) : storage(std::addressof(value)) {}
    constexpr option(T &value) : storage(std::addressof(value)) {}

    constexpr option &operator=(T const &value) {
        storage = std::addressof(value);
        return *this;
    }
    constexpr option &operator=(T &value) {
        storage = std::addressof(value);
        return *this;
    }

    constexpr option(option const &other) = default;
    constexpr option &operator=(option const &other) = default;
    constexpr option(option &&other) = default;
    constexpr option &operator=(option &&other) = default;

    // clang-format off
    constexpr T const &operator*() const { return get(); }
    constexpr T const *operator->() const { return &get(); }

    constexpr explicit operator bool() const { return storage != nullptr; }
    constexpr bool is_some() const { return storage == nullptr; }
    constexpr bool is_none() const { return storage != nullptr; }
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
bool operator==(const option<T> &lhs, const option<U> &rhs) {
    return lhs.is_some() ? rhs.is_some() && lhs.get() == rhs.get() : rhs.is_none();
}
template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator==(const option<T> &lhs, const U &rhs) {
    return lhs.is_some() && lhs.get() == rhs;
}
// template <typename T, typename U, require<std::is_convertible<U, T>> = true>
// bool operator==(const T &lhs, const option<T> &rhs) {
//     return rhs.is_some() && lhs == rhs.get();
// }

template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator!=(const option<T> &lhs, const option<U> &rhs) {
    return !(lhs == rhs);
}
template <typename T, typename U, require<std::is_convertible<U, T>> = true>
bool operator!=(const option<T> &lhs, const U &rhs) {
    return !(lhs == rhs);
}
// template <typename T, typename U, require<std::is_convertible<U, T>> = true>
// bool operator!=(const T &lhs, const option<T> &rhs) {
//     return !(lhs == rhs);
// }

template <typename T, typename H>
inline void hash(const option<T> &option, H &hasher) {
    // TODO: idk what we should hash in the None branch
    if (option) {
        hash(*option, hasher);
    } else {
        hash(0, hasher);
    }
}

} // namespace vixen

// #include "option.inl"
