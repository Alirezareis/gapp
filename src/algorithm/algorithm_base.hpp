/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_ALGORITHM_ALGORITHM_BASE_HPP
#define GA_ALGORITHM_ALGORITHM_BASE_HPP

#include "../population/population.hpp"
#include <vector>
#include <cstddef>
#include <concepts>
#include <optional>

namespace genetic_algorithm
{
    class GaInfo;
}

/** Algorithm types that can be used in the genetic algorithms (contains both single- and multi-objective algorithms). */
namespace genetic_algorithm::algorithm
{
    /**
    * Base class used for all of the algorithms. \n
    * 
    * The algorithms define the way the population is evolved over the generations (the selection and
    * population update methods used). They may either be single- or multi-objective (or both), and have 4
    * methods which must be overriden in the derived classes: \n
    * 
    *  - initialize:         Initializes the algorithm (at the start of a run). \n
    *  - prepareSelections:  Prepares the algorithm for the selections if needed. \n
    *  - select:             Selects a candidate from the population (this should be thread-safe). \n
    *  - nextPopulation:     Select the candidates of the next population from the parents and the children.
    */
    class Algorithm
    {
    public:
        using FitnessVector = detail::FitnessVector;
        using FitnessMatrix = detail::FitnessMatrix;

        /**
        * Initialize the algorithm method if needed. \n
        * This method will be called exactly once at start of the genetic algorithm,
        * after the initial population has already been created. \n
        *
        * @param ga The GA that uses the algorithm.
        */
        void initialize(const GaInfo& ga) { initializeImpl(ga); };

        /**
        * Prepare the algorithm for the selections beforehand if neccesary. \n
        * This method will be called exactly once every generation right before the selections are performed. \n
        *
        * @param ga The GA that uses the algorithm.
        * @param fmat The fitness matrix of the population of the GA.
        */
        void prepareSelections(const GaInfo& ga, const FitnessMatrix& fmat) { prepareSelectionsImpl(ga, fmat); }

        /**
        * Select a single candidate from the population. \n
        * This method will be called exactly (population_size) or (population_size + 1)
        * times in every generation (if the population_size is even or odd respectively). \n
        * The implementation should be thread-safe if parallel execution is enabled for the GAs
        * (enabled by default).
        *
        * @param ga The GA that uses the algorithm.
        * @param fmat The fitness matrix of the current population.
        * @returns The selected candidate's index in the fitness matrix.
        */
        size_t select(const GaInfo& ga, const FitnessMatrix& fmat) const
        {
            size_t selected_idx = selectImpl(ga, fmat);

            if (selected_idx >= ga.population_size())
            {
                GA_THROW(std::logic_error, "An invalid candidate was selected by the algorithm.");
            }

            return selected_idx;
        }

        /**
        * Select the Candidates of the next generation (next population) from the Candidates of the
        * current population and the child population that was generated from the current population. \n
        * This method will be called exactly once at the end of each generation. \n
        * 
        * The fitness matrix is given as the range [first, last), where
        * the subrange [first, children_first) belongs to the parents, and
        * the subrange [children_first, last) belongs to the children.
        *
        * @param ga The GA that uses the algorithm.
        * @param first The first element of the fitness matrix (first parent).
        * @param children_first The first element of the fitness matrix that belongs to a child.
        * @param last The end of the fitness matrix.
        * @returns The selected candidates' indices in the fitness matrix, assuming that the index of @p first is 0.
        */
        std::vector<size_t> nextPopulation(const GaInfo& ga,
                                           FitnessMatrix::const_iterator first,
                                           FitnessMatrix::const_iterator children_first,
                                           FitnessMatrix::const_iterator last)
        {
            assert(size_t(children_first - first) == ga.population_size());
            assert(size_t(last - children_first) >= ga.population_size());
            assert(std::all_of(first, last, [&](const FitnessVector& f) { return f.size() == first->size(); }));

            auto next_indices = nextPopulationImpl(ga, first, children_first, last);

            if (std::any_of(next_indices.begin(), next_indices.end(), detail::greater_eq_than(ga.population_size())))
            {
                GA_THROW(std::logic_error, "An invalid candidate was selected for the next population by the algorithm.");
            }

            return next_indices;
        }

        /**
        * Return the indices of the optimal solutions in a population that was created
        * using the indices returned by nextPopulation(), assuming the order of the candidates
        * in the population didn't change. \n
        * Returns unique indices in the range [0, ga.population_size) \n
        * 
        * This function doesn't have to be implemented in the derived algorithms, but it can be
        * useful if finding the optimal solutions can be implemented more efficiently than the default
        * algorithm used by the GAs ( calling detail::findParetoFront(ga.population()) ).
        * 
        * @param ga The GA that uses the algorithm.
        * @returns The indices of the optimal candidates.
        */
        std::optional<std::vector<size_t>> optimalSolutions(const GaInfo& ga) const { return optimalSolutionsImpl(ga); }


        Algorithm()                             = default;
        Algorithm(const Algorithm&)             = default;
        Algorithm(Algorithm&&)                  = default;
        Algorithm& operator=(const Algorithm&)  = default;
        Algorithm& operator=(Algorithm&&)       = default;
        virtual ~Algorithm()                    = default;

    private:

        /** Implementation of the initialize function. */
        virtual void initializeImpl(const GaInfo& ga) = 0;

        /** Implementation of the prepareSelections function. */
        virtual void prepareSelectionsImpl(const GaInfo& ga, const FitnessMatrix& fmat) = 0;

        /** Implementation of the select function. */
        virtual size_t selectImpl(const GaInfo& ga, const FitnessMatrix& fmat) const = 0;

        /** Implementation of the nextPopulation function. */
        virtual std::vector<size_t> nextPopulationImpl(const GaInfo& ga,
                                                       FitnessMatrix::const_iterator first,
                                                       FitnessMatrix::const_iterator children_first,
                                                       FitnessMatrix::const_iterator last) = 0;

        /** Implementation of the optimalSolutions function. */
        virtual std::optional<std::vector<size_t>> optimalSolutionsImpl(const GaInfo&) const { return {}; }
    };

    /** Algorithm types. */
    template<typename T>
    concept AlgorithmType = requires
    {
        requires std::derived_from<T, Algorithm>;
    };

} // namespace genetic_algorithm::algorithm

#endif // !GA_ALGORITHM_ALGORITHM_BASE_HPP