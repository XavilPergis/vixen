#include "vixen/unique.hpp"

namespace vixen {

template <typename T>
template <typename U>
unique<T>::unique(allocator *alloc, U &&other)
    : inner_pointer(heap::create_init<unique<T>::inner>(alloc, std::forward<U>(other), alloc)) {}

template <typename T>
unique<T>::unique(allocator *alloc, const unique<T> &other)
    : unique(alloc, copy_construct_maybe_allocator_aware(alloc, *other)) {}

template <typename T>
unique<T>::unique(unique<T> &&other) : inner_pointer(std::exchange(other.inner_pointer, nullptr)) {}

template <typename T>
unique<T> &unique<T>::operator=(unique<T> &&other) {
    if (std::addressof(other) == this)
        return *this;

    clear();
    inner_pointer = std::exchange(other.inner_pointer, nullptr);

    return *this;
}

template <typename T>
unique<T>::~unique() {
    clear();
}

#ifdef VIXEN_IS_DEBUG
#define VIXEN_ASSERT_NONNULL(ptr) VIXEN_ASSERT(ptr != nullptr, "Tried to deference a null pointer.")
#else
#define VIXEN_ASSERT_NONNULL(...)
#endif

template <typename T>
const T &unique<T>::operator*() const {
    VIXEN_ASSERT_NONNULL(inner_pointer->data);
    return inner_pointer->data;
}

template <typename T>
const T *unique<T>::operator->() const {
    VIXEN_ASSERT_NONNULL(inner_pointer->data);
    return &inner_pointer->data;
}

template <typename T>
T &unique<T>::operator*() {
    VIXEN_ASSERT_NONNULL(inner_pointer->data);
    return inner_pointer->data;
}

template <typename T>
T *unique<T>::operator->() {
    VIXEN_ASSERT_NONNULL(inner_pointer->data);
    return &inner_pointer->data;
}

template <typename T>
unique<T>::operator bool() const {
    return inner_pointer;
}

template <typename T>
unique<T>::operator const_pointer() const {
    return inner_pointer;
}

template <typename T>
unique<T>::operator pointer() {
    return inner_pointer;
}

template <typename T>
void unique<T>::clear() {
    if (inner_pointer) {
        heap::destroy_init(inner_pointer->alloc, inner_pointer);
    }
}

} // namespace vixen
