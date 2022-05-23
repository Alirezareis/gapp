/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_SELECTION_DTL_HPP
#define GA_SELECTION_DTL_HPP

#include "../population/population.hpp"
#include "multi_objective.hpp"
#include <vector>
#include <cstddef>
#include <functional>

namespace genetic_algorithm::selection::dtl
{
    using genetic_algorithm::detail::FitnessVector;
    using genetic_algorithm::detail::FitnessMatrix;
    using Point = genetic_algorithm::selection::multi_objective::NSGA3::Point;
    using RefPoint = genetic_algorithm::selection::multi_objective::NSGA3::RefPoint;

    /* Calculate the selection weights of the population for the roulette selection. */
    std::vector<double> rouletteWeights(const FitnessMatrix& pop);

    /* Calculate the selection weights of the population for the rank selection. */
    std::vector<double> rankWeights(const FitnessMatrix& pop, double wmin, double wmax);

    /* Calculate the selection weights of the population for the sigma selection. */
    std::vector<double> sigmaWeights(const FitnessMatrix& pop, double scale);

    /* Calculate the selection weights of the population for the Boltzmann selection. */
    std::vector<double> boltzmannWeights(const FitnessMatrix& pop, double temperature);

    /* Default temperature function used for the Boltzmann selection. */
    double boltzmannDefaultTemp(size_t gen, size_t max_gen) noexcept;

    /* Calculate the cumulative distribution function of the population from the selection weights. */
    std::vector<double> weightsToCdf(const std::vector<double>& selection_weights);


    /* Sorted (idx, rank) pairs. */
    using ParetoFronts = std::vector<std::pair<size_t, size_t>>;

    /* Non-dominated sorting for the multi-objective algorithms. Returns the pareto fronts (idx, rank pairs) of the population. */
    ParetoFronts nonDominatedSort(const FitnessMatrix& fmat);

    /* Returns the rank of each candidate based on the pareto fronts. */
    std::vector<size_t> paretoRanks(const ParetoFronts& pareto_fronts);

    /* Finds the first element of the front following the front which current is a part of. */
    ParetoFronts::iterator nextFrontBegin(ParetoFronts::iterator current, ParetoFronts::iterator last) noexcept;

    /* Finds the first and last elements of each front in the pareto fronts vector. */
    auto paretoFrontBounds(ParetoFronts& pareto_fronts) -> std::vector<std::pair<ParetoFronts::iterator, ParetoFronts::iterator>>;

    /* Calculate the crowding distances of the solutions in the NSGA2 algorithm. */
    std::vector<double> crowdingDistances(const FitnessMatrix& fmat, ParetoFronts pfronts);

    /* Generate n reference points on the unit simplex in dim dimensions from a uniform distribution (for the NSGA-III algorithm). */
    std::vector<Point> generateRefPoints(size_t n, size_t dim);

    /* Find the index and distance of the closest reference line to the point p. */
    std::pair<size_t, double> findClosestRef(const std::vector<RefPoint>& refs, const Point& p);

    /* Achievement scalarization function for the NSGA-III algorithm. */
    std::function<double(const std::vector<double>&)> ASF(std::vector<double> z, std::vector<double> w) noexcept;

} // namespace genetic_algorithm::selection::dtl

#endif // !GA_SELECTION_DTL_HPP