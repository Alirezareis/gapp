/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_UTILITY_ALGORITHM_HPP
#define GA_UTILITY_ALGORITHM_HPP

#include "concepts.hpp"
#include "type_traits.hpp"
#include "utility.hpp"
#include <vector>
#include <tuple>
#include <optional>
#include <algorithm>
#include <numeric>
#include <functional>
#include <iterator>
#include <random>
#include <type_traits>
#include <concepts>
#include <utility>
#include <cstddef>
#include <cassert>

namespace genetic_algorithm::detail
{
    inline std::vector<size_t> index_vector(size_t n, size_t first = 0)
    {
        std::vector<size_t> indices(n);
        std::iota(indices.begin(), indices.end(), first);

        return indices;
    }

    template<std::random_access_iterator Iter, typename Comp = std::less<typename std::iterator_traits<Iter>::value_type>>
    requires std::strict_weak_order<Comp, typename std::iterator_traits<Iter>::value_type,
                                          typename std::iterator_traits<Iter>::value_type>
    std::vector<size_t> argsort(Iter first, Iter last, Comp&& comp = std::less<typename std::iterator_traits<Iter>::value_type>{})
    {
        assert(std::distance(first, last) >= 0);

        auto indices = detail::index_vector(std::distance(first, last));

        if constexpr (detail::is_reverse_iterator_v<Iter>)
        {
            const size_t last_idx = indices.back();
            std::sort(indices.begin(), indices.end(), [&](size_t lidx, size_t ridx)
            {
                return std::invoke(comp, *(first + last_idx - ridx), *(first + last_idx - lidx));
            });
        }
        else
        {
            std::sort(indices.begin(), indices.end(), [&](size_t lidx, size_t ridx)
            {
                return std::invoke(comp, *(first + lidx), *(first + ridx));
            });
        }

        return indices;
    }

    template<std::random_access_iterator Iter, typename Comp = std::less<typename std::iterator_traits<Iter>::value_type>>
    requires std::strict_weak_order<Comp, typename std::iterator_traits<Iter>::value_type,
                                          typename std::iterator_traits<Iter>::value_type>
    std::vector<size_t> partial_argsort(Iter first, Iter middle, Iter last, Comp&& comp = std::less<typename std::iterator_traits<Iter>::value_type>{})
    {
        assert(std::distance(first, middle) >= 0);
        assert(std::distance(middle, last) >= 0);

        auto indices = detail::index_vector(std::distance(first, last));

        if constexpr (detail::is_reverse_iterator_v<Iter>)
        {
            const size_t last_idx = indices.back();
            std::partial_sort(indices.begin(), indices.begin() + std::distance(first, middle), indices.end(),
            [&](size_t lidx, size_t ridx)
            {
                return std::invoke(comp, *(first + last_idx - ridx), *(first + last_idx - lidx));
            });
        }
        else
        {
            std::partial_sort(indices.begin(), indices.begin() + std::distance(first, middle), indices.end(),
            [&](size_t lidx, size_t ridx)
            {
                return std::invoke(comp, *(first + lidx), *(first + ridx));
            });
        }

        return indices;
    }

    template<std::random_access_iterator Iter, typename Comp = std::less<typename std::iterator_traits<Iter>::value_type>>
    requires std::strict_weak_order<Comp, typename std::iterator_traits<Iter>::value_type,
                                          typename std::iterator_traits<Iter>::value_type>
    constexpr size_t argmax(Iter begin, Iter first, Iter last, Comp&& comp = std::less<typename std::iterator_traits<Iter>::value_type>{})
    {
        assert(std::distance(begin, first) >= 0);
        assert(std::distance(first, last) > 0);

        const auto it = std::max_element(first, last, std::forward<Comp>(comp));
        const size_t idx = std::distance(begin, it);

        if constexpr (detail::is_reverse_iterator_v<Iter>)
        {
            const size_t last_idx = std::distance(begin, last) - 1;
            return last_idx - idx;
        }
        else
        {
            return idx;
        }
    }

    template<std::random_access_iterator Iter, typename Comp = std::less<typename std::iterator_traits<Iter>::value_type>>
    requires std::strict_weak_order<Comp, typename std::iterator_traits<Iter>::value_type,
                                          typename std::iterator_traits<Iter>::value_type>
    constexpr size_t argmin(Iter begin, Iter first, Iter last, Comp&& comp = std::less<typename std::iterator_traits<Iter>::value_type>{})
    {
        assert(std::distance(begin, first) >= 0);
        assert(std::distance(first, last) > 0);

        const auto it = std::min_element(first, last, std::forward<Comp>(comp));
        const size_t idx = std::distance(begin, it);

        if constexpr (detail::is_reverse_iterator_v<Iter>)
        {
            const size_t last_idx = std::distance(begin, last) - 1;
            return last_idx - idx;
        }
        else
        {
            return idx;
        }
    }

    template<std::random_access_iterator Iter, typename URBG>
    void partial_shuffle(Iter first, Iter middle, Iter last, URBG&& gen)
    {
        for (; first != middle; ++first)
        {
            const auto max_offset = std::distance(first, last) - 1;
            const auto distribution = std::uniform_int_distribution(0_pd, max_offset);
            const auto offset = distribution(gen);
            const auto new_pos = std::next(first, offset);

            std::iter_swap(first, new_pos);
        }
    }

    template<std::input_iterator Iter>
    constexpr bool contains(Iter first, Iter last, const typename std::iterator_traits<Iter>::value_type& val)
    {
        return std::any_of(first, last, [&](const auto& elem) { return elem == val; });
    }

    template<std::input_iterator Iter, typename Pred>
    requires std::predicate<Pred, dereference_t<typename std::iterator_traits<Iter>::value_type>>
    Iter find_if_ptr(Iter first, Iter last, Pred&& pred)
    {
        assert(std::distance(first, last) >= 0);

        for (; first != last; ++first)
        {
            if (std::invoke(pred, **first)) break;
        }
        return first;
    }

    template<std::input_iterator Iter, typename Pred>
    requires std::predicate<Pred, typename std::iterator_traits<Iter>::value_type>
    std::vector<Iter> find_all(Iter first, Iter last, Pred&& pred)
    {
        assert(std::distance(first, last) >= 0);

        std::vector<Iter> result;
        result.reserve(last - first);

        for (; first != last; ++first)
        {
            if (std::invoke(pred, *first)) result.push_back(first);
        }

        return result;
    }

    template<std::input_iterator Iter, typename Pred>
    requires std::predicate<Pred, typename std::iterator_traits<Iter>::value_type>
    auto find_all_v(Iter first, Iter last, Pred&& pred)
    {
        assert(std::distance(first, last) >= 0);

        using ValueType = typename std::iterator_traits<Iter>::value_type;

        std::vector<ValueType> result;
        result.reserve(last - first);

        std::copy_if(first, last, std::back_inserter(result), std::forward<Pred>(pred));

        return result;
    }

    template<typename T, std::predicate<T> Pred>
    std::vector<size_t> find_indices(const std::vector<T>& container, Pred&& pred)
    {
        std::vector<size_t> indices;
        indices.reserve(indices.size());

        for (size_t i = 0; i < container.size(); i++)
        {
            if (std::invoke(pred, container[i])) indices.push_back(i);
        }

        return indices;
    }

    template<typename T>
    std::optional<size_t> index_of(const std::vector<T>& container, const T& val)
    {
        const auto found = std::find(container.begin(), container.end(), val);
        const size_t idx = std::distance(container.begin(), found);
        
        return idx == container.size() ? std::optional<size_t>{} : idx;
    }

    template<typename T, std::predicate<T> Pred>
    std::optional<size_t> find_index(const std::vector<T>& container, Pred&& pred)
    {
        const auto found = std::find_if(container.begin(), container.end(), std::forward<Pred>(pred));
        const size_t idx = std::distance(container.begin(), found);

        return idx == container.size() ? std::optional<size_t>{} : idx;
    }

    template<typename T>
    std::vector<T> elementwise_min(std::vector<T> lhs, const std::vector<T>& rhs)
    {
        assert(lhs.size() == rhs.size());

        std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), [](const T& lhs, const T& rhs) { return lhs < rhs ? lhs : rhs; });

        return std::move(lhs);
    }

    template<typename T>
    std::vector<T> elementwise_max(std::vector<T> lhs, const std::vector<T>& rhs)
    {
        assert(lhs.size() == rhs.size());

        std::transform(lhs.begin(), lhs.end(), rhs.begin(), lhs.begin(), [](const T& lhs, const T& rhs) { return lhs < rhs ? rhs : lhs; });

        return std::move(lhs);
    }

    template<typename T>
    bool erase_first_stable(std::vector<T>& container, const T& val)
    {
        const auto found = std::find(container.cbegin(), container.cend(), val);
        if (found != container.cend())
        {
            container.erase(found);
            return true;
        }
        return false;
    }

    template<typename ValueType>
    std::vector<ValueType> select(const std::vector<ValueType>& cont, const std::vector<size_t>& indices)
    {
        assert(std::all_of(indices.begin(), indices.end(), [&](size_t idx) { return idx < cont.size(); }));

        std::vector<ValueType> selected;
        selected.reserve(indices.size());

        std::transform(indices.begin(), indices.end(), std::back_inserter(selected), [&](size_t idx) { return cont[idx]; });

        return selected;
    }

    template<typename ValueType>
    std::vector<ValueType> select(std::vector<ValueType>&& cont, const std::vector<size_t>& indices)
    {
        assert(std::all_of(indices.begin(), indices.end(), [&](size_t idx) { return idx < cont.size(); }));

        std::vector<ValueType> selected;
        selected.reserve(indices.size());

        std::transform(indices.begin(), indices.end(), std::back_inserter(selected), [&](size_t idx) { return std::move(cont[idx]); });

        return selected;
    }

    template<typename T,
             std::predicate<T, T> Pred = std::equal_to<T>,
             std::strict_weak_order<T, T> Comp = std::less<T>>
    constexpr void erase_duplicates(std::vector<T>& container, Pred&& pred = std::equal_to<T>{}, Comp&& comp = std::less<T>{})
    {
        std::sort(container.begin(), container.end(), std::forward<Comp>(comp));
        const auto last = std::unique(container.begin(), container.end(), std::forward<Pred>(pred));
        container.erase(last, container.end());
    }

    namespace _
    {
        template<typename... Ts, typename R, typename Tr, typename Rd>
        R transform_reduce_impl(Tr&&, Rd&&, R&& acc, Ts&&...)
        {
            return acc;
        }

        template<typename T, typename... Ts, typename R, typename Tr, typename Rd>
        R transform_reduce_impl(Tr&& tr, Rd&& rd, R&& acc, T&& arg, Ts&&... args)
        {
            auto transform_result = std::invoke(tr, std::forward<T>(arg));

            acc = static_cast<std::remove_reference_t<R>>(std::invoke(rd, std::forward<R>(acc), std::move(transform_result)));

            return transform_reduce_impl(std::forward<Tr>(tr), std::forward<Rd>(rd), std::forward<R>(acc), std::forward<Ts>(args)...);
        }
    }

    template<typename Tuple, typename Acc, typename TransformOp, typename ReduceOp>
    requires is_specialization_of_v<std::remove_reference_t<Tuple>, std::tuple>
    Acc transform_reduce(Tuple&& tup, Acc&& init, TransformOp&& transform, ReduceOp&& reduce)
    {
        auto transform_reduce_ =
        [&init, &transform, &reduce] (auto&&... args) mutable -> Acc
        {
            return _::transform_reduce_impl(std::forward<TransformOp>(transform),
                                            std::forward<ReduceOp>(reduce), 
                                            std::forward<Acc>(init),
                                            std::forward<decltype(args)>(args)...);
        };

        return std::apply(std::move(transform_reduce_), std::forward<Tuple>(tup));
    }

}

#endif // !GA_UTILITY_ALGORITHM_HPP