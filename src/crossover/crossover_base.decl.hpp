/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_CROSSOVER_BASE_DECL_HPP
#define GA_CROSSOVER_BASE_DECL_HPP

#include "../population/candidate.hpp"
#include <vector>
#include <utility>

namespace genetic_algorithm
{
    class GaInfo;
}

/** Crossover operators used in the algorithms. */
namespace genetic_algorithm::crossover
{
    /**
    * Base class used for all of the crossover operators.
    * Every implemented crossover operator takes 2 Candidates (the parents), and creates 2 children from these parents.
    * The crossover operation is performed on the 2 parents with the set (pc) probability only, the rest of the time
    * the returned children will be the same as the parents.
    */
    template<Gene T>
    class Crossover
    {
    public:
        using GeneType = T;

        /**
        * Create a crossover operator that will use @p pc as the crossover rate.
        *
        * @param pc The crossover probability. Must be in the closed interval [0.0, 1.0].
        */
        explicit Crossover(double pc = 0.8);

        Crossover(const Crossover&)             = default;
        Crossover(Crossover&&)                  = default;
        Crossover& operator=(const Crossover&)  = default;
        Crossover& operator=(Crossover&&)       = default;
        virtual ~Crossover()                    = default;

        /**
        * Sets the crossover rate used in the algorithm to @p pc. 
        *
        * @param pc The crossover probability. Must be in the closed interval [0.0, 1.0].
        */
        void crossover_rate(double pc);

        /** @returns The crossover rate currently set for this crossover operator. */
        [[nodiscard]]
        double crossover_rate() const noexcept { return pc_; }

        /**
        * Perform the crossover operation on 2 individuals with the set probability.
        *
        * @param ga The genetic algorithm the crossover operator is being used in.
        * @param parent1 The first parent.
        * @param parent2 The second parent.
        * @returns The pair of children resulting from the crossover.
        */
        CandidatePair<T> operator()(const GaInfo& ga, const Candidate<T>& parent1, const Candidate<T>& parent2) const;

    protected:

        /* The actual crossover function. Performs the crossover every time and does nothing else. */
        virtual CandidatePair<T> crossover(const GaInfo& ga, const Candidate<T>& parent1, const Candidate<T>& parent2) const = 0;

        double pc_ = 0.8;   /* The probability of performing the crossover operation on the parents. */
    };

} // namespace genetic_algorithm::crossover

#endif // !GA_CROSSOVER_BASE_DECL_HPP