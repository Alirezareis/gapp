/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "integer.hpp"
#include "crossover_dtl.hpp"
#include "../utility/rng.hpp"
#include <utility>
#include <stdexcept>

namespace genetic_algorithm::crossover::integer
{
    auto SinglePoint::crossover(const GaInfo&, const Candidate<GeneType>& parent1, const Candidate<GeneType>& parent2) const -> CandidatePair<GeneType>
    {
        return dtl::singlePointCrossoverImpl(parent1, parent2);
    }

    auto TwoPoint::crossover(const GaInfo&, const Candidate<GeneType>& parent1, const Candidate<GeneType>& parent2) const -> CandidatePair<GeneType>
    {
        return dtl::twoPointCrossoverImpl(parent1, parent2);
    }

    NPoint::NPoint(size_t n) :
        Crossover()
    {
        num_crossover_points(n);
    }

    NPoint::NPoint(Probability pc, size_t n) :
        Crossover(pc)
    {
        num_crossover_points(n);
    }

    void NPoint::num_crossover_points(size_t n)
    {
        if (n == 0) GA_THROW(std::invalid_argument, "The number of crossover points must be at least 1 for the n-point crossover.");

        n_ = n;
    }

    auto NPoint::crossover(const GaInfo&, const Candidate<GeneType>& parent1, const Candidate<GeneType>& parent2) const -> CandidatePair<GeneType>
    {
        if (n_ == 1)
            return dtl::singlePointCrossoverImpl(parent1, parent2);
        else if (n_ == 2)
            return dtl::twoPointCrossoverImpl(parent1, parent2);
        else
            return dtl::nPointCrossoverImpl(parent1, parent2, n_);
    }

    Uniform::Uniform(Probability pc, Probability swap_prob) noexcept :
        Crossover(pc), ps_(swap_prob)
    {
    }

    void Uniform::swap_probability(Probability ps)
    {
        ps_ = ps;
    }

    auto Uniform::crossover(const GaInfo&, const Candidate<GeneType>& parent1, const Candidate<GeneType>& parent2) const -> CandidatePair<GeneType>
    {
        if (parent1.chromosome.size() != parent2.chromosome.size())
        {
            GA_THROW(std::invalid_argument, "The parent chromosomes must be the same length for the uniform crossover.");
        }
        
        const size_t chrom_len = parent1.chromosome.size();
        const size_t num_swapped = rng::randomBinomial(chrom_len, ps_);
        const auto swapped_indices = rng::sampleUnique(0_sz, chrom_len, num_swapped);

        Candidate child1 = parent1;
        Candidate child2 = parent2;

        for (const auto& idx : swapped_indices)
        {
            using std::swap;
            swap(child1.chromosome[idx], child2.chromosome[idx]);
        }

        return { std::move(child1), std::move(child2) };
    }

} // namespace genetic_algorithm::crossover::integer