#pragma once

#include "vixen/allocator/allocator.hpp"
#include "vixen/string.hpp"
#include "vixen/traits.hpp"
#include "vixen/tuple.hpp"

#include <iterator>

namespace vixen::stream {

template <typename T, typename... Cs>
struct StreamPipeline;

template <typename T, typename CA, typename CB, typename... Cs>
struct StreamPipeline<T, CA, CB, Cs...> {
    using ComponentOutput = typename CA::template Output<T>;
    using ChildPipeline = StreamPipeline<ComponentOutput, CB, Cs...>;
    using PipelineOutput = typename ChildPipeline::PipelineOutput;

    CA component;
    ChildPipeline rest;

    template <typename I>
    void push(I &&item) {
        component.push(rest, std::forward<I>(item));
    }

    template <typename... As>
    StreamPipeline<T, CA, CB, Cs..., As...> append_pipeline(
        StreamPipeline<PipelineOutput, As...> &&appended) {
        return StreamPipeline<T, CA, CB, Cs..., As...>{mv(component),
            rest.append_pipeline(mv(appended))};
    }
};

template <typename T, typename C>
struct StreamPipeline<T, C> {
    using PipelineOutput = typename C::template Output<T>;

    C component;

    // Assumes completed pipeline
    template <typename I>
    void push(I item) {
        component.push(std::forward<I>(item));
    }

    // Assumes incomplete pipeline
    template <typename U, typename... As>
    StreamPipeline<T, C, As...> append_pipeline(StreamPipeline<U, As...> &&appended) {
        return StreamPipeline<T, C, As...>{mv(component), mv(appended)};
    }
};

namespace detail {

template <typename Coll>
struct BackInserterSink {
    Coll *collection;

    template <typename I>
    using Output = void;

    template <typename I>
    void push(I &&item) {
        collection->push(std::forward<I>(item));
    }
};

template <typename F>
struct FunctorSink {
    F functor;

    template <typename I>
    using Output = void;

    template <typename I>
    void push(I &&item) {
        functor(std::forward<I>(item));
    }
};

template <typename F>
struct FilterAdapter {
    F predicate;

    template <typename I>
    using Output = I;

    template <typename S, typename I>
    void push(S &sink, I &&item) {
        if (predicate(item)) {
            sink.push(std::forward<I>(item));
        }
    }
};

template <typename F>
struct MapAdapter {
    F mapper;

    template <typename I>
    using Output = decltype(std::declval<F>()(std::declval<I>()));

    template <typename S, typename I>
    void push(S &sink, I item) {
        sink.push(mapper(std::forward<I>(item)));
    }
};

// @note: this is a sink and not an adapter, because sources can only have one downstream sink.
template <typename... Pipelines>
struct SplitSink {
    Tuple<Pipelines...> pipelines;

    template <typename I>
    using Output = void;

    template <typename... Is>
    void push(Is... items) {
        pipelines.each([&](auto &pipeline) {
            pipeline.push(items...);
        });
    }
};

template <typename T, typename... Cs>
struct OutputIteratorSource
    : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    explicit OutputIteratorSource(StreamPipeline<T, Cs...> &adapted)
        : adapted(std::addressof(adapted)) {}

    OutputIteratorSource &operator=(T &&value) {
        adapted->push(std::forward<T>(value));
        return *this;
    }

    OutputIteratorSource &operator=(const T &value) {
        adapted->push(value);
        return *this;
    }

    // clang-format off
    OutputIteratorSource &operator*() { return *this; }
    OutputIteratorSource &operator++() { return *this; }
    OutputIteratorSource &operator++(int) { return *this; }
    // clang-format on

private:
    StreamPipeline<T, Cs...> *adapted;
};

} // namespace detail

} // namespace vixen::stream
