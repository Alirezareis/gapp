/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "pop_update.hpp"
#include "../core/ga_info.hpp"
#include "../utility/algorithm.hpp"
#include "../utility/math.hpp"
#include "../utility/utility.hpp"
#include <algorithm>
#include <numeric>
#include <cassert>

namespace genetic_algorithm::update
{
    std::vector<size_t> KeepChildren::operator()(const GaInfo& ga,
                                                 FitnessMatrix::const_iterator first,
                                                 FitnessMatrix::const_iterator children_first,
                                                 FitnessMatrix::const_iterator last)
    {
        assert((children_first - first) > 0);
        assert(size_t(children_first - first) == ga.population_size());
        assert((children_first - first) <= (last - children_first));

        GA_UNUSED(first, children_first, last);

        return detail::index_vector(ga.population_size(), ga.population_size());
    }

    Elitism::Elitism(size_t n)
    {
        elite_num(n);
    }

    void Elitism::elite_num(size_t n) noexcept
    {
        n_ = n;
    }

    std::vector<size_t> Elitism::operator()(const GaInfo& ga,
                                            FitnessMatrix::const_iterator first,
                                            FitnessMatrix::const_iterator children_first,
                                            FitnessMatrix::const_iterator last)
    {
        assert(children_first - first > 0);
        assert(size_t(children_first - first) == ga.population_size());
        assert(children_first - first <= last - children_first);
        assert(std::all_of(first, last, [](const FitnessVector& fvec) { return !fvec.empty(); }));

        GA_UNUSED(last);

        auto sorted_parent_indices =
        detail::partial_argsort(first, first + n_, children_first,
        [](const FitnessVector& lhs, const FitnessVector& rhs) noexcept
        {
            return math::paretoCompareLess(rhs, lhs); // descending
        });

        std::vector<size_t> indices(ga.population_size());
        std::copy(sorted_parent_indices.cbegin(), sorted_parent_indices.cbegin() + n_, indices.begin());
        std::iota(indices.begin() + n_, indices.end(), ga.population_size());

        return indices;
    }

    std::vector<size_t> KeepBest::operator()(const GaInfo& ga,
                                             FitnessMatrix::const_iterator first,
                                             FitnessMatrix::const_iterator children_first,
                                             FitnessMatrix::const_iterator last)
    {
        assert(children_first - first > 0);
        assert(size_t(children_first - first) == ga.population_size());
        assert(children_first - first <= last - children_first);
        assert(std::all_of(first, last, [](const FitnessVector& fvec) { return !fvec.empty(); }));

        GA_UNUSED(children_first);

        auto sorted_indices =
        detail::partial_argsort(first, first + ga.population_size(), last,
        [](const FitnessVector& lhs, const FitnessVector& rhs) noexcept
        {
            return math::paretoCompareLess(rhs, lhs); // descending
        });
        sorted_indices.resize(ga.population_size());

        return sorted_indices;
    }

} // namespace genetic_algorithm::update