#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/bits/stream/basic.inl"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/tuple.hpp"
#include "vixen/typeops.hpp"

// dear god I'm so bad at documentation

/// @defgroup vixen_streams Streams
/// @brief Defines various stream sinks and adapters.
///
/// Stream components (the `C` or `Cs` in a lot of template parameters) must define one of two
/// interfaces, depending on what sort of component they are:
/// - Adapters, which take an input, and may push some sort of value down the pipeline.
/// - Sinks, which terminate a pipeline.
///
/// A pipeline contains stream components, and complete pipelines will execute all stream components
/// in order, from first to last.
///
/// Any pipeline that does not have a sink component as the last component is *incomplete*.
/// Incomplete pipelines may be constructed, but any attempts to push values down the pipeline may
/// not work, most likely resulting in a compile error. Incomplete pipelines are useful for
/// composition, as they allow larger, more complicated pipelines to be given a single name, and
/// reused.
///
/// To conform to the adapter interface, the adapter type must define these operations:
/// @code
/// struct some_adapter {
///     template <typename P, typename... Is>
///     void push(P &pipeline, Is... items);
/// };
/// @endcode
///
/// To conform to the sink interface, the sink type must define these operations:
/// @code
/// struct some_sink {
///     template <typename... Is>
///     void push(Is... items);
/// };
/// @endcode

/// @file
/// @ingroup vixen_streams
/// @brief The main stream interface, containing the stream concatenation operator, the definition
/// of `pipeline`, and core stream components.

namespace vixen::stream {

/// @ingroup vixen_streams
/// @brief Lifts a conforming stream component and makes it usable with generic stream utilities.
template <typename T, typename C>
StreamPipeline<T, C> make_pipeline(C &&component) {
    return StreamPipeline<T, C>{mv(component)};
}

/// @ingroup vixen_streams
/// @brief Creates an adapter that takes an input `n` and pushes an lvalue reference of `n` to each
/// child sink.
template <typename T, typename... Pipelines>
inline StreamPipeline<T, detail::split_sink<Pipelines...>> broadcast(Pipelines &&...pipelines) {
    return make_pipeline<T>(detail::split_sink<Pipelines...>{Tuple<Pipelines...>(pipelines...)});
}

/// @ingroup vixen_streams
/// @brief Creates a sink that takes an input `n` and calls `coll.push(n)`.
template <typename Coll>
inline StreamPipeline<typename collection_types<Coll>::value_type, detail::back_inserter_sink<Coll>>
    back_inserter(Coll &collection) {
    return make_pipeline<typename collection_types<Coll>::value_type>(
        detail::back_inserter_sink<Coll>{std::addressof(collection)});
}

/// @ingroup vixen_streams
/// @brief Creates an adapter that takes an input `n` and pushes `n` if `predicate(n)` returns true.
template <typename F>
inline StreamPipeline<typename function_traits<F>::argument<0>, detail::filter_adapter<F>> filter(
    F &&predicate) {
    return make_pipeline<typename function_traits<F>::argument<0>>(
        detail::filter_adapter<F>{predicate});
}

/// @ingroup vixen_streams
/// @brief Creates an adapter that takes an input `n` and pushes `mapper(n)`.
///
/// @code
/// usize add_5(usize x) { return x + 5; }
///
/// vixen::vector<usize> output;
/// auto stream = map(add_5) | back_inserter(output);
/// stream.push(1);
/// stream.push(2);
/// stream.push(3);
///
/// // `output` should now contain the values [6, 7, 8]
/// @endcode
template <typename F>
inline StreamPipeline<typename function_traits<F>::argument<0>, detail::map_adapter<F>> map(
    F &&mapper) {
    return make_pipeline<typename function_traits<F>::argument<0>>(detail::map_adapter<F>{mapper});
}

/// @ingroup vixen_streams
/// @brief Creates an output iterator that pushes all writes to the passed-in stream.
template <typename T, typename CA, typename... Cs>
inline detail::output_iterator_source<T, CA, Cs...> make_stream_output_iterator(
    StreamPipeline<T, CA, Cs...> &stream) {
    return detail::output_iterator_source(stream);
}

/// @ingroup vixen_streams
/// @brief Creates a sink that takes an input `n` and calls `func(n)`
template <typename F>
inline StreamPipeline<typename function_traits<F>::argument<0>, detail::functor_sink<F>> make_sink(
    F &&functor) {
    return make_pipeline<typename function_traits<F>::argument<0>>(
        detail::functor_sink<F>{std::forward<F>(functor)});
}

/// @ingroup vixen_streams
/// @brief Stream concatenation operator.
///
/// Creates a new pipeline that contains all of the components from `first`, followed by all the
/// components from `last`.
template <typename T, typename U, typename... As, typename... Bs>
inline StreamPipeline<T, As..., Bs...> operator|(
    StreamPipeline<T, As...> &&first, StreamPipeline<U, Bs...> &&last) {
    return first.append_pipeline(mv(last));
}

} // namespace vixen::stream
