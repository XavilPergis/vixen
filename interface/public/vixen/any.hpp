#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/map.hpp"
#include "vixen/types.hpp"
#include "vixen/unique.hpp"

namespace vixen {

struct TypeInfoImpl {
    // if I want to add type info here in the future, i can do it with this!
    // virtual T getT() = 0;
};

template <typename T>
struct TypeInfoImplInstance : TypeInfoImpl {};

struct TypeInfo {
    bool operator==(TypeInfo const &other) noexcept { return this->mId == other.mId; }
    bool operator!=(TypeInfo const &other) noexcept { return this->mId != other.mId; }

protected:
    TypeInfo(TypeInfoImpl *id) noexcept : mId(id) {}
    TypeInfoImpl *mId;

    template <typename T>
    friend constexpr TypeInfo &getTypeInfo();
};

template <typename T>
struct get_type_id_impl {
    static TypeInfoImplInstance<T> *inst;
};

template <typename T>
TypeInfoImplInstance<T> *get_type_id_impl<T>::inst = TypeInfoImplInstance<T>{};

template <typename T>
constexpr TypeInfo getTypeInfo() noexcept {
    return TypeInfo{*get_type_id_impl<T>::inst};
}

template <typename T>
constexpr TypeInfo getTypeInfoOf(T const &value) noexcept {
    return TypeInfo{*get_type_id_impl<T>::inst};
}

template <typename T>
struct TypeKey {};

struct Any {
    struct ValueBase {
        virtual ~ValueBase() {}
    };

    template <typename T>
    struct Value : ValueBase {
        template <typename... Args>
        Value(Args &&...args) : value(std::forward<Args>(args)...) {}

        T value;
    };

    template <typename T, typename... Args>
    Any(TypeKey<T>, Allocator &alloc, Args &&...args)
        : typeInfo(getTypeInfo<T>())
        , value(makeUnique<Value<T>>(alloc, std::forward<Args>(args)...)) {}

    template <typename T>
    Any(Allocator &alloc, T &&value) : Any(TypeKey<T>{}, alloc, std::forward<T>(value)) {}

    template <typename T>
    Option<T> downcast() noexcept {
        if (typeInfo == getTypeInfo<T>()) {
            return std::move(static_cast<Value<T> &>(*value).value);
        } else {
            return EMPTY_OPTION;
        }
    }

    template <typename T>
    T &get() noexcept {
        VIXEN_DEBUG_ASSERT(typeInfo == getTypeInfo<T>());
        return static_cast<Value<T> &>(*value).value;
    }

    template <typename T>
    const T &get() const noexcept {
        VIXEN_DEBUG_ASSERT(typeInfo == getTypeInfo<T>());
        return static_cast<const Value<T> &>(*value).value;
    }

    TypeInfo typeInfo;
    Unique<ValueBase> value;
};

template <typename T, typename... Args>
Any makeAny(Allocator &alloc, Args &&...args) {
    return Any{TypeKey<T>{}, alloc, std::forward<Args>(args)...};
}

struct Blackboard {
    Blackboard(Allocator &alloc) noexcept : alloc(alloc), values(alloc) {}

    template <typename T, typename... Args>
    Option<T> insert(Args &&...args) {
        static_assert(std::is_same_v<std::decay_t<T>, T>);

        auto old = values.insert(getTypeInfo<T>(),
            Any{TypeKey<T>{}, alloc, std::forward<Args>(args)...});
        if (old.isSome()) {
            return mv(old->template get<T>());
        } else {
            return EMPTY_OPTION;
        }
    }

    template <typename T>
    bool has() const noexcept {
        static_assert(std::is_same_v<std::decay_t<T>, T>);

        return values.containsKey(getTypeInfo<T>());
    }

    template <typename T>
    T &get() noexcept {
        static_assert(std::is_same_v<std::decay_t<T>, T>);
        VIXEN_DEBUG_ASSERT(values.containsKey(getTypeInfo<T>()));

        return values[getTypeInfo<T>()].template get<T>();
    }

    template <typename T>
    Option<T> remove() noexcept {
        static_assert(std::is_same_v<std::decay_t<T>, T>);

        auto old = values.remove(getTypeInfo<T>());
        if (old.isSome()) {
            return mv(old->template get<T>());
        } else {
            return EMPTY_OPTION;
        }
    }

    template <typename T>
    bool has(TypeKey<T>) const noexcept {
        return this->template has<T>();
    }

    template <typename T>
    T &get(TypeKey<T>) noexcept {
        return this->template get<T>();
    }

    template <typename T>
    Option<T> remove(TypeKey<T>) noexcept {
        return this->template remove<T>();
    }

    Allocator &alloc;
    Map<TypeInfo, Any> values;
};

} // namespace vixen
