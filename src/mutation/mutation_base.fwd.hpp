/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_MUTATION_BASE_FWD_HPP
#define GA_MUTATION_BASE_FWD_HPP

#include "../population/candidate.hpp"
#include <concepts>

namespace genetic_algorithm::mutation
{
    template<Gene T>
    class Mutation;

    template<typename T, typename G>
    concept MutationMethod = requires
    {
        requires Gene<G>;
        requires std::derived_from<T, Mutation<G>>;
    };

} // namespace genetic_algorithm::mutation

#endif // !GA_MUTATION_BASE_FWD_HPP