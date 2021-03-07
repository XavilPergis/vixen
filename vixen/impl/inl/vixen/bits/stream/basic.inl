#pragma once

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
    using component_output = typename CA::output<T>;
    using child_pipeline = StreamPipeline<component_output, CB, Cs...>;
    using pipeline_output = typename child_pipeline::pipeline_output;

    CA component;
    child_pipeline rest;

    template <typename I>
    void push(I &&item) {
        component.push(rest, std::forward<I>(item));
    }

    template <typename... As>
    StreamPipeline<T, CA, CB, Cs..., As...> append_pipeline(
        StreamPipeline<pipeline_output, As...> &&appended) {
        return StreamPipeline<T, CA, CB, Cs..., As...>{mv(component),
            rest.append_pipeline(mv(appended))};
    }
};

template <typename T, typename C>
struct StreamPipeline<T, C> {
    using pipeline_output = typename C::output<T>;

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
struct back_inserter_sink {
    Coll *collection;

    template <typename I>
    using output = void;

    template <typename I>
    void push(I &&item) {
        collection->push(std::forward<I>(item));
    }
};

template <typename F>
struct filter_adapter {
    F predicate;

    template <typename I>
    using output = I;

    template <typename S, typename I>
    void push(S &sink, I &&item) {
        if (predicate(item)) {
            sink.push(std::forward<I>(item));
        }
    }
};

template <typename F>
struct map_adapter {
    F mapper;

    template <typename I>
    using output = decltype(std::declval<F>()(std::declval<I>()));

    template <typename S, typename I>
    void push(S &sink, I item) {
        sink.push(mapper(std::forward<I>(item)));
    }
};

// @note: this is a sink and not an adapter, because sources can only have one downstream sink.
template <typename... Pipelines>
struct split_sink {
    Tuple<Pipelines...> pipelines;

    template <typename I>
    using output = void;

    template <typename... Is>
    void push(Is... items) {
        pipelines.each([&](auto &pipeline) {
            pipeline.push(items...);
        });
    }
};

template <typename T, typename... Cs>
struct output_iterator_source
    : public std::iterator<std::output_iterator_tag, void, void, void, void> {
    explicit output_iterator_source(StreamPipeline<T, Cs...> &adapted)
        : adapted(std::addressof(adapted)) {}

    output_iterator_source &operator=(T &&value) {
        adapted->push(std::forward<T>(value));
        return *this;
    }

    output_iterator_source &operator=(const T &value) {
        adapted->push(value);
        return *this;
    }

    // clang-format off
    output_iterator_source &operator*() { return *this; }
    output_iterator_source &operator++() { return *this; }
    output_iterator_source &operator++(int) { return *this; }
    // clang-format on

private:
    StreamPipeline<T, Cs...> *adapted;
};

} // namespace detail

} // namespace vixen::stream
