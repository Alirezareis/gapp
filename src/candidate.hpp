/*
*  MIT License
*
*  Copyright (c) 2021 Kriszti�n Rug�si
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

/**
* This file contains the Candidate class used in the algorithms.
*
* @file candidate.h
*/

#ifndef GA_CANDIDATE_H
#define GA_CANDIDATE_H

#include "math.hpp"
#include "concepts.hpp"

#include <vector>
#include <cstddef>

namespace genetic_algorithm
{
    /**
    * The Candidate class that is used to represent solutions in the genetic algorithms.
    * This is used as the general purpose candidate type in all of the algorithms (SOGA, NSGA-II, NSGA-III).
    */
    template<gene T>
    struct Candidate
    {
        using Gene = T;

        std::vector<Gene> chromosome;       /**< The chromosome encoding the solution. */
        std::vector<double> fitness;        /**< The fitness values (for each objective) of the solution. */

        bool is_evaluated = false;          /**< False if the candidate's fitness value needs to be computed. */

        Candidate() = default;
        explicit Candidate(const std::vector<Gene>& chrom) : chromosome(chrom) {}
    };

    /** A pair of candidates. */
    template<gene T>
    using CandidatePair = std::pair<Candidate<T>, Candidate<T>>;

    /** Two candidates are considered equal if their chromosomes are the same. */
    template<typename T>
    inline bool operator==(const Candidate<T>& lhs, const Candidate<T>& rhs);

    /** Two candidates are considered not equal if their chromosomes are different. */
    template<typename T>
    inline bool operator!=(const Candidate<T>& lhs, const Candidate<T>& rhs);

    /** Hash function for the Candidate so they can be stored in an unordered set/map. */
    template<detail::hashable T>
    struct CandidateHasher
    {
        size_t operator()(const Candidate<T>& candidate) const noexcept;
    };

} // namespace genetic_algorithm


/* IMPLEMENTATION */

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace genetic_algorithm
{
    template<typename GeneType>
    inline bool operator==(const Candidate<GeneType>& lhs, const Candidate<GeneType>& rhs)
    {
        if constexpr (std::is_floating_point_v<GeneType>)
        {
            return std::equal(lhs.chromosome.begin(), lhs.chromosome.end(), rhs.chromosome.begin(),
            [](const GeneType& lhs, const GeneType& rhs)
            {
                return detail::floatIsEqual(lhs, rhs);
            });
        }
        else
        {
            return lhs.chromosome == rhs.chromosome;
        }
    }

    template<typename GeneType>
    inline bool operator!=(const Candidate<GeneType>& lhs, const Candidate<GeneType>& rhs)
    {
        return !(lhs == rhs);
    }

    template<detail::hashable GeneType>
    inline size_t CandidateHasher<GeneType>::operator()(const Candidate<GeneType>& candidate) const noexcept
    {
        size_t seed = candidate.chromosome.size();
        for (const auto& gene : candidate.chromosome)
        {
            seed ^= std::hash<GeneType>()(gene) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

} // namespace genetic_algorithm

#endif // !GA_CANDIDATE_H