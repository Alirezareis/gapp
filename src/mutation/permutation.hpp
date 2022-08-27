/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_MUTATION_PERMUTATION_HPP
#define GA_MUTATION_PERMUTATION_HPP

#include "mutation_base.hpp"
#include "../encoding/gene_types.hpp"

/** Predefined mutation operators for the permutation encoded genetic algorithm. */
namespace genetic_algorithm::mutation::perm
{
    /**
    * Inversion mutation operator for the permutation genetic algorithm. \n
    * Each individual is mutated with the specified mutation probability.
    * In the mutated individuals, a randomly selected range of genes are reversed.
    */
    class Inversion : public Mutation<PermutationGene>
    {
    public:
        /**
        * Create an inversion mutation operator.
        * 
        * @param pm The mutation probability. Must be in the closed interval [0.0, 1.0].
        * @param range_max The value of the range max parameter. Must be in the closed interval [0.0, 1.0].
        */
        explicit Inversion(Probability pm, double range_max = 0.75);

        /**
        * Set the value of the maximum range length parameter for the operator. \n
        * The parameter determines the maximum length of the reversed range of genes during mutation:
        * the maximum length is @p rm * chrom_len. \n
        * The value of this parameter must be in the closed interval [0.0, 1.0].
        * 
        * @param rm The value of the range max parameter.
        */
        void range_max(double rm);

        /** @ returns The value of the range_max parameter. */
        [[nodiscard]]
        double range_max() const noexcept { return range_max_; }

    private:
        void mutate(const GaInfo& ga, Candidate<GeneType>& candidate) const override;

        double range_max_;
    };

    /**
    * Single swap/swap2 mutation operator for the permutation genetic algorithm. \n
    * Each individual is mutated with the set mutation probability.
    * Two randomly selected genes are swapped in the mutated individuals.
    */
    class Swap2 final : public Mutation<PermutationGene>
    {
    public:
        using Mutation::Mutation;
    private:
        void mutate(const GaInfo& ga, Candidate<GeneType>& candidate) const override;
    };

    /**
    * Swap-3 mutation operator for the permutation genetic algorithm. \n
    * Each individual is mutated with the set mutation probability.
    * In the mutated candidates, 3 randomly selected genes are reordered as: (a-b-c) -> (c-a-b).
    */
    class Swap3 final : public Mutation<PermutationGene>
    {
    public:
        using Mutation::Mutation;
    private:
        void mutate(const GaInfo& ga, Candidate<GeneType>& candidate) const override;
    };

    /**
    * Shuffle/scramble mutation operator for the permutation genetic algorithm. \n
    * Each individual is mutated with the set mutation probability.
    * In the mutated candidates, a random range of genes is selected and then shuffled.
    */
    class Shuffle final : public Mutation<PermutationGene>
    {
    public:
        /**
        * Create a shuffle mutation operator.
        *
        * @param pm The mutation probability. Must be in the closed interval [0.0, 1.0].
        * @param range_max The value of the range_max parameter. Must be in the closed interval [0.0, 1.0].
        */
        explicit Shuffle(Probability pm, double range_max = 0.5);

        /**
        * Set the value of the maximum range length parameter for the operator. \n
        * The parameter determines the maximum length of the shuffled range of genes during mutation:
        * the maximum length is @p rm * chrom_len. \n
        * The value of this parameter must be in the closed interval [0.0, 1.0].
        *
        * @param rm The value of the range max parameter.
        */
        void range_max(double rm);

        /** @ returns The value of the range_max parameter. */
        [[nodiscard]]
        double range_max() const noexcept { return range_max_; }

    private:
        void mutate(const GaInfo& ga, Candidate<GeneType>& candidate) const override;

        double range_max_;
    };

    /**
    * Shift/slide mutation operator for the permutation encoded genetic algorithm. \n
    * Each individual is mutated with the set mutation proability.
    * In the mutated candidates, a random range of genes is selected and then moved to a
    * different position in the chromosome.
    */
    class Shift final : public Mutation<PermutationGene>
    {
    public:
        /**
        * Create a shift mutation operator.
        *
        * @param pm The mutation probability. Must be in the closed interval [0.0, 1.0].
        * @param range_max The value of the range_max parameter. Must be in the closed interval [0.0, 1.0].
        */
        explicit Shift(Probability pm, double range_max = 0.75);

        /**
        * Set the value of the maximum range length parameter for the operator. \n
        * The parameter determines the maximum length of the shifted range of genes during mutation:
        * the maximum length is @p rm * chrom_len. \n
        * The value of this parameter must be in the closed interval [0.0, 1.0].
        *
        * @param rm The value of the range max parameter.
        */
        void range_max(double rm);

        /** @ returns The value of the range_max parameter. */
        [[nodiscard]]
        double range_max() const noexcept { return range_max_; }

    private:
        void mutate(const GaInfo& ga, Candidate<GeneType>& candidate) const override;

        double range_max_;
    };

} // namespace genetic_algorithm::mutation::perm

#endif // !GA_MUTATION_PERMUTATION_HPP