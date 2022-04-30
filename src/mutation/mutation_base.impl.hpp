/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_MUTATION_BASE_IMPL_HPP
#define GA_MUTATION_BASE_IMPL_HPP

#include "mutation_base.decl.hpp"
#include "../algorithms/ga_base.hpp"
#include <algorithm>
#include <stdexcept>

namespace genetic_algorithm::mutation
{
    template<Gene T>
    Mutation<T>::Mutation(double pm)
    {
        mutation_rate(pm);
    }

    template<Gene T>
    void Mutation<T>::mutation_rate(double pm)
    {
        if (!(0.0 <= pm && pm <= 1.0))
        {
            throw std::invalid_argument("The mutation probability must be in the closed range [0.0, 1.0]");
        }

        pm_ = pm;
    }

    template<Gene T>
    void Mutation<T>::operator()(const GaInfo& ga, Candidate<T>& candidate) const
    {
        auto old_candidate = candidate;

        mutate(ga, candidate);
        candidate.is_evaluated = false;

        if (candidate.chromosome == old_candidate.chromosome)
        {
            candidate.fitness = std::move(old_candidate.fitness);
            candidate.is_evaluated = old_candidate.is_evaluated;
        }
    }

} // namespace genetic_algorithm::mutation

#endif // !GA_MUTATION_BASE_IMPL_HPP