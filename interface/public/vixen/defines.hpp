#pragma once

#if !defined(NDEBUG) || defined(VIXEN_FORCE_DEBUG)
#define VIXEN_IS_DEBUG
#endif

// Defer to the allocator copy-construct operator.
// The varargs here are SO dumb. Something like `HashMap<K, V>` will be interpreted as `HashMap<K`
// and `V>`.
#define VIXEN_DEFINE_CLONE_METHOD(...)                         \
    __VA_ARGS__ clone(::vixen::heap::Allocator &alloc) const { \
        return __VA_ARGS__(copy_tag, alloc, *this);            \
    }
