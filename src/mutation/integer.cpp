/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "integer.hpp"
#include "../encoding/integer.hpp"
#include "../utility/rng.hpp"
#include "../utility/utility.hpp"
#include <algorithm>
#include <vector>

namespace genetic_algorithm::mutation::integer
{
    void Uniform::mutate(const GA<GeneType>& ga, Candidate<GeneType>& candidate) const
    {
        const auto& bounds = ga.gene_bounds();
        const size_t chrom_len = candidate.chromosome.size();

        const size_t mutate_count = rng::randomBinomial(chrom_len, mutation_rate());
        const auto mutated_indices = rng::sampleUnique(0_sz, chrom_len, mutate_count);

        for (const auto& idx : mutated_indices)
        {
            GeneType new_gene = rng::randomInt(bounds[idx].lower, bounds[idx].upper);

            while (new_gene == candidate.chromosome[idx])
            {
                new_gene = rng::randomInt(bounds[idx].lower, bounds[idx].upper);
            }

            candidate.chromosome[idx] = new_gene;
        }
    }

} // namespace genetic_algorithm::mutation::integer