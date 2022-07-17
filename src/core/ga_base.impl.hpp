/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_CORE_GA_BASE_IMPL_HPP
#define GA_CORE_GA_BASE_IMPL_HPP

#include "ga_base.decl.hpp"
#include "../population/population.hpp"
#include "../selection/selection.hpp"
#include "../crossover/crossover_base.hpp"
#include "../crossover/lambda.hpp"
#include "../mutation/mutation_base.hpp"
#include "../mutation/lambda.hpp"
#include "../utility/rng.hpp"
#include "../utility/math.hpp"
#include "../utility/utility.hpp"
#include "../utility/algorithm.hpp"
#include <execution>
#include <numeric>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <cassert>
#include <cmath>

namespace genetic_algorithm
{
    template<Gene T, typename D>
    GA<T, D>::GA(size_t chrom_len, FitnessFunction f)
        : GA<T, D>(DEFAULT_POPSIZE, chrom_len, std::move(f))
    {}

    template<Gene T, typename D>
    GA<T, D>::GA(size_t population_size, size_t chrom_len, FitnessFunction f)
        : GaInfo(population_size, chrom_len)
    {
        if (!f) throw std::invalid_argument("The fitness function can't be a nullptr, it is requires for the GA.");

        fitness_function_ = std::move(f);
        /* Can't call findNumObjectives() here yet (and the setter can't be called either because of this),
         * because the derived constructor hasn't been called yet, and derived.generateCandidate (which is required for
         * calling findNumObjectives) might not return a valid candidate yet if it depends on the state of derived.
         * (The number of objecives should be determined initially by calling setDefaultAlgorithm() in the derived ctors,
         * and after that it's updated every time the fitness function is changed) */
    }

    template<Gene T, typename D>
    D& GA<T, D>::derived() noexcept
    {
        return static_cast<D&>(*this);
    }

    template<Gene T, typename D>
    const D& GA<T, D>::derived() const noexcept
    {
        return static_cast<const D&>(*this);
    }

    template<Gene T, typename D>
    void GA<T, D>::fitness_function(FitnessFunction f)
    {
        if (!f) throw std::invalid_argument("The fitness function can't be a nullptr, it is requires for the GA.");

        size_t old_objectives = num_objectives();
        fitness_function_ = std::move(f);
        num_objectives(findNumObjectives(fitness_function_));

        if (old_objectives != num_objectives())
        {
            /* The fitness vectors of the old solutions couldn't be compared to the new ones. */
            can_continue_ = false;
        }
    }

    template<Gene T, typename D>
    template<typename F>
    requires crossover::CrossoverMethod<F, T> && std::is_final_v<F>
    void GA<T, D>::crossover_method(F&& f)
    {
        crossover_ = std::make_unique<F>(std::forward<F>(f));
    }

    template<Gene T, typename D>
    template<crossover::CrossoverMethod<T> F>
    void GA<T, D>::crossover_method(std::unique_ptr<F>&& f)
    {
        if (!f) throw std::invalid_argument("The crossover method can't be a nullptr.");

        crossover_ = std::move(f);
    }

    template<Gene T, typename D>
    void GA<T, D>::crossover_method(CrossoverFunction f)
    {
        if (!f) throw std::invalid_argument("The crossover function can't be a nullptr.");

        crossover_ = std::make_unique<crossover::dtl::Lambda<T>>(std::move(f));
    }

    template<Gene T, typename D>
    template<crossover::CrossoverMethod<T> F>
    F& GA<T, D>::crossover_method() &
    {
        return dynamic_cast<F&>(*crossover_);
    }

    template<Gene T, typename D>
    void GA<T, D>::crossover_rate(double pc)
    {
        crossover_->crossover_rate(pc);
    }

    template<Gene T, typename D>
    double GA<T, D>::crossover_rate() const noexcept
    {
        return crossover_->crossover_rate();
    }

    template<Gene T, typename D>
    template<typename F>
    requires mutation::MutationMethod<F, T> && std::is_final_v<F>
    void GA<T, D>::mutation_method(F&& f)
    {
        mutation_ = std::make_unique<F>(std::forward<F>(f));
    }

    template<Gene T, typename D>
    template<mutation::MutationMethod<T> F>
    void GA<T, D>::mutation_method(std::unique_ptr<F>&& f)
    {
        if (!f) throw std::invalid_argument("The mutation method can't be a nullptr.");
        /* Nullptr could be allowed here to disable mutations, but the better way to disable them
         * would be to set the mutation_rate to 0 */

        mutation_ = std::move(f);
    }

    template<Gene T, typename D>
    void GA<T, D>::mutation_method(MutationFunction f)
    {
        if (!f) throw std::invalid_argument("Thhe mutation method can't be empty.");

        mutation_ = std::make_unique<mutation::dtl::Lambda<T>>(std::move(f));
    }

    template<Gene T, typename D>
    template<mutation::MutationMethod<T> F>
    F& GA<T, D>::mutation_method() &
    {
        return dynamic_cast<F&>(*mutation_);
    }

    template<Gene T, typename D>
    void GA<T, D>::mutation_rate(double pm)
    {
        mutation_->mutation_rate(pm);
    }

    template<Gene T, typename D>
    double GA<T, D>::mutation_rate() const noexcept
    {
        return mutation_->mutation_rate();
    }

    template<Gene T, typename D>
    void GA<T, D>::repair_function(const RepairFunction& f)
    {
        /* Nullptr is fine here, it just won't be called */
        repair_ = f;
    }

    template<Gene T, typename D>
    bool GA<T, D>::fitnessMatrixIsValid() const noexcept
    {
        if (fitness_matrix_.size() != population_.size() ||
            !std::all_of(fitness_matrix_.begin(), fitness_matrix_.end(), [this](const FitnessVector& fvec) { return fvec.size() == num_objectives_; }) ||
            !std::all_of(population_.begin(), population_.end(), [this](const Candidate& sol) { return sol.fitness.size() == num_objectives_; }))
        {
            return false;
        }
               
        for (size_t i = 0; i < fitness_matrix_.size(); i++)
        {
            for (size_t j = 0; j < fitness_matrix_[0].size(); j++)
            {
                if (fitness_matrix_[i][j] != population_[i].fitness[j]) return false;
            }
        }

        return true;
    }

    template<Gene T, typename D>
    size_t GA<T, D>::findNumObjectives(const FitnessFunction& f) const
    {
        Candidate dummy = generateCandidate();
        auto fvec = f(dummy.chromosome);
        return fvec.size();
    }

    template<Gene T, typename D>
    void GA<T, D>::setDefaultAlgorithm()
    {
        num_objectives(findNumObjectives(fitness_function_));

        (num_objectives_ == 1) ?
            selection_method(std::make_unique<selection::single_objective::Tournament>()) :
            selection_method(std::make_unique<selection::multi_objective::NSGA3>());
    }

    template<Gene T, typename D>
    void GA<T, D>::initializeAlgorithm()
    {
        /* The number of objectives is determined from the return value of the fitness func,
        *  this assumes that the returned vector will always be the same size during a run.
        *
        *  This also needs generateCandidate() to create a dummy solution to pass to the fitness
        *  function, which might only return valid values after the derived class contructor set
        *  up some stuff for the candidate generation (eg. bounds), so this can't be called earlier,
        *  eg. in the base ctor. */
        num_objectives(findNumObjectives(fitness_function_));

        /* Reset state variables just in case run() has already been called before. */
        can_continue_ = false;
        generation_cntr_ = 0;
        num_fitness_evals_.store(0, std::memory_order::relaxed);
        solutions_.clear();
        population_.clear();

        /* Create and evaluate the initial population of the algorithm. */
        population_ = generatePopulation(population_size_);
        fitness_matrix_ = evaluatePopulation(population_);

        /* Initialize the selection method.
         * This must be done after the initial population has been created and evaluted,
         * as it might want to use the population's fitness values (fitness_matrix_). */
        (*selection_).init(*this);
    }

    template<Gene T, typename D>
    auto GA<T, D>::generateCandidate() const -> Candidate
    {
        assert(chrom_len_ > 0);

        return derived().generateCandidate();
    }

    template<Gene T, typename D>
    auto GA<T, D>::generatePopulation(size_t pop_size) const -> Population
    {
        assert(pop_size);

        Population population;
        population.reserve(pop_size);

        while (pop_size--) population.push_back(generateCandidate());

        return population;
    }

    template<Gene T, typename D>
    void GA<T, D>::prepareSelections() const
    {
        assert(fitnessMatrixIsValid());

        (*selection_).prepare(*this, fitness_matrix());
    }

    template<Gene T, typename D>
    auto GA<T, D>::select() const -> const Candidate&
    {
        size_t idx = (*selection_).select(*this, fitness_matrix());

        return population_[idx];
    }

    template<Gene T, typename D>
    auto GA<T, D>::crossover(const Candidate& parent1, const Candidate& parent2) const -> CandidatePair
    {
        return (*crossover_)(*this, parent1, parent2);
    }

    template<Gene T, typename D>
    void GA<T, D>::mutate(Candidate& sol) const
    {
        return (*mutation_)(*this, sol);
    }

    template<Gene T, typename D>
    void GA<T, D>::repair(Candidate& sol) const
    {
        /* Don't try to do anything unless a repair function is set. */
        if (!repair_) return;

        auto improved_chrom = repair_(sol.chromosome);

        if (improved_chrom != sol.chromosome)
        {
            sol.is_evaluated = false;
            sol.chromosome = std::move(improved_chrom);
        }
    }

    template<Gene T, typename D>
    void GA<T, D>::updatePopulation(Population& current_pop, Population&& children)
    {
        assert(fitnessMatrixIsValid());

        /* The current pop has already been evaluted in the previous generation */
        auto child_fmat = evaluatePopulation(children);

        std::move(children.begin(), children.end(), std::back_inserter(current_pop));
        std::move(child_fmat.begin(), child_fmat.end(), std::back_inserter(fitness_matrix_));
        
        assert(fitnessMatrixIsValid());

        auto next_indices = (*selection_).nextPopulation(*this, fitness_matrix_);
        // TODO: maybe just remove the bad ones instead of creating new vectors
        current_pop = detail::map(next_indices, [&current_pop](size_t idx) { return current_pop[idx]; });
        fitness_matrix_ = detail::map(next_indices, [this](size_t idx) { return fitness_matrix_[idx]; });

        assert(fitnessMatrixIsValid());
    }

    template<Gene T, typename D>
    bool GA<T, D>::stopCondition() const
    {
        return (*stop_condition_)(*this);
    }

    template<Gene T, typename D>
    void GA<T, D>::evaluateSolution(Candidate& sol)
    {
        /* If the fitness function is static, and the solution has already
         * been evaluted sometime earlier (in an earlier generation), there
         * is no point doing it again. */
        if (!sol.is_evaluated || dynamic_fitness)
        {
            sol.fitness = fitness_function_(sol.chromosome);
            sol.is_evaluated = true;

            num_fitness_evals_.fetch_add(1, std::memory_order::relaxed);

            assert(sol.fitness.size() == num_objectives_);
            assert(std::all_of(sol.fitness.begin(), sol.fitness.end(), std::isfinite<double>));
        }
    }

    template<Gene T, typename D>
    auto GA<T, D>::evaluatePopulation(Population& pop) -> FitnessMatrix
    {
        assert(fitness_function_);

        std::for_each(GA_EXECUTION_UNSEQ, pop.begin(), pop.end(), [this](Candidate& sol) { evaluateSolution(sol); });

        return detail::toFitnessMatrix(pop);
    }

    template<Gene T, typename D>
    void GA<T, D>::updateOptimalSolutions(Candidates& optimal_sols, const Population& pop) const
    {
        assert(std::all_of(pop.begin(), pop.end(), [](const Candidate& sol) { return sol.is_evaluated; }));

        // TODO: theres probably a better way to do this
        optimal_sols.insert(optimal_sols.end(), pop.begin(), pop.end());
        optimal_sols = detail::findParetoFront(optimal_sols);
        detail::erase_duplicates(optimal_sols);
    }

    template<Gene T, typename D>
    void GA<T, D>::advance()
    {
        if (keep_all_optimal_solutions) updateOptimalSolutions(solutions_, population_);

        size_t num_children = population_size_ + population_size_ % 2;
        std::vector<CandidatePair> child_pairs(num_children / 2);

        prepareSelections();
        std::generate(GA_EXECUTION_UNSEQ, child_pairs.begin(), child_pairs.end(),
        [this]
        {
            const auto& parent1 = select();
            const auto& parent2 = select();

            return crossover(parent1, parent2);
        });

        auto children = detail::flatten(std::move(child_pairs));

        std::for_each(GA_EXECUTION_UNSEQ, children.begin(), children.end(),
        [this](Candidate& child)
        {
            mutate(child);
            repair(child);
        });

        updatePopulation(population_, std::move(children));

        if (endOfGenerationCallback) endOfGenerationCallback(*this);
        generation_cntr_++;
    }

    template<Gene T, typename D>
    auto GA<T, D>::run(size_t num_generations) -> Candidates
    {
        max_gen(num_generations);

        initializeAlgorithm();
        while (!stopCondition())
        {
            advance();
        }
        updateOptimalSolutions(solutions_, population_);

        if (endOfRunCallback) endOfRunCallback(*this);
        can_continue_ = true;

        return solutions_;
    }

    template<Gene T, typename D>
    auto GA<T, D>::continueFor(size_t num_generations) -> Candidates
    {
        max_gen(max_gen_ + num_generations);

        if (!can_continue_) initializeAlgorithm();
        while (!stopCondition())
        {
            advance();
        }
        updateOptimalSolutions(solutions_, population_);

        if (endOfRunCallback) endOfRunCallback(*this);
        can_continue_ = true;

        return solutions_;
    }

} // namespace genetic_algorithm

#endif // !GA_CORE_GA_BASE_IMPL_HPP