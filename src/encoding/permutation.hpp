/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_ENCODING_PERMUTATION_HPP
#define GA_ENCODING_PERMUTATION_HPP

#include "../core/ga_base.decl.hpp"
#include "../population/candidate.hpp"
#include "gene_types.hpp"
#include <memory>
#include <utility>
#include <cstddef>

namespace genetic_algorithm
{
    /**
    * Genetic algorithm where the chromosomes encode permutations. \n
    * The genes of the chromosomes are all unique unsigned integers on thhe closed interval [0, chrom_len - 1].
    */
    class PermutationGA : public GA<PermutationGene>
    {
    public:
        /**
        * Construct a permutation encoded genetic algorithm.
        *
        * @param fitness_function The fitness function used in the algorithm.
        * @param population_size The number of candidates in the population.
        */
        explicit PermutationGA(std::unique_ptr<FitnessFunction<PermutationGene>> fitness_function, size_t population_size = DEFAULT_POPSIZE);

        /**
        * Construct a permutation encoded genetic algorithm.
        *
        * @param fitness_function The fitness function used in the algorithm.
        * @param population_size The number of candidates in the population.
        */
        template<typename F>
        requires FitnessFunctionType<F, PermutationGene> && std::is_final_v<F>
        explicit PermutationGA(F fitness_function, size_t population_size = DEFAULT_POPSIZE) :
            GA(std::make_unique<F>(std::move(fitness_function)), population_size)
        {}

    private:
        void initialize() override;
        Candidate<GeneType> generateCandidate() const override;
    };

} // namespace genetic_algorithm

#endif // !GA_ENCODING_PERMUTATION_HPP