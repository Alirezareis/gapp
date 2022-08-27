/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "population.hpp"
#include "../utility/math.hpp"
#include "../utility/algorithm.hpp"
#include <algorithm>
#include <iterator>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstddef>

namespace genetic_algorithm::detail
{
    FitnessVector toFitnessVector(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::all_of(first, last, [](const FitnessVector& f) { return f.size() == 1; }));

        std::vector<double> fvec(last - first);
        std::transform(first, last, fvec.begin(), [](const FitnessVector& f) { return f[0]; });

        return fvec;
    }

    FitnessVector minFitness(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::distance(first, last) > 0);
        assert(std::all_of(first, last, [first](const FitnessVector& sol) { return sol.size() == first->size(); }));

        return std::reduce(std::next(first), last, *first, detail::elementwise_min<double>);
    }

    FitnessVector maxFitness(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::distance(first, last) > 0);
        assert(std::all_of(first, last, [first](const FitnessVector& sol) { return sol.size() == first->size(); }));

        return std::reduce(std::next(first), last, *first, detail::elementwise_max<double>);
    }

    FitnessVector fitnessMean(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::distance(first, last) > 0);
        assert(std::all_of(first, last, [first](const FitnessVector& sol) { return sol.size() == first->size(); }));

        FitnessVector fitness_mean(first->size());
        std::for_each(first, last, [&](const FitnessVector& fvec)
        {
            std::transform(fvec.begin(), fvec.end(), fitness_mean.begin(), fitness_mean.begin(),
            [ninv = 1.0 / (first - last)](double mean, double f) noexcept
            {
                return mean + f * ninv;
            });
        });

        return fitness_mean;
    }

    FitnessVector fitnessStdDev(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        return fitnessStdDev(first, last, fitnessMean(first, last));
    }

    FitnessVector fitnessStdDev(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last, const FitnessVector& mean)
    {
        assert(std::distance(first, last) > 0);
        assert(std::all_of(first, last, [first](const FitnessVector& sol) { return sol.size() == first->size(); }));

        FitnessVector variance(first->size());

        if (std::distance(first, last) == 1) return variance;

        std::for_each(first, last, [&, ninv = 1.0 / (last - first - 1)](const FitnessVector& fvec) noexcept
        {
            for (size_t dim = 0; dim < fvec.size(); dim++)
            {
                variance[dim] += std::pow(fvec[dim] - mean[dim], 2) * ninv;
            }
        });

        for (double& var : variance)
        { 
            var = std::sqrt(var);
        }

        return variance;
    }

} // namespace genetic_algorithm::detail

namespace genetic_algorithm::detail::_
{
    std::vector<size_t> findParetoFront1D(const FitnessMatrix& fmat)
    {
        const auto best = std::max_element(fmat.begin(), fmat.end(),
        [](const FitnessVector& lhs, const FitnessVector& rhs) noexcept
        {
            return lhs[0] < rhs[0];
        });

        return detail::find_indices(fmat, 
        [fmax = (*best)[0]](const FitnessVector& fvec) noexcept
        {
            return detail::floatIsEqual(fmax, fvec[0]);
        });
    }

    std::vector<size_t> findParetoFrontSort(const FitnessMatrix& fmat)
    {
        auto indices = detail::argsort(fmat.begin(), fmat.end(),
        [](const FitnessVector& lhs, const FitnessVector& rhs) noexcept
        {
            for (size_t i = 0; i < lhs.size(); i++)
            {
                if (lhs[i] != rhs[i]) return lhs[i] > rhs[i];
            }
            return false;
        });

        std::vector<size_t> optimal_indices;

        for (const auto& idx : indices)
        {
            bool dominated = std::any_of(optimal_indices.begin(), optimal_indices.end(),
            [&fmat, idx](size_t optimal_idx) noexcept
            {
                return detail::paretoCompareLess(fmat[idx], fmat[optimal_idx]);
            });
            if (!dominated) optimal_indices.push_back(idx);
        }

        return optimal_indices;
    }

    std::vector<size_t> findParetoFrontBest(const FitnessMatrix& fmat)
    {
        /* Implementation of the BEST algorithm based on the description in:
         * Godfrey et al. "Algorithms and analyses for maximal vector computation." The VLDB Journal 16, no. 1 (2007): 5-28. */

        if (fmat.empty()) return {};

        auto indices = detail::index_vector(fmat.size());

        std::vector<size_t> optimal_indices;
        optimal_indices.reserve(fmat.size());

        auto first = indices.begin();
        auto last  = indices.end();

        while (first != last)
        {
            auto best = first;
            for (auto it = std::next(first); it < last; ++it)
            {
                auto comp = detail::paretoCompare(fmat[*best], fmat[*it]);
                if (comp > 0)
                {
                    std::iter_swap(it--, --last);
                }
                else if (comp < 0)
                {
                    /* Replace and remove best from the index list (can't swap to the back, as that element hasn't been checked yet). */
                    std::iter_swap(best, first++);
                    best = it;
                }
            }
            optimal_indices.push_back(*best);   /* best is definitely optimal */

            /* best was only compared with the elements after it, there could be dominated elements before it still in the index list. */
            for (auto it = first; it < best; ++it)
            {
                if (detail::paretoCompareLess(fmat[*it], fmat[*best]))
                {
                    std::iter_swap(it, first++);
                }
            }
            std::iter_swap(best, --last);       /* best shouldn't be selected again. */

            /* None of the remaining indices in [first, last) are dominated by best, but they could be by another element in the list,
               so they can't be added to the optimal list yet. */
        }

        return optimal_indices;
    }

    bool kungCompareLess(const FitnessVector& lhs, const FitnessVector& rhs) noexcept
    {
        bool is_dominated = detail::paretoCompareLess(lhs, rhs, 1);

        bool is_equal = !detail::floatIsEqual(lhs[0], rhs[0]) &&
                        std::equal(lhs.begin() + 1, lhs.end(),
                                   rhs.begin() + 1,
                                   detail::floatIsEqual<double>);

        return is_dominated || is_equal;
    }

    std::vector<size_t> findParetoFrontKungImpl(const FitnessMatrix& fmat, std::vector<size_t>::const_iterator first, std::vector<size_t>::const_iterator last)
    {
        if (std::distance(first, last) == 1) return { *first };

        auto middle = first + std::distance(first, last) / 2;

        auto top_half    = _::findParetoFrontKungImpl(fmat, first, middle);
        auto bottom_half = _::findParetoFrontKungImpl(fmat, middle, last);

        for (const auto& bad : bottom_half)
        {
            bool is_dominated = false;
            for (const auto& good : top_half)
            {
                if (_::kungCompareLess(fmat[bad], fmat[good]))
                {
                    is_dominated = true;
                    break;
                }
            }
            if (!is_dominated) top_half.push_back(bad);
        }

        return top_half;
    }

    std::vector<size_t> findParetoFrontKung(const FitnessMatrix& fmat)
    {
        /* See: Kung et al. "On finding the maxima of a set of vectors." Journal of the ACM (JACM) 22.4 (1975): 469-476. */
        /* Doesn't work for d = 1 (single-objective optimization). */

        auto indices = detail::argsort(fmat.begin(), fmat.end(),
        [](const FitnessVector& lhs, const FitnessVector& rhs) noexcept
        {
            for (size_t i = 0; i < lhs.size(); i++)
            {
                if (lhs[i] != rhs[i]) return lhs[i] > rhs[i];
            }
            return false;
        });

        return _::findParetoFrontKungImpl(fmat, indices.cbegin(), indices.cend());
    }

} // namespace genetic_algorithm::detail::_