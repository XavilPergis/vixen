#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/hash/map.hpp"
#include "vixen/types.hpp"
#include "vixen/unique.hpp"

namespace vixen {

using TypeId = usize;

template <typename T>
struct get_type_id_impl {
    static T *inst;
};

template <typename T>
T *get_type_id_impl<T>::inst = nullptr;

template <typename T>
constexpr TypeId getTypeId() {
    return reinterpret_cast<TypeId>(&get_type_id_impl<T>::inst);
}

template <typename T>
struct AnyType {};

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
    Any(AnyType<T>, Allocator *alloc, Args &&...args)
        : typeId(getTypeId<T>()), value(makeUnique<Value<T>>(alloc, std::forward<Args>(args)...)) {}

    template <typename T>
    T &get() {
        VIXEN_DEBUG_ASSERT(typeId == getTypeId<T>());
        return static_cast<Value<T> *>(&*value)->value;
    }

    template <typename T>
    const T &get() const {
        VIXEN_DEBUG_ASSERT(typeId == getTypeId<T>());
        return static_cast<const Value<T> *>(&*value)->value;
    }

    TypeId typeId;
    Unique<ValueBase> value;
};

template <typename T, typename... Args>
Any makeAny(Allocator *alloc, Args &&...args) {
    return Any{AnyType<T>{}, alloc, std::forward<Args>(args)...};
}

struct Blackboard {
    Blackboard(Allocator *alloc) : alloc(alloc), values(alloc) {}

    template <typename T, typename... Args>
    Option<T> insert(Args &&...args) {
        static_assert(std::is_same_v<std::decay_t<T>, T>);

        auto old
            = values.insert(getTypeId<T>(), Any{AnyType<T>{}, alloc, std::forward<Args>(args)...});
        if (old.isSome()) {
            return mv(old->template get<T>());
        } else {
            return empty_opt;
        }
    }

    template <typename T>
    bool has() const {
        static_assert(std::is_same_v<std::decay_t<T>, T>);

        return values.contains_key(getTypeId<T>());
    }

    template <typename T>
    T &get() {
        static_assert(std::is_same_v<std::decay_t<T>, T>);
        VIXEN_DEBUG_ASSERT(values.contains_key(getTypeId<T>()));

        return values[getTypeId<T>()].template get<T>();
    }

    template <typename T>
    Option<T> remove() {
        static_assert(std::is_same_v<std::decay_t<T>, T>);

        auto old = values.remove(getTypeId<T>());
        if (old.isSome()) {
            return mv(old->template get<T>());
        } else {
            return empty_opt;
        }
    }

    Allocator *alloc;
    Map<TypeId, Any> values;
};

} // namespace vixen
