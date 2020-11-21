#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/tuple.hpp"

#define DECLVAL(Ty) std::declval<Ty>()

namespace vixen::stream {

template <typename T>
struct is_source : std::false_type {};

template <typename T>
struct is_sink : std::false_type {};

template <typename T>
struct is_adapter : std::conjunction<is_source<T>, is_sink<T>> {};

namespace placeholder {

template <typename F>
struct filter_adapter {
    F predicate;
};

template <typename F>
struct map_adapter {
    F mapper;
};

template <typename... Sinks>
struct split_adapter {
    tuple<Sinks...> sinks;
};

} // namespace placeholder

template <typename Coll>
struct back_inserter_sink {
    static_assert(is_collection<Coll>::value);

    Coll *collection;

    template <typename... Is>
    void push(Is... items) {
        (collection->push(items), ...);
    }
};

template <typename Sink, typename F>
struct filter_adapter {
    F predicate;
    Sink sink;

    template <typename... Is>
    void push(Is... items) {
        if (predicate(items...)) {
            sink.push(items...);
        }
    }
};

template <typename Sink, typename F>
struct map_adapter {
    F mapper;
    Sink sink;

    template <typename... Is>
    void push(Is... items) {
        sink.push(mapper(items...));
    }
};

template <typename... Sinks>
struct split_adapter {
    tuple<Sinks...> sinks;

    template <typename... Is>
    void push(Is... items) {
        sinks.each([&](auto &sink) {
            sink.push(items...);
        });
    }
};

template <typename Sink, typename F>
inline filter_adapter<Sink, F> operator>>=(placeholder::filter_adapter<F> &&stream, Sink &&sink) {
    return filter_adapter<Sink, F>{stream.predicate, sink};
}

template <typename Sink, typename F>
inline map_adapter<Sink, F> operator>>=(placeholder::map_adapter<F> &&stream, Sink &&sink) {
    return map_adapter<Sink, F>{stream.mapper, sink};
}

template <typename... Sinks>
inline split_adapter<Sinks...> split(Sinks &&...sinks) {
    return split_adapter<Sinks...>{tuple<Sinks...>(sinks...)};
}

template <typename Coll>
inline back_inserter_sink<Coll> back_inserter(Coll *collection) {
    return back_inserter_sink<Coll>{collection};
}

template <typename Sink, typename F>
inline placeholder::filter_adapter<F> filter(F &&predicate) {
    return placeholder::filter_adapter<F>{predicate};
}

template <typename Sink, typename F>
inline placeholder::map_adapter<F> map(F &&mapper) {
    return placeholder::map_adapter<F>{mapper};
}

// file_source(alloc, path) >>= split(a >>= b, c >>= d)

} // namespace vixen::stream
