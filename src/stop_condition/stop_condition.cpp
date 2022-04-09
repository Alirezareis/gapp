/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "stop_condition.hpp"
#include "../algorithms/ga_base.decl.hpp"
#include "../population/population.hpp"
#include "../utility/math.hpp"
#include <algorithm>
#include <stdexcept>

namespace genetic_algorithm::stopping
{
    FitnessEvals::FitnessEvals(size_t max_fitness_evals)
        : StopCondition()
    {
        this->max_fitness_evals(max_fitness_evals);
    }

    void FitnessEvals::max_fitness_evals(size_t max_fitness_evals)
    {
        max_fitness_evals_ = max_fitness_evals;
    }

    bool FitnessEvals::operator()(const GaInfo& ga)
    {
        return (ga.num_fitness_evals() >= max_fitness_evals_);
    }

    FitnessValue::FitnessValue(const std::vector<double>& fitness_threshold) :
        StopCondition()
    {
        this->fitness_threshold(fitness_threshold);
    }

    void FitnessValue::fitness_threshold(const std::vector<double>& fitness_threshold)
    {
        if (fitness_threshold.empty())
        {
            throw std::invalid_argument("Empty fitness threshold vector.");
        }

        fitness_threshold_ = fitness_threshold;
    }

    bool FitnessValue::operator()(const GaInfo& ga)
    {
        if (ga.num_objectives() != fitness_threshold_.size())
        {
            throw std::domain_error("The size of the fitness threshold vector does not match the size of the fitness vectors.");
        }

        auto fitness_matrix = ga.fitness_matrix();

        return std::any_of(fitness_matrix.begin(), fitness_matrix.end(),
        [this](const std::vector<double>& sol)
        {
            return detail::paretoCompareLess(fitness_threshold_, sol);
        });
    }

    FitnessMeanStall::FitnessMeanStall(size_t patience, double delta)
        : StopCondition()
    {
        this->patience(patience);
        this->delta(delta);
    }

    void FitnessMeanStall::patience(size_t patience)
    {
        patience_ = patience;
        resetCntr();
    }

    void FitnessMeanStall::delta(double delta)
    {
        delta_ = delta;
    }

    void FitnessMeanStall::resetCntr()
    {
        cntr_ = patience_ + 1;
    }

    bool FitnessMeanStall::operator()(const GaInfo& ga)
    {
        auto current_mean = detail::populationFitnessMean(ga.fitness_matrix());

        /* Init on first gen. */
        if (ga.generation_cntr() == 0)
        {
            resetCntr();
            best_fitness_mean_ = current_mean;

            return false;
        }

        bool improved = false;
        for (size_t i = 0; i < current_mean.size(); i++)
        {
            if (current_mean[i] >= best_fitness_mean_[i] + delta_)
            {
                best_fitness_mean_[i] = current_mean[i];
                improved = true;
            }
        }

        if (improved)
        {
            resetCntr();
        }
        else
        {
            --cntr_;
        }

        return cntr_ == 0;
    }

    FitnessBestStall::FitnessBestStall(size_t patience, double delta) :
        StopCondition()
    {
        this->patience(patience);
        this->delta(delta);
    }

    void FitnessBestStall::patience(size_t patience)
    {
        patience_ = patience;
        resetCntr();
    }

    void FitnessBestStall::delta(double delta)
    {
        delta_ = delta;
    }

    void FitnessBestStall::resetCntr()
    {
        cntr_ = patience_ + 1;
    }

    bool FitnessBestStall::operator()(const GaInfo& ga)
    {
        auto current_max = detail::populationFitnessMax(ga.fitness_matrix());

        /* Init on first gen. */
        if (ga.generation_cntr() == 0)
        {
            resetCntr();
            best_fitness_max_ = current_max;

            return false;
        }

        bool improved = false;
        for (size_t i = 0; i < current_max.size(); i++)
        {
            if (current_max[i] >= best_fitness_max_[i] + delta_)
            {
                best_fitness_max_[i] = current_max[i];
                improved = true;
            }
        }

        if (improved)
        {
            resetCntr();
        }
        else
        {
            --cntr_;
        }

        return cntr_ == 0;
    }

} // namespace genetic_algorithm::stopping