#include "vixen/unique.hpp"

namespace vixen {

template <typename T>
template <typename... Args>
Unique<T>::Unique(Allocator *alloc, Args &&...args)
    : inner_pointer(heap::create_init<Unique<T>::inner>(
        alloc, Unique<T>::inner{alloc, std::forward<Args>(args)...})) {}

template <typename T>
Unique<T>::Unique(Allocator *alloc, const Unique<T> &other)
    : Unique(alloc, copy_construct_maybe_allocator_aware(alloc, *other)) {}

template <typename T>
Unique<T>::Unique(Unique<T> &&other) : inner_pointer(std::exchange(other.inner_pointer, nullptr)) {}

template <typename T>
Unique<T> &Unique<T>::operator=(Unique<T> &&other) {
    if (std::addressof(other) == this)
        return *this;

    clear();
    inner_pointer = std::exchange(other.inner_pointer, nullptr);

    return *this;
}

template <typename T>
Unique<T>::~Unique() {
    clear();
}

#ifdef VIXEN_IS_DEBUG
#define VIXEN_ASSERT_NONNULL(ptr) \
    VIXEN_ASSERT_EXT(ptr != nullptr, "Tried to deference a null pointer.")
#else
#define VIXEN_ASSERT_NONNULL(...)
#endif

template <typename T>
const T &Unique<T>::operator*() const {
    VIXEN_ASSERT_NONNULL(inner_pointer);
    return inner_pointer->data;
}

template <typename T>
const T *Unique<T>::operator->() const {
    VIXEN_ASSERT_NONNULL(inner_pointer);
    return &inner_pointer->data;
}

template <typename T>
T &Unique<T>::operator*() {
    VIXEN_ASSERT_NONNULL(inner_pointer);
    return inner_pointer->data;
}

template <typename T>
T *Unique<T>::operator->() {
    VIXEN_ASSERT_NONNULL(inner_pointer);
    return &inner_pointer->data;
}

template <typename T>
Unique<T>::operator bool() const {
    return inner_pointer;
}

template <typename T>
Unique<T>::operator const_pointer() const {
    return inner_pointer ? &inner_pointer->data : nullptr;
}

template <typename T>
Unique<T>::operator pointer() {
    return inner_pointer ? &inner_pointer->data : nullptr;
}

template <typename T>
void Unique<T>::clear() {
    if (inner_pointer) {
        heap::destroy_init(inner_pointer->alloc, inner_pointer);
    }
}

} // namespace vixen
