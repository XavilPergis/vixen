#pragma once

#include "vixen/log.hpp"
#include "vixen/typeops.hpp"
#include "vixen/util.hpp"
#include "vixen/vector.hpp"

#include <algorithm>
#include <type_traits>

namespace vixen {

template <typename T>
inline Vector<T>::Vector(Vector<T> &&other) noexcept
    : mAlloc(util::exchange(other.mAlloc, nullptr))
    , mData(util::exchange(other.mData, nullptr))
    , mLength(util::exchange(other.mLength, 0))
    , mCapacity(util::exchange(other.mCapacity, 0)) {}

template <typename T>
inline Vector<T> &Vector<T>::operator=(Vector<T> &&other) noexcept {
    if (std::addressof(other) == this) return *this;

    this->mAlloc = util::exchange(other.mAlloc, nullptr);
    this->mData = util::exchange(other.mData, nullptr);
    this->mLength = util::exchange(other.mLength, 0);
    this->mCapacity = util::exchange(other.mCapacity, 0);
    return *this;
}

template <typename T>
inline Vector<T>::~Vector() {
    if (mCapacity > 0) {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (usize i = 0; i < mLength; ++i) {
                mData[i].~T();
            }
        }
        mAlloc->dealloc(heap::Layout::arrayOf<T>(mCapacity), mData);
    }
}

template <typename T>
inline Vector<T>::Vector(copy_tag_t, Allocator &alloc, Vector<T> const &other) : mAlloc(&alloc) {
    setCapacity(other.mLength);
    for (usize i = 0; i < other.mLength; ++i) {
        copyAllocateEmplace(ptrTo(i), alloc, other.mData[i]);
    }
    mLength = other.mLength;
}

template <typename T>
inline T *Vector<T>::reserveLast(usize elements) {
    ensureCapacity(elements);
    return mData + mLength;
}

template <typename T>
inline void Vector<T>::unsafeGrowBy(usize elements) noexcept {
    VIXEN_ASSERT_EXT(mLength + elements <= mCapacity,
        "tried to grow a(n) {}-element long vector by {}, but its capacity is only {}.",
        mLength,
        elements,
        mCapacity);
    mLength += elements;
}

template <typename T>
inline void Vector<T>::shrinkBy(usize elements) noexcept {
    VIXEN_ASSERT_EXT(mLength >= elements,
        "tried to shrink a(n) {}-element long vector by {}",
        mLength,
        elements);

    shrinkTo(mLength - elements);
}

template <typename T>
inline void Vector<T>::shrinkByNoDestroy(usize elements) noexcept {
    VIXEN_ASSERT_EXT(mLength >= elements,
        "tried to shrink a(n) {}-element long vector by {}",
        mLength,
        elements);

    shrinkToNoDestroy(mLength - elements);
}

template <typename T>
inline void Vector<T>::shrinkTo(usize length) noexcept {
    auto prevLength = mLength;
    shrinkToNoDestroy(length);

    for (usize i = length; i < prevLength; ++i) {
        mData[i].~T();
    }
}

template <typename T>
inline void Vector<T>::shrinkToNoDestroy(usize length) noexcept {
    VIXEN_DEBUG_ASSERT_EXT(length <= mLength,
        "tried to shrink a(n) {}-element long vector to {} element(s), which is greater than the current length",
        mLength,
        length);

    mLength = length;
}

template <typename T>
inline void Vector<T>::unsafeSetLength(usize newLength) noexcept {
    VIXEN_ASSERT_EXT(newLength < mCapacity,
        "tried to set the length of a vector to {}, but its capacity was {}.",
        newLength,
        mCapacity);
    if (newLength > mLength) {
        mLength = newLength;
    } else if (newLength < mLength) {
        shrinkTo(newLength);
    }
}

template <typename T>
inline void Vector<T>::unsafeSetLengthNoDestroy(usize newLength) noexcept {
    VIXEN_ASSERT_EXT(newLength < mCapacity,
        "tried to set the length of a vector to {}, but its capacity was {}.",
        newLength,
        mCapacity);
    mLength = newLength;
}

template <typename T, typename U, typename... Us>
constexpr void constructSequenceAt(T *location, usize &outSuccessfulCount, U &&head, Us &&...tail) {
    new (location) T{std::forward<U>(head)};
    outSuccessfulCount += 1;

    if constexpr (sizeof...(Us) > 0) {
        constructSequenceAt(location + 1, outSuccessfulCount, std::forward<Us>(tail)...);
    }
}

template <typename T>
template <typename... Us>
inline void Vector<T>::insertLast(Us &&...values) {
    if constexpr (sizeof...(Us) > 0) {
        ensureCapacity(sizeof...(Us));

        // NOTE: i believe that most major compilers have zero-cost exception handling for the
        // common case, so we should only pay for it if an exception is actually thrown.
        usize elementsInitialized = 0;
        try {
            constructSequenceAt(ptrTo(mLength), elementsInitialized, std::forward<Us>(values)...);
        } catch (...) {
            // rollback
            for (usize i = 0; i < elementsInitialized; ++i) {
                mData[mLength + i].~T();
            }
            throw;
        }
        mLength += sizeof...(Us); // commit
    }
}

template <typename T>
template <typename... Args>
inline void Vector<T>::emplaceLast(Args &&...args) {
    ensureCapacity(1);
    // nothing to rollback
    new (ptrTo(mLength)) T{std::forward<Args>(args)...};
    mLength += 1; // commit
}

template <typename T, typename U, typename... Us>
constexpr void vectorEmplaceSwapPack(Vector<T> &vector, usize i, U &&head, Us &&...tail) {
    vector.emplaceSwap(i, std::forward<U>(head));
    if constexpr (sizeof...(Us) > 0) {
        vectorEmplaceSwapPack(vector, i + 1, std::forward<Us>(tail)...);
    }
}

template <typename T>
template <typename... Us>
View<T> Vector<T>::insertSwap(usize idx, Us &&...values) {
    VIXEN_DEBUG_ASSERT_EXT(idx <= mLength,
        "tried to insert element at {}, but the length was {}",
        idx,
        mLength);
    ensureCapacity(sizeof...(Us));

    if (idx == mLength) {
        usize elementsInitialized = 0;
        try {
            constructSequenceAt(ptrTo(mLength), elementsInitialized, std::forward<Us>(values)...);
        } catch (...) {
            // rollback
            for (usize i = 0; i < elementsInitialized; ++i) {
                mData[idx + i].~T();
            }
            throw;
        }
        mLength += sizeof...(Us); // commit
    } else {
        vectorEmplaceSwapPack(*this, idx, std::forward<Us>(values)...);
    }

    return View{ptrTo(idx), sizeof...(Us)};
}

template <typename T>
template <typename... Args>
T &Vector<T>::emplaceSwap(usize idx, Args &&...args) {
    VIXEN_DEBUG_ASSERT_EXT(idx <= mLength,
        "tried to insert element at {}, but the length was {}",
        idx,
        mLength);

    ensureCapacity(1);

    if (idx == mLength) {
        // nothing to rollback
        new (ptrTo(mLength)) T{std::forward<Args>(args)...};
        mLength += 1; // commit
    } else {
        // convoluted swap operation that handles uninitialized memory.
        new (ptrTo(mLength)) T{mv(mData[idx])};
        mData[idx].~T();
        try {
            new (ptrTo(idx)) T{std::forward<Args>(args)...};
            mLength += 1; // commit
        } catch (...) {
            // rollback
            new (ptrTo(idx)) T{mv(mData[mLength])};
            mData[mLength].~T();
            throw;
        }
    }

    return mData[idx];
}

constexpr usize zeroClampedSub(usize a, usize b) {
    return b > a ? 0 : a - b;
}

template <typename T>
template <typename... Us>
View<T> Vector<T>::insertShift(usize idx, Us &&...values) {
    VIXEN_DEBUG_ASSERT_EXT(idx <= mLength,
        "tried to insert element at {}, but the length was {}",
        idx,
        mLength);

    ensureCapacity(sizeof...(Us));

    if (idx == mLength) {
        usize elementsInitialized = 0;
        try {
            constructSequenceAt(ptrTo(idx), elementsInitialized, std::forward<Us>(values)...);
        } catch (...) {
            // rollback
            for (usize i = 0; i < elementsInitialized; ++i) {
                mData[idx + i].~T();
            }
            throw;
        }
        mLength += sizeof...(Us); // commit
        return View{ptrTo(idx), sizeof...(Us)};
    }

    auto uninitCopyAmount = util::min(sizeof...(Us), mLength - idx);
    auto uninitSrcIndex = mLength - uninitCopyAmount;
    auto uninitDstIndex = uninitSrcIndex + sizeof...(Us);
    util::arrayMoveUninitializedNonoverlapping(ptrTo(uninitSrcIndex),
        ptrTo(uninitDstIndex),
        sizeof...(Us));

    usize initCopyAmount = zeroClampedSub(mLength - idx, uninitCopyAmount);
    util::arrayMove(ptrTo(idx), ptrTo(idx + sizeof...(Us)), initCopyAmount);

    for (usize i = 0; i < sizeof...(Us); ++i) {
        mData[idx + i].~T();
    }

    // in-place construction
    usize elementsInitialized = 0;
    try {
        constructSequenceAt(ptrTo(idx), elementsInitialized, std::forward<Us>(values)...);
        mLength += sizeof...(Us);
    } catch (...) {
        // because i dont feel like figuring out how to keep ordering sane here, well just satisfy
        // the basic exception safety guarantee... which means we can screw up ordering all we want,
        // but we can't just leak our resources. So, we just swap what we've done so far back into
        // place and shrink the vector back to its orininal size.

        // undo the insertion of our new elements, to leave an blank uninit space large enough to
        // swap all of our tail into.
        for (usize i = 0; i < elementsInitialized; ++i) {
            mData[idx + i].~T();
        }

        // ...and swap the tail in. god this is o much easier to write when i can assume the move
        // constructor and destructor won't throw!
        for (usize i = 0; i < uninitCopyAmount; ++i) {
            new (ptrTo(idx + i)) T{mv(mData[uninitDstIndex + i])};
            mData[uninitDstIndex + i].~T();
        }

        throw;
    }

    return View{ptrTo(idx), sizeof...(Us)};
}

template <typename T>
template <typename... Args>
T &Vector<T>::emplaceShift(usize idx, Args &&...args) {
    VIXEN_ASSERT_EXT(idx <= mLength,
        "tried to insert element at {}, but the length was {}",
        idx,
        mLength);
    ensureCapacity(1);

    if (idx == mLength) {
        // we don't need to guard exceptions here because we only grow the length *after* we unpack
        // all the new elements, so there's no possibility of accidentally including uninitialized
        // elements
        new (ptrTo(mLength)) T{std::forward<Args>(args)...};
        mLength += 1;
    } else {
        new (ptrTo(mLength)) T{mv(mData[mLength - 1])};

        usize initCopyAmount = mLength - idx - 1;
        util::arrayMove(ptrTo(idx), ptrTo(idx + 1), initCopyAmount);

        mData[idx].~T();

        try {
            new (ptrTo(idx)) T{std::forward<Args>(args)...};
            mLength += 1;
        } catch (...) {
            new (ptrTo(idx)) T{mv(mData[mLength])};
            mData[mLength].~T();
            throw;
        }
    }

    return mData[idx];
}

template <typename T>
inline T Vector<T>::removeLast() noexcept {
    mLength -= 1;

    T oldValue{mv(mData[mLength])};
    mData[mLength].~T();
    return mv(oldValue);
}

template <typename T>
inline T Vector<T>::removeSwap(usize idx) noexcept {
    VIXEN_DEBUG_ASSERT_EXT(mLength > idx,
        "tried to remove element {} from a {}-element vector",
        idx,
        mLength);

    mLength -= 1;

    T oldValue{mv(mData[idx])};
    swap(mLength, idx);
    // we have to actually end the lifetime of the object we moved out of :p
    mData[mLength].~T();
    return mv(oldValue);
}

template <typename T>
inline T Vector<T>::removeShift(usize idx) noexcept {
    VIXEN_DEBUG_ASSERT_EXT(mLength > idx,
        "tried to remove element {} from a {}-element vector",
        idx,
        mLength);

    mLength -= 1;

    T oldValue{mv(mData[idx])};
    util::arrayMove(ptrTo(idx + 1), ptrTo(idx), mLength - idx);
    mData[mLength].~T();
    return mv(oldValue);
}

template <typename T>
inline void Vector<T>::removeAll() noexcept {
    for (usize i = 0; i < mLength; ++i) {
        mData[i].~T();
    }
    mLength = 0;
}

template <typename T>
inline void Vector<T>::dedupUnstable() noexcept {
    for (usize i = 0; i < mLength; ++i) {
        for (usize j = i + 1; j < mLength; ++j) {
            if (mData[i] == mData[j]) {
                removeSwap(j);
            }
        }
    }
}

template <typename T>
inline void Vector<T>::dedup() noexcept {
    for (usize i = 0; i < mLength; ++i) {
        for (usize j = i + 1; j < mLength; ++j) {
            if (mData[i] == mData[j]) {
                removeShift(j);
            }
        }
    }
}

template <typename T>
inline Option<usize> Vector<T>::firstIndexOf(const T &value) noexcept {
    for (usize i = 0; i < mLength; ++i) {
        if (mData[i] == value) {
            return i;
        }
    }

    return {};
}

template <typename T>
inline Option<usize> Vector<T>::lastIndexOf(const T &value) noexcept {
    for (usize i = 0; i < mLength; ++i) {
        if (mData[mLength - i - 1] == value) {
            return mLength - i - 1;
        }
    }

    return {};
}

template <typename T>
inline void Vector<T>::swap(usize a, usize b) noexcept {
    VIXEN_DEBUG_ASSERT_EXT((mLength > a) && (mLength > b),
        "Tried swapping elements {} and {} in a(n) {}-element vector.",
        a,
        b,
        mLength);

    std::swap(mData[a], mData[b]);
}

template <typename T>
inline const T &Vector<T>::operator[](usize i) const noexcept {
    _VIXEN_BOUNDS_CHECK(i, mLength);
    return mData[i];
}

template <typename T>
inline T &Vector<T>::operator[](usize i) noexcept {
    _VIXEN_BOUNDS_CHECK(i, mLength);
    return mData[i];
}

template <typename T>
inline Vector<T>::operator View<const T>() const noexcept {
    return {mData, mLength};
}

template <typename T>
inline Vector<T>::operator View<T>() noexcept {
    return {mData, mLength};
}

template <typename T>
template <typename S>
S &Vector<T>::operator<<(S &s) {
    s << "[";
    if (mLength > 0) {
        s << mData[0];
        for (usize i = 0; i < mLength; ++i) {
            s << ", " << mData[i];
        }
    }
    s << "]";
    return s;
}

template <typename T>
inline usize Vector<T>::nextCapacity() noexcept {
    return mCapacity == 0 ? DEFAULT_VEC_CAPACITY : 2 * mCapacity;
}

template <typename T>
inline void Vector<T>::ensureCapacity(usize elementsNeeded) {
    usize minimumCapacityNeeded = mLength + elementsNeeded;
    if (minimumCapacityNeeded >= mCapacity) {
        setCapacity(util::max(nextCapacity(), minimumCapacityNeeded));
    }
}

template <typename T>
inline void Vector<T>::setCapacity(usize cap) {
    VIXEN_ASSERT_EXT(mAlloc != nullptr, "Tried to grow a vector with no allocator.");

    if (cap > 0) {
        auto oldLayout = heap::Layout::arrayOf<T>(mCapacity);
        auto newLayout = heap::Layout::arrayOf<T>(cap);
        mData = static_cast<T *>(mAlloc->realloc(oldLayout, newLayout, static_cast<void *>(mData)));
        mCapacity = cap;
    }
}

template <typename T, typename H>
inline void hash(const Vector<T> &values, H &hasher) {
    for (const T &value : values) {
        hash(value, hasher);
    }
}

} // namespace vixen
