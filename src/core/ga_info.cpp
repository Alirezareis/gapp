/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "ga_info.hpp"
#include "../algorithm/algorithm_base.hpp"
#include "../algorithm/single_objective.hpp"
#include "../stop_condition/stop_condition.hpp"
#include "../algorithm/nsga3.hpp"
#include "../utility/utility.hpp"
#include <vector>
#include <atomic>
#include <memory>
#include <utility>
#include <stdexcept>

namespace genetic_algorithm
{
    GaInfo::GaInfo(GaInfo&&) noexcept            = default;
    GaInfo& GaInfo::operator=(GaInfo&&) noexcept = default;
    GaInfo::~GaInfo()                            = default;

    GaInfo::GaInfo(size_t population_size, size_t nobj)
    {
        if (nobj == 0) GA_THROW(std::invalid_argument, "The number of objectives must be at least 1.");

        this->population_size(population_size);

        (nobj == 1) ?
            algorithm(std::make_unique<algorithm::SingleObjective>()) :
            algorithm(std::make_unique<algorithm::NSGA3>());

        stop_condition_ = std::make_unique<stopping::NoEarlyStop>();
    }

    void GaInfo::population_size(size_t size)
    {
        if (size == 0) GA_THROW(std::invalid_argument, "The population size must be at least 1.");

        population_size_ = size;
    }

    void GaInfo::max_gen(size_t max_gen)
    {
        if (max_gen == 0) GA_THROW(std::invalid_argument, "The number of generations must be at least 1.");

        max_gen_ = max_gen;
    }

    size_t GaInfo::num_fitness_evals() const noexcept
    {
        std::atomic_ref num_fitness_evals{ num_fitness_evals_ };
        return num_fitness_evals.load(std::memory_order_acquire);
    }

    void GaInfo::stop_condition(StopConditionCallable f)
    {
        stop_condition_ = std::make_unique<stopping::Lambda>(std::move(f));
    }

} // namespace genetic_algorithm