#include "vixen/unique.hpp"

namespace vixen {

template <typename T>
template <typename U>
unique<T>::unique(allocator *alloc, U &&other)
    : alloc(alloc), data(heap::create_init<T>(alloc, std::forward<U>(other))) {}

template <typename T>
unique<T>::unique(allocator *alloc, const unique<T> &other)
    : unique(alloc, MOVE(copy_construct_maybe_allocator_aware(alloc, *other))) {}

template <typename T>
unique<T>::unique(unique<T> &&other)
    : alloc(std::exchange(other.alloc, nullptr)), data(std::exchange(other.data, nullptr)) {}

template <typename T>
unique<T> &unique<T>::operator=(unique<T> &&other) {
    VIXEN_WARN("unique::operator=(unique<T>&&)");
    if (std::addressof(other) == this)
        return *this;

    clear();

    alloc = std::exchange(other.alloc, nullptr);
    data = std::exchange(other.data, nullptr);

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
    VIXEN_ASSERT_NONNULL(data);
    return *data;
}

template <typename T>
const T *unique<T>::operator->() const {
    VIXEN_ASSERT_NONNULL(data);
    return data;
}

template <typename T>
T &unique<T>::operator*() {
    VIXEN_ASSERT_NONNULL(data);
    return *data;
}

template <typename T>
T *unique<T>::operator->() {
    VIXEN_ASSERT_NONNULL(data);
    return data;
}

template <typename T>
unique<T>::operator bool() const {
    return data;
}

template <typename T>
unique<T>::operator const_pointer() const {
    return data;
}

template <typename T>
unique<T>::operator pointer() {
    return data;
}

template <typename T>
void unique<T>::clear() {
    if (data) {
        data->~T();
        heap::destroy(alloc, data);
    }
}

} // namespace vixen
