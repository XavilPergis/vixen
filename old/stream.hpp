#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/bits/stream/basic.inl"
#include "vixen/function.hpp"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/tuple.hpp"

// dear god I'm so bad at documentation

/**
 * @file
 * @ingroup vixen_streams
 * @brief The main stream interface, containing the stream concatenation operator, the definition
 * of `pipeline`, and core stream components.
 */

/**
 * @defgroup vixen_streams Streams
 * @brief Defines various stream sinks and adapters
 *
 * # Overview
 * Stream pipelines are an ordered collection of pipeline *components*, starting with any number of
 * adapters, and ending with one sink. Pipelines are "push-based", in contrast to "pull-based"
 * iterables in a lot of other languages. Instead of asking the pipeline what the next value it
 * contains is, or "pulling" from the bottom, you force items through the pipeline by "pushing" them
 * in the top. You can think of pipelines like a nice recursive onion. Each layer of the onion wraps
 * a whole complete core, translating from *its* input to the input of the core. In doing so, it
 * becomes part of the core, which now has the type of the new outer layer. In the very center is a
 * layer that has no subsequent layers, and ends the pipeline.
 *
 * Any pipeline that does not have a sink component as the last component is said to be
 * *incomplete*. Incomplete pipelines may be constructed, but the behavior of the pipeline when
 * items are pushed into it is not defined. Incomplete pipelines are useful for composition, as they
 * allow larger, more complicated pipelines to be given a single name, and reused.
 *
 * # Implementing a Component
 * When defining a pipeline component, two members must be present.
 * - a templated `Output` typedef, which describes what the component will produce given its
 * template parameter as an input.
 * - a templated `push` member function, that describes the behavior of the component. The exact
 * type that the input parameter takes is a bit flexible, and can be any value that is convertible
 * from its template parameter.
 *
 * For adapters, the first argument is a pipeline constructed from the subsequent components in the
 * pipeline, to which you should push the output of your component. The second argument is the item
 * that was input into the component. For sinks, the child pipeline argument is dropped, any only
 * the pipeline input is recieved.
 *
 * Note that `push` is declared to return `void` in either case, so `Output` describes the child
 * pipeline's input.
 *
 * When a pipeline component recieves an input, it is free to do as it likes with all the context it
 * has, including calling its downstream pipeline multiple times or not at all, or transforming the
 * input to a different type, or causing side effects. The sky's the limit!
 *
 * ## Examples
 * @code
 * struct ExampleSink {
 *     template <typename ComponentInput>
 *     using Output = void;
 *
 *     template <typename T>
 *     void push(ConvertibleFrom<T> item) { ... }
 * };
 * @endcode
 *
 * @code
 * struct ExampleAdapter {
 *     template <typename ComponentInput>
 *     using Output = ComponentOutput;
 *
 *     template <typename P, typename T>
 *     void push(P &subsequent, ConvertibleFrom<T> item) {
 *         subsequent.push(...);
 *     }
 * };
 * @endcode
 */

namespace vixen::stream {

/**
 * @ingroup vixen_streams
 * @brief lifts a conforming stream component and makes it usable with generic stream utilities
 *
 * @note lifting a component does not necessarily produce a *complete* pipeline, that is only the
 * case when a sink component is lifted.
 *
 * @param component component to lift into a pipeline
 * @return a pipeline consisting of only `component`
 */
template <typename T, typename C>
StreamPipeline<T, C> lift_pipeline_component(C &&component) {
    return StreamPipeline<T, C>{mv(component)};
}

template <typename... Ps>
struct UnifyPipelines;

template <typename P0, typename P1, typename... Ps>
struct UnifyPipelines<P0, P1, Ps...> {
    using Type = typename P0::PipelineInput;

    static_assert(std::is_same_v<Type, typename UnifyPipelines<P1, Ps...>::Type>,
        "all pipelines in a `sequence` must accept the same input type!");
};

template <typename P>
struct UnifyPipelines<P> {
    using Type = typename P::PipelineInput;
};

/**
 * @ingroup vixen_streams
 * @brief creates a sink that pushes each input to all child pipelines
 *
 * This sink will call `push` on each of its child pipelines in the order they were specified in
 * `pipelines`. This component is a sink because adapters do not store their downstream pipelines
 * themselves, instead recieving them upon the execution of the pipeline. This means that each
 * component can have exactly zero or one downstream component, but in the case of this component,
 * we want to have *multiple* downstream. The way we deal with this is by storing all the complete
 * child pipelines directly in the *component itself*, and by pushing references into the top of
 * each one.
 *
 * @note because this sink dispatches to multiple others, the best it can do is push references
 * downstream, so be wary of any implicit copy construction that may happen
 */
template <typename... Pipelines>
inline StreamPipeline<typename UnifyPipelines<Pipelines...>::Type, detail::SplitSink<Pipelines...>>
    sequence(Pipelines &&...pipelines) {
    return lift_pipeline_component<typename UnifyPipelines<Pipelines...>::Type>(
        detail::SplitSink<Pipelines...>{Tuple<Pipelines...>(pipelines...)});
}

template <typename F>
using FirstArgumentType = typename function_traits<F>::template argument<0>;

template <typename F, usize Idx>
using FnArgument = typename function_traits<F>::template argument<Idx>;

template <typename Coll>
using CollectionType = typename collection_types<Coll>::value_type;

/**
 * @ingroup vixen_streams
 * @brief creates an adapter that applies `mapper` to each input
 *
 * @param mapper a callable object that takes in a (possibly rvalue) reference to the component
 * input, and returns an expression with arbitrary type
 */
template <typename F>
inline StreamPipeline<FnArgument<F, 0>, detail::MapAdapter<F>> map(F &&mapper) {
    return lift_pipeline_component<FnArgument<F, 0>>(detail::MapAdapter<F>{mapper});
}

/**
 * @ingroup vixen_streams
 * @brief creates an adapter that passes through its inputs if the input meets the condition defined
 * by `predicate`
 *
 * @param predicate a callable object that takes in a reference to the component input, and returns
 * an expression convertible to `bool`
 */
template <typename F>
inline StreamPipeline<FnArgument<F, 0>, detail::FilterAdapter<F>> filter(F &&predicate) {
    return lift_pipeline_component<FnArgument<F, 0>>(detail::FilterAdapter<F>{predicate});
}

/**
 * @ingroup vixen_streams
 * @brief creates a sink that inserts items into the back of the passed-in collection
 */
template <typename Coll>
requires InsertableContainer<RemoveReference<Coll>>
inline StreamPipeline<ContainedType<RemoveReference<Coll>>, detail::BackInserterSink<Coll>>
    back_inserter(Coll &collection) {
    return lift_pipeline_component<ContainedType<RemoveReference<Coll>>>(
        detail::BackInserterSink<Coll>{std::addressof(collection)});
}

/**
 * @ingroup vixen_streams
 * @brief wraps a stream into a C++ output iterator
 *
 * All writes into the output iterator are pushed into the wrapped pipeline.
 */
template <typename T, typename CA, typename... Cs>
inline detail::OutputIteratorSource<T, CA, Cs...> make_stream_output_iterator(
    StreamPipeline<T, CA, Cs...> &stream) {
    return detail::OutputIteratorSource(stream);
}

/**
 * @ingroup vixen_streams
 * @brief creates a sink that calls `functor` with the input that it recieved
 */
template <typename F>
inline StreamPipeline<FnArgument<F, 0>, detail::FunctorSink<F>> make_sink(F &&functor) {
    return lift_pipeline_component<FnArgument<F, 0>>(
        detail::FunctorSink<F>{std::forward<F>(functor)});
}

/**
 * @ingroup vixen_streams
 * @brief the pipeline concatenation operator
 *
 * This operator creates a new pipeline comprised of all the components from `first` followed by all
 * the components from `last`. This is the main way to compose streams together.
 */
template <typename T, typename U, typename... As, typename... Bs>
inline StreamPipeline<T, As..., Bs...> operator|(StreamPipeline<T, As...> &&first,
    StreamPipeline<U, Bs...> &&last) {
    return first.append_pipeline(mv(last));
}

} // namespace vixen::stream
