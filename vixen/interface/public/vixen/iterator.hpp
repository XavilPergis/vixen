#pragma once

#include "vixen/option.hpp"
#include "vixen/tuple.hpp"
#include "vixen/types.hpp"

#include <iterator>

#define DECLVAL(Ty) std::declval<Ty>()

namespace vixen::iter {

template <typename... Cs>
struct reified_pipeline;

template <typename CA, typename CB, typename... Cs>
struct reified_pipeline<CA, CB, Cs...> {
    using type = typename CA::reified<typename reified_pipeline<CB, Cs...>::type>;
};

template <typename C>
struct reified_pipeline<C> {
    using type = typename C::reified;
};

template <typename... Cs>
struct iterator_pipeline;

template <typename CA, typename CB, typename... Cs>
struct iterator_pipeline<CA, CB, Cs...> {
    using child_pipeline = iterator_pipeline<CB, Cs...>;
    // using pipeline_output = typename CA::output<iterator_pipeline<CB, Cs...>::pipeline_output>;
    // using reified_component = typename reified_pipeline<CA, CB, Cs...>::type;

    CA component;
    child_pipeline downstream;

    template <typename... As>
    using concat_pipeline = iterator_pipeline<CA, CB, Cs..., As...>;

    template <typename... As>
    concat_pipeline<As...> concat(iterator_pipeline<As...> &&concatenated) {
        return concat_pipeline<As...>{mv(component), downstream.concat(mv(concatenated))};
    }
};

template <typename C>
struct iterator_pipeline<C> {
    // using pipeline_output = typename C::output<void>;
    // using reified_component = typename reified_pipeline<C>::type;

    C component;

    template <typename... As>
    iterator_pipeline<C, As...> concat(iterator_pipeline<As...> &&concatenated) {
        return iterator_pipeline<C, As...>(mv(component), mv(concatenated));
    }
};

template <typename C, typename... Cs>
auto reify(iterator_pipeline<C, Cs...> &&pipeline) {
    using downstream_reified = typename reified_pipeline<Cs...>::type;
    return typename C::reified<downstream_reified>(mv(pipeline.component), mv(pipeline.downstream));
}

template <typename C>
auto reify(iterator_pipeline<C> &&pipeline) {
    return typename C::reified{mv(pipeline.component)};
}

// template <typename Iter>
// // using pipeline_output = typename Iter::pipeline_output;

template <typename Iter>
using iter_output = typename Iter::output;

/// @ingroup vixen_iterators
/// @brief Lifts a conforming iterator component and makes it usable with generic iterator
/// utilities.
template <typename C>
iterator_pipeline<C> make_pipeline(C &&component) {
    return iterator_pipeline<C>{mv(component)};
}

// template <typename... Is>
// struct zip_iterator {
//     template <typename Iter>
//     struct iter_mapper {
//         using type = iter_output<Iter>;
//     };

//     using output = cons_tuple<map<iter_mapper, unpack<Is...>>>;

//     tuple<Is...> iters;

//     bool has_next() {
//         return iters.fold(true, [](bool acc, auto &iter) {
//             return acc && iter.has_next();
//         });
//     }

//     output next() {
//         return mapper(prev.next());
//     }
// };

template <typename F>
struct map_iterator {
    F mapper;

    template <typename Iter>
    struct reified {
        reified(map_iterator &&desc, Iter &&downstream)
            : mapper(mv(desc.mapper)), downstream(mv(downstream)) {}

        using output = decltype(std::declval<F>()(std::declval<iter_output<Iter>>()));

        F mapper;
        Iter downstream;

        bool has_next() {
            return downstream.has_next();
        }

        output next() {
            return mapper(downstream.next());
        }
    };
};

/// @ingroup vixen_iterators
template <typename F>
inline iterator_pipeline<map_iterator<F>> map(F &&mapper) {
    return make_pipeline(map_iterator<F>{mv(mapper)});
}

template <typename F>
struct filter_iterator {
    F predicate;

    template <typename Iter>
    struct reified {
        reified(filter_iterator &&desc, Iter &&downstream)
            : predicate(mv(desc.predicate)), downstream(mv(downstream)) {}

        using output = iter_output<Iter>;

        F predicate;
        Iter downstream;
        Option<output> last_output;

        bool has_next() {
            if (last_output)
                return true;
            loop {
                if (!downstream.has_next()) {
                    return false;
                    last_output = empty_opt;
                }
                last_output = downstream.next();
                if (predicate(*last_output)) {
                    return true;
                }
            }
        }

        output next() {
            auto ret = mv(*last_output);
            last_output = empty_opt;
            return mv(ret);
        }
    };
};

/// @ingroup vixen_iterators
/// @brief Creates an adapter that takes an input `n` and pushes `n` if `predicate(n)` returns true.
template <typename F>
inline iterator_pipeline<filter_iterator<F>> filter(F &&predicate) {
    return make_pipeline(filter_iterator<F>{mv(predicate)});
}

template <typename T>
struct slice_iterator {
    T *begin;
    T *end;

    struct reified {
        reified(slice_iterator &&desc) : cur(desc.begin), end(desc.end) {}

        using output = T &;

        T *cur;
        T *end;

        bool has_next() {
            return cur < end;
        }

        output next() {
            return *(cur++);
        }
    };
};

template <typename II>
struct std_to_iter {
    II begin;
    II end;

    struct reified {
        reified(std_to_iter &&desc) : cur(desc.begin), end(desc.end) {}

        using output = decltype(*std::declval<II>());

        II cur;
        II end;

        bool has_next() {
            return cur != end;
        }

        output next() {
            return *(cur++);
        }
    };
};

template <typename II>
std_to_iter<II> wrap_input_iterator(II begin, II end) {}

template <typename Iter>
struct iter_to_std : public std::iterator<std::input_iterator_tag, typename Iter::output> {
    explicit iter_to_std(Iter &&iter) : iter(mv(iter)) {
        load_next();
    }

    iter_to_std(iter_to_std &&other) = default;

    using value_type = typename Iter::output;
    using reference = typename Iter::output &;

    // end iterator
    iter_to_std() {}

    iter_to_std begin() const {
        return mv(*this);
    }

    iter_to_std end() const {
        return iter_to_std();
    }

    // clang-format off
    reference operator*() { return *prev; }
    iter_to_std &operator++() { load_next(); return *this; }
    bool operator!=(iter_to_std const &rhs) const { return !(*this == rhs); }
    // clang-format on

    bool operator==(iter_to_std const &rhs) const {
        auto matching_end_iterators = prev.isNone() && rhs.prev.isNone();
        return matching_end_iterators || iter == rhs.iter && prev == rhs.prev;
    }

    iter_to_std operator++(int) {
        iter_to_std old(nullptr, mv(*prev));
        load_next();
        return old;
    }

    // only empty if this iterator is an end iterator.
    Option<value_type> prev;
    // only empty if this is a post-increment iterator, in which case it is no longer
    // dereferencable.
    Option<Iter> iter;

private:
    explicit iter_to_std(value_type &&value) : iter(nullptr), prev(mv(value)) {}

    void load_next() {
        if (iter->has_next()) {
            prev = iter->next();
        }
    }
};

template <typename... Cs>
iter_to_std<typename reified_pipeline<Cs...>::type> make_input_iterator(
    iterator_pipeline<Cs...> &&iter) {
    return iter_to_std<typename reified_pipeline<Cs...>::type>(reify(mv(iter)));
}

// NOTE: we do this backwards because values are pulled down the pipeline from front to back,
// starting at the root component and traversing children until the source. When composing a
// pipeline with `operator|`, `a | b` means that `b` will request values from `a` when the
// iterator is executed.
template <typename... As, typename... Bs>
inline iterator_pipeline<Bs..., As...> operator|(
    iterator_pipeline<As...> &&tail, iterator_pipeline<Bs...> &&head) {
    return head.concat(mv(tail));
}

/*

auto foo = iter::reify(thing(a, b) | map(f) | filter(f));
if (foo.has_next()) { foo.next().blah(); }

*/

} // namespace vixen::iter

#define CLS1(...)             \
    ([](auto it) {            \
        return (__VA_ARGS__); \
    })
