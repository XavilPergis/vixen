#pragma once

#include <type_traits>

namespace vixen {

template <typename T>
struct TypeCell {
    using Type = T;
};

template <typename T>
concept IsTypeCell = requires {
    typename T::Type;
};

template <typename T, T Value>
struct ValueCell : TypeCell<T> {
    static constexpr T value = Value;
};

struct TrueCell : ValueCell<bool, true> {};
struct FalseCell : ValueCell<bool, false> {};

template <typename T, typename U>
struct IsSameImpl : FalseCell {};
template <typename T>
struct IsSameImpl<T, T> : TrueCell {};

// clang-format off
template <typename T, typename U>
concept IsSame = IsSameImpl<T, U>::value;

template <typename T, typename U>
concept IsDifferent = !IsSame<T, U>;

template <typename T> struct RemoveReferenceImpl      : TypeCell<T> {};
template <typename T> struct RemoveReferenceImpl<T&>  : TypeCell<T> {};
template <typename T> struct RemoveReferenceImpl<T&&> : TypeCell<T> {};

template <typename T> struct RemoveConstImpl          : TypeCell<T> {};
template <typename T> struct RemoveConstImpl<T const> : TypeCell<T> {};

template <typename T> struct RemoveVolatileImpl             : TypeCell<T> {};
template <typename T> struct RemoveVolatileImpl<T volatile> : TypeCell<T> {};

template <typename T> using RemoveReference = typename RemoveReferenceImpl<T>::Type;
template <typename T> using RemoveConst = typename RemoveConstImpl<T>::Type;
template <typename T> using RemoveVolatile = typename RemoveVolatileImpl<T>::Type;
template <typename T> using RemoveCv = RemoveConst<RemoveVolatile<T>>;
template <typename T> using RemoveCvRef = RemoveReference<RemoveCv<T>>;

template <typename T, typename U>
concept IsConstructibleFrom = requires(U &&value) {
    { T(std::forward<U>(value)) } -> IsSame<T>;
};

template <typename T>
concept IsCopyConstructible = requires(T other) {
    { T(static_cast<T const &>(other)) } -> IsSame<T>;
};

template <typename T>
concept IsMoveConstructible = requires(T other) {
    { T(static_cast<T &&>(other)) } -> IsSame<T>;
};

template <typename T>
concept IsCopyAssignable = requires(T &place, T value) {
    { place = static_cast<T const &>(value) } -> IsSame<T &>;
};

template <typename T>
concept IsMoveAssignable = requires(T &place, T value) {
    { place = static_cast<T&&>(value) } -> IsSame<T &>;
};

template <typename T, typename U>
concept IsMoveAssignableFrom = requires(T &place, U value) {
    { place = static_cast<U&&>(value) } -> IsSame<T &>;
};

template <typename To, typename From>
concept IsConvertibleFrom = std::is_convertible_v<From, To>;
// clang-format on

// template <typename T, typename H>
// inline void hash(const T &value, H &hasher) {
//     hasher.write(value);
// }

// template <typename C>
// struct ContainerTraits {};

// // clang-format off
// template <typename Co>
// concept ContainerOperations = requires {
//     // Co = ContainerTraits<Vector<char>>
//     typename Co::Container; // true
//     typename Co::Contained; // true

//     requires requires(typename Co::Container const &container) {
//         {Co::elementCount(container)} -> IsSame<usize>; // true
//     };
// };

// template <typename Co>
// concept InsertableContainerOperations = requires {
//     // Co = ContainerTraits<Vector<char>>
//     requires ContainerOperations<Co>; // true

//     requires !IsMoveConstructible<typename Co::Contained> || requires(
//         typename Co::Container &container, // Vector<char> &container
//         RemoveReference<typename Co::Contained> &&element // char &&element
//     ) {
//         // ContainerTraits<Vector<char>>::insertLast(container, mv(element))
//         {Co::insertLast(container, mv(element))};
//     };

//     requires !IsCopyConstructible<typename Co::Contained> || requires(
//         typename Co::Container &container,
//         typename Co::Contained const &element
//     ) {
//         {Co::insertLast(container, element)};
//     };
// };

// template <typename C>
// concept Container = ContainerOperations<ContainerTraits<C>>;

// template <typename C>
// concept InsertableContainer = InsertableContainerOperations<ContainerTraits<C>>;
// // clang-format on

// template <InsertableContainer C, typename U>
// void containerInsertLast(C &container, U &&item) {
//     ContainerTraits<C>::insertLast(container, std::forward<U>(item));
// }

// template <typename C>
// using ContainedType = typename ContainerTraits<C>::Contained;

template <typename C>
struct is_collection : std::false_type {};

template <typename C>
struct collection_types {};

template <typename T>
struct standard_collection_types {
    using value_type = T;
    using reference = T &;
    using const_reference = T const &;
    using pointer = T &;
    using const_pointer = T const &;
};

struct copyable {};
struct noncopyable {
    noncopyable() = default;
    noncopyable(noncopyable const &) = delete;
    noncopyable &operator=(noncopyable const &) = delete;
    noncopyable(noncopyable &&) = default;
    noncopyable &operator=(noncopyable &&) = default;
};

struct moveable {};
struct nonmoveable {
    nonmoveable() = default;
    nonmoveable(nonmoveable const &) = default;
    nonmoveable &operator=(nonmoveable const &) = default;
    nonmoveable(nonmoveable &&) = delete;
    nonmoveable &operator=(nonmoveable &&) = delete;
};

struct defaultable {};
struct nondefaultable {
    nondefaultable() = delete;
    nondefaultable(nondefaultable const &) = default;
    nondefaultable &operator=(nondefaultable const &) = default;
    nondefaultable(nondefaultable &&) = default;
    nondefaultable &operator=(nondefaultable &&) = default;
};

// poisons generation of default copy constructors when `T` isn't copy-constructible.
template <typename T>
using mimic_copy_ctor = std::conditional_t<std::is_copy_constructible_v<T>, copyable, noncopyable>;

template <typename T>
using mimic_move_ctor = std::conditional_t<std::is_move_constructible_v<T>, moveable, nonmoveable>;

template <typename T>
using mimic_default_ctor
    = std::conditional_t<std::is_default_constructible_v<T>, defaultable, nondefaultable>;

template <typename T>
constexpr bool has_trivial_copy_ops
    = std::is_trivially_copy_constructible_v<T> &&std::is_trivially_copy_assignable_v<T>;

template <typename T>
constexpr bool has_trivial_move_ops
    = std::is_trivially_copy_constructible_v<T> &&std::is_trivially_copy_assignable_v<T>;

template <typename T>
constexpr bool has_trivial_destructor = std::is_trivially_destructible_v<T>;

} // namespace vixen
