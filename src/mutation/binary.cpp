/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "binary.hpp"
#include "../utility/rng.hpp"
#include "../utility/utility.hpp"
#include <cstddef>

namespace genetic_algorithm::mutation::binary
{
    void Flip::mutate(const GaInfo&, Candidate<GeneType>& candidate) const
    {
        size_t flip_count = rng::randomBinomialApprox(candidate.chromosome.size(), pm_);
        auto flipped_indices = rng::sampleUnique(0_sz, candidate.chromosome.size(), flip_count);
        // TODO try to improve this, maybe masks
        for (const auto& idx : flipped_indices)
        {
            candidate.chromosome[idx] = !bool(candidate.chromosome[idx]);
        }
    }

} // namespace genetic_algorithm::mutation::binary