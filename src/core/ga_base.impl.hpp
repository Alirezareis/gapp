﻿/* Copyright (c) 2022 Krisztián Rugási. Subject to the MIT License. */

#ifndef GAPP_CORE_GA_BASE_IMPL_HPP
#define GAPP_CORE_GA_BASE_IMPL_HPP

#include "ga_base.decl.hpp"
#include "ga_traits.hpp"
#include "population.hpp"
#include "fitness_function.hpp"
#include "../algorithm/algorithm_base.hpp"
#include "../algorithm/single_objective.hpp"
#include "../algorithm/nsga3.hpp"
#include "../crossover/crossover_base.hpp"
#include "../crossover/lambda.hpp"
#include "../mutation/mutation_base.hpp"
#include "../mutation/lambda.hpp"
#include "../stop_condition/stop_condition_base.hpp"
#include "../utility/algorithm.hpp"
#include "../utility/functional.hpp"
#include "../utility/thread_pool.hpp"
#include "../utility/scope_exit.hpp"
#include "../utility/utility.hpp"
#include <algorithm>
#include <functional>
#include <numeric>
#include <type_traits>
#include <memory>
#include <atomic>
#include <utility>

namespace gapp
{
    template<typename T>
    GA<T>::GA(Positive<size_t> population_size,
              std::unique_ptr<typename algorithm::Algorithm> algorithm,
              std::unique_ptr<typename crossover::Crossover<T>> crossover,
              std::unique_ptr<typename mutation::Mutation<T>> mutation,
              std::unique_ptr<typename stopping::StopCondition> stop_condition) :
        GaInfo(population_size, std::move(algorithm), std::move(stop_condition)), crossover_(std::move(crossover)), mutation_(std::move(mutation))
    {
        GAPP_ASSERT(crossover_, "The crossover method can't be a nullptr.");
        GAPP_ASSERT(mutation_, "The mutation method can't be a nullptr.");
    }

    template<typename T>
    GA<T>::GA(Positive<size_t> population_size) :
        GA(population_size, std::make_unique<algorithm::SingleObjective>(), std::make_unique<typename GaTraits<T>::DefaultCrossover>(), std::make_unique<typename GaTraits<T>::DefaultMutation>(0.01))
    {
        use_default_algorithm_ = true;
        use_default_mutation_rate_ = true;
    }

    template<typename T>
    GA<T>::GA(Positive<size_t> population_size, std::unique_ptr<typename algorithm::Algorithm> algorithm) :
        GA(population_size, std::move(algorithm), std::make_unique<typename GaTraits<T>::DefaultCrossover>(), std::make_unique<typename GaTraits<T>::DefaultMutation>(0.01))
    {
        use_default_algorithm_ = false;
        use_default_mutation_rate_ = true;
    }

    template<typename T>
    GA<T>::GA(Positive<size_t> population_size,
              std::unique_ptr<typename crossover::Crossover<T>> crossover,
              std::unique_ptr<typename mutation::Mutation<T>> mutation,
              std::unique_ptr<typename stopping::StopCondition> stop_condition) :
        GA(population_size, std::make_unique<algorithm::SingleObjective>(), std::move(crossover), std::move(mutation), std::move(stop_condition))
    {
        use_default_algorithm_ = true;
        use_default_mutation_rate_ = false;
    }

    template<typename T>
    template<typename AlgorithmType>
    requires std::derived_from<AlgorithmType, algorithm::Algorithm>
    GA<T>::GA(Positive<size_t> population_size, AlgorithmType algorithm) :
        GA(population_size, std::make_unique<AlgorithmType>(std::move(algorithm)))
    {}

    template<typename T>
    template<typename CrossoverType, typename MutationType, typename StoppingType>
    requires std::derived_from<CrossoverType, crossover::Crossover<T>> &&
             std::derived_from<MutationType, mutation::Mutation<T>> &&
             std::derived_from<StoppingType, stopping::StopCondition>
    GA<T>::GA(Positive<size_t> population_size, CrossoverType crossover, MutationType mutation, StoppingType stop_condition) :
        GA(population_size,
           std::make_unique<algorithm::SingleObjective>(),
           std::make_unique<CrossoverType>(std::move(crossover)),
           std::make_unique<MutationType>(std::move(mutation)),
           std::make_unique<StoppingType>(std::move(stop_condition)))
    {}

    template<typename T>
    template<typename AlgorithmType, typename CrossoverType, typename MutationType, typename StoppingType>
    requires std::derived_from<AlgorithmType, algorithm::Algorithm> &&
             std::derived_from<CrossoverType, crossover::Crossover<T>> &&
             std::derived_from<MutationType, mutation::Mutation<T>> &&
             std::derived_from<StoppingType, stopping::StopCondition>
    GA<T>::GA(Positive<size_t> population_size, AlgorithmType algorithm, CrossoverType crossover, MutationType mutation, StoppingType stop_condition) :
        GA(population_size,
           std::make_unique<AlgorithmType>(std::move(algorithm)),
           std::make_unique<CrossoverType>(std::move(crossover)),
           std::make_unique<MutationType>(std::move(mutation)),
           std::make_unique<StoppingType>(std::move(stop_condition)))
    {}

    
    template<typename T>
    inline const FitnessFunctionBase<T>* GA<T>::fitness_function() const& noexcept
    {
        return fitness_function_.get();
    }

    template<typename T>
    inline size_t GA<T>::chrom_len() const noexcept
    {
        return fitness_function() ? fitness_function()->chrom_len() : 0_sz;
    }


    template<typename T>
    inline const BoundsVector<T>& GA<T>::gene_bounds() const noexcept requires(is_bounded<T>)
    {
        return bounds_;
    }


    template<typename T>
    template<typename F>
    requires std::derived_from<F, crossover::Crossover<T>>
    inline void GA<T>::crossover_method(F f)
    {
        crossover_ = std::make_unique<F>(std::move(f));
    }

    template<typename T>
    inline void GA<T>::crossover_method(std::unique_ptr<typename crossover::Crossover<T>> f)
    {
        GAPP_ASSERT(f, "The crossover method can't be a nullptr.");

        crossover_ = std::move(f);
    }

    template<typename T>
    inline void GA<T>::crossover_method(CrossoverCallable f)
    {
        crossover_ = std::make_unique<crossover::Lambda<T>>(std::move(f));
    }

    template<typename T>
    inline const crossover::Crossover<T>& GA<T>::crossover_method() const& noexcept
    {
        GAPP_ASSERT(crossover_);

        return *crossover_;
    }

    template<typename T>
    inline void GA<T>::crossover_rate(Probability pc) noexcept
    {
        GAPP_ASSERT(crossover_);

        crossover_->crossover_rate(pc);
    }

    template<typename T>
    inline Probability GA<T>::crossover_rate() const noexcept
    {
        GAPP_ASSERT(crossover_);

        return crossover_->crossover_rate();
    }

    template<typename T>
    template<typename F>
    requires std::derived_from<F, mutation::Mutation<T>>
    inline void GA<T>::mutation_method(F f)
    {
        mutation_ = std::make_unique<F>(std::move(f));
        use_default_mutation_rate_ = false;
    }

    template<typename T>
    inline void GA<T>::mutation_method(std::unique_ptr<typename mutation::Mutation<T>> f)
    {
        GAPP_ASSERT(f, "The mutation method can't be a nullptr.");

        mutation_ = std::move(f);
        use_default_mutation_rate_ = false;
    }

    template<typename T>
    inline void GA<T>::mutation_method(MutationCallable f)
    {
        mutation_ = std::make_unique<mutation::Lambda<T>>(std::move(f));
        use_default_mutation_rate_ = true;
    }

    template<typename T>
    inline const mutation::Mutation<T>& GA<T>::mutation_method() const& noexcept
    {
        GAPP_ASSERT(mutation_);

        return *mutation_;
    }

    template<typename T>
    inline void GA<T>::mutation_rate(Probability pm) noexcept
    {
        GAPP_ASSERT(mutation_);

        mutation_->mutation_rate(pm);
        use_default_mutation_rate_ = false;
    }

    template<typename T>
    inline Probability GA<T>::mutation_rate() const noexcept
    {
        GAPP_ASSERT(mutation_);

        return mutation_->mutation_rate();
    }

    template<typename T>
    inline void GA<T>::constraints_function(ConstraintsFunction f) noexcept
    {
        /* Nullptr is fine here, it just won't be called */
        constraints_function_ = std::move(f);
    }

    template<typename T>
    inline void GA<T>::repair_function(RepairCallable f) noexcept
    {
        /* Nullptr is fine here, it just won't be called */
        repair_ = std::move(f);
    }

    template<typename T>
    inline void GA<T>::cache_size(size_t generations) noexcept
    {
        cached_generations_ = generations;
    }

    template<typename T>
    inline std::pair<Positive<size_t>, size_t> GA<T>::findObjectiveProperties() const
    {
        GAPP_ASSERT(fitness_function_);

        Candidate<T> candidate = generateCandidate();
        validate(candidate);

        const FitnessVector fitness = (*fitness_function_)(candidate);
        GAPP_ASSERT(!fitness.empty(), "The number of objectives must be greater than 0.");

        return { fitness.size(), candidate.num_constraints() };
    }

    template<typename T>
    inline std::unique_ptr<algorithm::Algorithm> GA<T>::defaultAlgorithm() const
    {
        GAPP_ASSERT(fitness_function_);
        GAPP_ASSERT(num_objectives() > 0);

        if (num_objectives() == 1)
        {
            return std::make_unique<algorithm::SingleObjective>();
        }
        return std::make_unique<algorithm::NSGA3>();
    }

    template<typename T>
    inline Probability GA<T>::defaultMutationRate() const
    {
        GAPP_ASSERT(fitness_function_);

        return GaTraits<T>::defaultMutationRate(chrom_len());
    }

    template<typename T>
    inline bool GA<T>::hasValidFitness(const Candidate<T>& sol) const noexcept
    {
        GAPP_ASSERT(fitness_function_);

        return sol.is_evaluated() && (sol.fitness.size() == num_objectives());
    }

    template<typename T>
    inline bool GA<T>::hasValidConstraints(const Candidate<T>& sol) const noexcept
    {
        GAPP_ASSERT(fitness_function_);

        return sol.num_constraints() == num_constraints();
    }

    template<typename T>
    inline bool GA<T>::hasValidChromosome(const Candidate<T>& sol) const noexcept
    {
        return (crossover_->allow_variable_chrom_length() && mutation_->allow_variable_chrom_length()) ||
               (sol.chromosome.size() == chrom_len());
    }

    template<typename T>
    inline bool GA<T>::isValidEvaluatedPopulation(const Population<T>& pop) const
    {
        return std::all_of(pop.begin(), pop.end(), [this](const Candidate<T>& sol)
        {
            return hasValidChromosome(sol) && hasValidFitness(sol) && hasValidConstraints(sol);
        });
    }

    template<typename T>
    inline bool GA<T>::isValidUnevaluatedPopulation(const Population<T>& pop) const
    {
        return std::all_of(pop.begin(), pop.end(), [this](const Candidate<T>& sol)
        {
            return hasValidChromosome(sol) && (!sol.is_evaluated() || hasValidFitness(sol));
        });
    }

    template<typename T>
    inline bool GA<T>::fitnessMatrixIsSynced() const
    {
        return std::equal(fitness_matrix_.begin(), fitness_matrix_.end(), population_.begin(), population_.end(),
        [](const auto& fvec, const auto& sol)
        {
            return fvec == sol.fitness;
        });
    }


    template<typename T>
    void GA<T>::initializeAlgorithm(MaybeBoundsVector bounds, Population<T> initial_population)
    {
        GAPP_ASSERT(fitness_function_);
        GAPP_ASSERT(algorithm_ && stop_condition_);
        GAPP_ASSERT(crossover_ && mutation_);

        detail::execution_context::global_thread_pool.reset_scheduler();

        /* Reset state in case solve() has already been called before. */
        generation_cntr_ = 0;
        num_fitness_evals_ = 0;
        solutions_.clear();
        population_.clear();

        fitness_cache_.reset(!fitness_function_->is_dynamic() * cached_generations_ * population_size_);

        if constexpr (is_bounded<T>) { bounds_ = std::move(bounds); }

        /* Derived GA. */
        initialize();

        /* Create and evaluate the initial population of the algorithm. */
        std::tie(num_objectives_, num_constraints_) = findObjectiveProperties();
        population_ = generatePopulation(population_size_, std::move(initial_population));
        detail::parallel_for(population_.begin(), population_.end(), [this](Candidate<T>& sol) { validate(sol); repair(sol); evaluate(sol); });
        fitness_matrix_ = detail::toFitnessMatrix(population_);
        if (keep_all_optimal_sols_) solutions_ = detail::findParetoFront(population_);

        GAPP_ASSERT(isValidEvaluatedPopulation(population_));
        GAPP_ASSERT(fitnessMatrixIsSynced());

        /* Initialize the algorithm used.
         * This must be done after the initial population has been created and evaluted,
         * as it might want to use the population's fitness values (fitness_matrix_). */
        if (use_default_algorithm_) algorithm_ = defaultAlgorithm();
        algorithm_->initialize(*this);

        if (use_default_mutation_rate_) mutation_rate(defaultMutationRate());

        stop_condition_->initialize(*this);

        metrics_.initialize(*this);
        metrics_.update(*this);

        if (on_generation_end_) on_generation_end_(*this);
    }

    template<typename T>
    Population<T> GA<T>::generatePopulation(Positive<size_t> pop_size, Population<T> initial_population) const
    {
        GAPP_ASSERT(isValidUnevaluatedPopulation(initial_population), "An invalid initial population was specified for the GA.");

        Population<T> population;
        population.reserve(pop_size);

        const size_t npreset = std::min(size_t(pop_size), initial_population.size());
        std::move(initial_population.begin(), initial_population.begin() + npreset, std::back_inserter(population));

        while (population.size() < pop_size)
        {
            population.push_back(generateCandidate());
            GAPP_ASSERT(hasValidChromosome(population.back()), "An invalid solution was returned by generateCandidate().");
        }

        return population;
    }

    template<typename T>
    inline void GA<T>::prepareSelections() const
    {
        GAPP_ASSERT(algorithm_);
        GAPP_ASSERT(isValidEvaluatedPopulation(population_));
        GAPP_ASSERT(fitnessMatrixIsSynced());

        algorithm_->prepareSelections(*this, population_);
    }

    template<typename T>
    inline const Candidate<T>& GA<T>::select() const
    {
        GAPP_ASSERT(algorithm_);

        return algorithm_->select(*this, population_);
    }

    template<typename T>
    inline CandidatePair<T> GA<T>::crossover(const Candidate<T>& parent1, const Candidate<T>& parent2) const
    {
        GAPP_ASSERT(crossover_);

        return (*crossover_)(*this, parent1, parent2);
    }

    template<typename T>
    inline void GA<T>::mutate(Candidate<T>& sol) const
    {
        GAPP_ASSERT(mutation_);

        (*mutation_)(*this, sol);
    }

    template<typename T>
    void GA<T>::validate(Candidate<T>& sol) const
    {
        GAPP_ASSERT(hasValidChromosome(sol));

        /* Don't do anything for unconstrained optimization problems. */
        if (!constraints_function_) return;

        sol.constraint_violation = constraints_function_(*this, sol.chromosome);
    }

    template<typename T>
    void GA<T>::repair(Candidate<T>& sol) const
    {
        GAPP_ASSERT(hasValidChromosome(sol));

        /* Don't try to do anything unless a repair function is set. */
        if (!repair_) return;

        if (repair_(*this, sol, sol.chromosome))
        {
            sol.fitness.clear();
            validate(sol);
        }

        GAPP_ASSERT(hasValidChromosome(sol), "Invalid chromosome returned by the repair function.");
    }

    template<typename T>
    void GA<T>::updatePopulation(Population<T>&& children)
    {
        GAPP_ASSERT(algorithm_);
        GAPP_ASSERT(isValidEvaluatedPopulation(population_));
        GAPP_ASSERT(fitnessMatrixIsSynced());

        fitness_cache_.insert(population_.begin(), population_.end(), &Candidate<T>::fitness);
        population_ = algorithm_->nextPopulation(*this, std::move(population_), std::move(children));
        fitness_matrix_ = detail::toFitnessMatrix(population_);
    }

    template<typename T>
    inline bool GA<T>::stopCondition() const
    {
        GAPP_ASSERT(stop_condition_);

        return (*stop_condition_)(*this);
    }

    template<typename T>
    inline void GA<T>::evaluate(Candidate<T>& sol)
    {
        GAPP_ASSERT(fitness_function_);
        GAPP_ASSERT(hasValidChromosome(sol));

        /* If the fitness function is static, and the solution has already
         * been evaluted sometime earlier (in an earlier generation), there
         * is no point doing it again. */
        if (!fitness_function_->is_dynamic() && sol.is_evaluated()) return;
        
        if (cached_generations_)
        {
            GAPP_ASSERT(!fitness_function_->is_dynamic());

            if (const FitnessVector* fitness = fitness_cache_.get(sol))
            {
                sol.fitness = *fitness;
                return;
            }
        }

        std::atomic_ref{ num_fitness_evals_ }.fetch_add(1, std::memory_order_release);
        sol.fitness = (*fitness_function_)(sol);

        GAPP_ASSERT(hasValidFitness(sol));
    }

    template<typename T>
    void GA<T>::updateOptimalSolutions(Candidates<T>& optimal_sols, const Population<T>& pop) const
    {
        GAPP_ASSERT(algorithm_);

        auto optimal_pop = algorithm_->optimalSolutions(*this, pop);

        optimal_sols = detail::mergeParetoSets(std::move(optimal_sols), std::move(optimal_pop));

        /* Duplicate elements are removed from optimal_sols using exact comparison
        *  of the chromosomes in order to avoid issues with using a non-transitive
        *  comparison function for std::sort and std::unique. */
        auto chrom_eq = [](const auto& lhs, const auto& rhs) { return lhs.chromosome == rhs.chromosome; };
        auto chrom_less = [](const auto& lhs, const auto& rhs) { return lhs.chromosome < rhs.chromosome; };

        std::sort(optimal_sols.begin(), optimal_sols.end(), chrom_less);
        auto last = std::unique(optimal_sols.begin(), optimal_sols.end(), chrom_eq);
        optimal_sols.erase(last, optimal_sols.end());
    }

    template<typename T>
    void GA<T>::advance()
    {
        GAPP_ASSERT(population_.size() == population_size_);

        Population<T> children(population_size_);

        prepareSelections();

        detail::parallel_for(detail::iota_iterator(0_sz), detail::iota_iterator(population_size_ / 2), [&](size_t i)
        {
            CandidatePair child_pair = crossover(select(), select());
            children[2 * i]     = std::move(child_pair.first);
            children[2 * i + 1] = std::move(child_pair.second);
        });

        if (population_size_ % 2) children.back() = crossover(select(), select()).first;

        detail::parallel_for(children.begin(), children.end(), [this](Candidate<T>& child)
        {
            mutate(child);
            validate(child);
            repair(child);
            evaluate(child);
        });

        updatePopulation(std::move(children));

        if (keep_all_optimal_sols_) updateOptimalSolutions(solutions_, population_);
        metrics_.update(*this);

        if (on_generation_end_) on_generation_end_(*this);
        generation_cntr_++;
    }

    template<typename T>
    Candidates<T> GA<T>::solve(std::unique_ptr<FitnessFunctionBase<T>> fitness_function, size_t generations, Population<T> initial_population) requires (!is_bounded<T>)
    {
        GAPP_ASSERT(fitness_function, "The fitness function can't be a nullptr.");

        detail::restore_on_exit _{ max_gen_ };

        fitness_function_ = std::move(fitness_function);
        max_gen(generations);

        initializeAlgorithm({ /* no bounds */ }, std::move(initial_population));
        while (!stopCondition())
        {
            advance();
        }
        if (!keep_all_optimal_sols_) updateOptimalSolutions(solutions_, population_);

        return solutions_;
    }

    template<typename T>
    Candidates<T> GA<T>::solve(std::unique_ptr<FitnessFunctionBase<T>> fitness_function, BoundsVector<T> bounds, size_t generations, Population<T> initial_population) requires (is_bounded<T>)
    {
        GAPP_ASSERT(fitness_function, "The fitness function can't be a nullptr.");
        GAPP_ASSERT(bounds.size() == fitness_function->chrom_len(), "The length of the bounds vector must match the chromosome length.");

        detail::restore_on_exit _{ max_gen_ };

        fitness_function_ = std::move(fitness_function);
        max_gen(generations);

        initializeAlgorithm(std::move(bounds), std::move(initial_population));
        while (!stopCondition())
        {
            advance();
        }
        if (!keep_all_optimal_sols_) updateOptimalSolutions(solutions_, population_);

        return solutions_;
    }

    template<typename T>
    Candidates<T> GA<T>::solve(std::unique_ptr<FitnessFunctionBase<T>> fitness_function, Population<T> initial_population) requires (!is_bounded<T>)
    {
        return solve(std::move(fitness_function), max_gen(), std::move(initial_population));
    }

    template<typename T>
    template<typename F>
    requires (!is_bounded<T> && std::derived_from<F, FitnessFunctionBase<T>>)
    Candidates<T> GA<T>::solve(F fitness_function, Population<T> initial_population)
    {
        return solve(std::make_unique<F>(std::move(fitness_function)), max_gen(), std::move(initial_population));
    }

    template<typename T>
    template<typename F>
    requires (!is_bounded<T> && std::derived_from<F, FitnessFunctionBase<T>>)
    Candidates<T> GA<T>::solve(F fitness_function, size_t generations, Population<T> initial_population)
    {
        return solve(std::make_unique<F>(std::move(fitness_function)), generations, std::move(initial_population));
    }

    template<typename T>
    Candidates<T> GA<T>::solve(std::unique_ptr<FitnessFunctionBase<T>> fitness_function, Bounds<T> bounds, size_t generations, Population<T> initial_population) requires (is_bounded<T>)
    {
        const size_t chrom_len = fitness_function->chrom_len();

        return solve(std::move(fitness_function), BoundsVector<T>(chrom_len, bounds), generations, std::move(initial_population));
    }

    template<typename T>
    Candidates<T> GA<T>::solve(std::unique_ptr<FitnessFunctionBase<T>> fitness_function, BoundsVector<T> bounds, Population<T> initial_population) requires (is_bounded<T>)
    {
        return solve(std::move(fitness_function), std::move(bounds), max_gen(), std::move(initial_population));
    }

    template<typename T>
    Candidates<T> GA<T>::solve(std::unique_ptr<FitnessFunctionBase<T>> fitness_function, Bounds<T> bounds, Population<T> initial_population) requires (is_bounded<T>)
    {
        const size_t chrom_len = fitness_function->chrom_len();

        return solve(std::move(fitness_function), BoundsVector<T>(chrom_len, bounds), max_gen(), std::move(initial_population));
    }

    template<typename T>
    template<typename F>
    requires (is_bounded<T> && std::derived_from<F, FitnessFunctionBase<T>>)
    Candidates<T> GA<T>::solve(F fitness_function, BoundsVector<T> bounds, Population<T> initial_population)
    {
        return solve(std::make_unique<F>(std::move(fitness_function)), std::move(bounds), max_gen(), std::move(initial_population));
    }

    template<typename T>
    template<typename F>
    requires (is_bounded<T> && std::derived_from<F, FitnessFunctionBase<T>>)
    Candidates<T> GA<T>::solve(F fitness_function, Bounds<T> bounds, Population<T> initial_population)
    {
        const size_t chrom_len = fitness_function.FitnessFunctionBase<T>::chrom_len();

        return solve(std::make_unique<F>(std::move(fitness_function)), BoundsVector<T>(chrom_len, bounds), max_gen(), std::move(initial_population));
    }

    template<typename T>
    template<typename F>
    requires (is_bounded<T> && std::derived_from<F, FitnessFunctionBase<T>>)
    Candidates<T> GA<T>::solve(F fitness_function, BoundsVector<T> bounds, size_t generations, Population<T> initial_population)
    {
        return solve(std::make_unique<F>(std::move(fitness_function)), std::move(bounds), generations, std::move(initial_population));
    }

    template<typename T>
    template<typename F>
    requires (is_bounded<T> && std::derived_from<F, FitnessFunctionBase<T>>)
    Candidates<T> GA<T>::solve(F fitness_function, Bounds<T> bounds, size_t generations, Population<T> initial_population)
    {
        const size_t chrom_len = fitness_function.FitnessFunctionBase<T>::chrom_len();

        return solve(std::make_unique<F>(std::move(fitness_function)), BoundsVector<T>(chrom_len, bounds), generations, std::move(initial_population));
    }

} // namespace gapp

#endif // !GAPP_CORE_GA_BASE_IMPL_HPP
