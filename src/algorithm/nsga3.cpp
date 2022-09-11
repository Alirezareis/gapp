/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#include "nsga3.hpp"
#include "nd_sort.hpp"
#include "reference_lines.hpp"
#include "../core/ga_info.hpp"
#include "../population/population.hpp"
#include "../utility/algorithm.hpp"
#include "../utility/functional.hpp"
#include "../utility/math.hpp"
#include "../utility/rng.hpp"
#include "../utility/utility.hpp"
#include "../utility/cone_tree.hpp"
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <cstddef>
#include <cassert>
#include <stdexcept>

namespace genetic_algorithm::algorithm
{
    using namespace dtl;
    using math::Point;
    using detail::FitnessVector;
    using detail::FitnessMatrix;


    /* Achievement scalarization function. */
    static inline double ASF(const std::vector<double>& ideal_point, const std::vector<double>& weights, const std::vector<double>& fitness) noexcept
    {
        assert(!ideal_point.empty());
        assert(weights.size() == ideal_point.size());
        assert(fitness.size() == weights.size());

        double dmax = -math::inf<double>;

        for (size_t i = 0; i < fitness.size(); i++)
        {
            dmax = std::max(dmax, (ideal_point[i] - fitness[i]) / weights[i]);
        }
        
        return dmax;
    }

    /* Create a weight vector for the given axis (used in the ASF). */
    static inline std::vector<double> weightVector(size_t dimensions, size_t axis)
    {
        assert(dimensions > axis);

        std::vector weights(dimensions, 1E-6);
        weights[axis] = 1.0;

        return weights;
    }

    /* Find an approximation of the pareto front's nadir point using the minimum of the extreme points. */
    static inline Point findNadirPoint(const std::vector<Point>& extreme_points)
    {
        assert(!extreme_points.empty());

        /* Nadir point estimate = minimum of extreme points along each objective axis. */
        Point nadir = extreme_points[0];
        for (size_t i = 1; i < extreme_points.size(); i++)
        {
            nadir = detail::elementwise_min(std::move(nadir), extreme_points[i]);
        }

        return nadir;
    }

    /* Normalize a fitness vector using the ideal and nadir points. */
    static inline FitnessVector normalizeFitnessVec(const FitnessVector& fvec, const Point& ideal_point, const Point& nadir_point)
    {
        assert(fvec.size() == ideal_point.size());
        assert(ideal_point.size() == nadir_point.size());

        FitnessVector fnorm(fvec.size());

        for (size_t i = 0; i < fnorm.size(); i++)
        {
            fnorm[i] = (ideal_point[i] - fvec[i]) / std::max(ideal_point[i] - nadir_point[i], 1E-6);
        }

        return fnorm;
    }

    /* Increment the niche count of ref, while keeping refs sorted based on niche counts. */
    inline static void incrementNicheCount(std::vector<RefLine*>& refs, RefLine* ref)
    {
        ref->niche_count++;

        auto current = std::find(refs.begin(), refs.end(), ref);
        auto first_eq = std::find_if(std::next(current), refs.end(), [&](const RefLine* r) { return r->niche_count >= (*current)->niche_count; });

        std::iter_swap(current, std::prev(first_eq));
    }


    /* NSGA3 IMPLEMENTATION */

    static constexpr auto RefProjection = Fn<&RefLine::direction>;

    using RefProjectionType = std::remove_cvref_t<decltype(RefProjection)>;
    using RefLineSearchTree = detail::ConeTree<RefLine, RefProjectionType>;

    struct NSGA3::Impl
    {
        struct CandidateInfo
        {
            size_t rank;
            RefLine* ref;
            double ref_dist;
        };

        std::vector<CandidateInfo> sol_info_;
        RefLineSearchTree ref_lines_;

        Point ideal_point_;
        Point nadir_point_;
        std::vector<Point> extreme_points_;

        /* Update the ideal point approximation using the new points in fmat, assuming maximization. */
        void updateIdealPoint(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last);

        /* Update the extreme points using the new points in fmat, assuming maximization. */
        void updateExtremePoints(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last);

        /* Update the current nadir point based on the extreme points. */
        void updateNadirPoint(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last);

        /* Recalculate the niche counts of the reference lines based on the ref lines associated with the candidates in [first, last). */
        void recalcNicheCounts(ParetoFronts::const_iterator first, ParetoFronts::const_iterator last);

        /* Find the closest reference and its distance for each of the points in the fitness matrix. */
        void associatePopWithRefs(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last,
                                  ParetoFronts::const_iterator pfirst, ParetoFronts::const_iterator plast);

        /* Return true if pop[lhs] is better than pop[rhs]. */
        bool nichedCompare(size_t lhs, size_t rhs) const noexcept;

        /* Return the associated reference direction of a candidate. */
        RefLine& refPointOf(const FrontInfo& sol) noexcept;
        const RefLine& refPointOf(const FrontInfo& sol) const noexcept;

        /* Return the associated reference direction's distance for a candidate. */
        double refDistOf(const FrontInfo& sol) const noexcept;

        /* Return the (unique) reference lines that are associated with at least one element in the range [first, last), sorted based on their niche counts. */
        std::vector<RefLine*> referenceSetOf(ParetoFronts::const_iterator first, ParetoFronts::const_iterator last);
    };

    NSGA3::NSGA3() :
        pimpl_(std::make_unique<Impl>())
    {}

    //NSGA3::NSGA3(const NSGA3& rhs) :
    //    pimpl_(std::make_unique<Impl>(*rhs.pimpl_))
    //{}

    //NSGA3& NSGA3::operator=(const NSGA3& rhs)
    //{
    //    pimpl_ = std::make_unique<Impl>(*rhs.pimpl_);
    //}

    NSGA3::NSGA3(NSGA3&&) noexcept            = default;
    NSGA3& NSGA3::operator=(NSGA3&&) noexcept = default;
    NSGA3::~NSGA3()                           = default;

    inline void NSGA3::Impl::updateIdealPoint(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::distance(first, last) > 0);

        ideal_point_ = detail::elementwise_max(std::move(ideal_point_), detail::maxFitness(first, last));
    }

    void NSGA3::Impl::updateExtremePoints(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        assert(std::distance(first, last) > 0);
        assert(first->size() == ideal_point_.size());

        const size_t popsize = size_t(last - first);

        std::vector<Point> new_extreme_points;
        new_extreme_points.reserve(extreme_points_.size());

        for (size_t dim = 0; dim < ideal_point_.size(); dim++)
        {
            auto weights = weightVector(ideal_point_.size(), dim);
            auto ASFi = [&](const FitnessVector& fvec) noexcept
            {
                return ASF(ideal_point_, weights, fvec);
            };

            std::vector<double> chebysev_distances(popsize + extreme_points_.size());

            std::transform(first, last, chebysev_distances.begin(), ASFi);
            std::transform(extreme_points_.begin(), extreme_points_.end(), chebysev_distances.begin() + popsize, ASFi);

            size_t idx = detail::argmin(chebysev_distances.begin(), chebysev_distances.begin(), chebysev_distances.end());

            (idx >= popsize) ?
                new_extreme_points.push_back(extreme_points_[idx - popsize]) :
                new_extreme_points.push_back(first[idx]);
        }

        extreme_points_ = std::move(new_extreme_points);
    }

    inline void NSGA3::Impl::updateNadirPoint(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last)
    {
        updateExtremePoints(first, last);
        nadir_point_ = findNadirPoint(extreme_points_);
    }

    inline void NSGA3::Impl::recalcNicheCounts(ParetoFronts::const_iterator first, ParetoFronts::const_iterator last)
    {
        std::for_each(ref_lines_.data().begin(), ref_lines_.data().end(), [](RefLine& ref) { ref.niche_count = 0; });
        std::for_each(first, last, [&](const FrontInfo& sol) { refPointOf(sol).niche_count++; });
    }

    void NSGA3::Impl::associatePopWithRefs(FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last,
                                           ParetoFronts::const_iterator pfirst, ParetoFronts::const_iterator plast)
    {
        assert(std::distance(first, last) > 0);
        assert(std::all_of(first, last, [first](const FitnessVector& sol) { return sol.size() == first[0].size(); }));
        assert(!ref_lines_.size() == 0);

        updateIdealPoint(first, last);
        updateExtremePoints(first, last);
        nadir_point_ = findNadirPoint(extreme_points_);

        sol_info_.resize(last - first);

        std::for_each(GA_EXECUTION_UNSEQ, pfirst, plast, [&](const FrontInfo& sol) /* par exec is worse here for popsize 100 */
        {
            const FitnessVector& fvec = first[sol.idx];
            const FitnessVector fnorm = normalizeFitnessVec(fvec, ideal_point_, nadir_point_);

            const auto best = ref_lines_.findBestMatch(fnorm);

            sol_info_[sol.idx].ref = best.elem;
            sol_info_[sol.idx].ref_dist = math::perpendicularDistanceSq(best.elem->direction, fnorm);
        });
    }

    inline bool NSGA3::Impl::nichedCompare(size_t lhs, size_t rhs) const noexcept
    {
        /* This version of the compare implementation is from the U-NSGA-III algorithm. */
        if (sol_info_[lhs].ref == sol_info_[rhs].ref)
        {
            if (sol_info_[lhs].rank != sol_info_[rhs].rank)
            {
                return sol_info_[lhs].rank < sol_info_[rhs].rank;
            }
            else
            {
                return sol_info_[lhs].ref_dist < sol_info_[rhs].ref_dist;
            }
        }

        return rng::randomBool();
    }

    inline RefLine& NSGA3::Impl::refPointOf(const FrontInfo& sol) noexcept
    {
        return *sol_info_[sol.idx].ref;
    }

    inline const RefLine& NSGA3::Impl::refPointOf(const FrontInfo& sol) const noexcept
    {
        return *sol_info_[sol.idx].ref;
    }

    inline double NSGA3::Impl::refDistOf(const FrontInfo& sol) const noexcept
    {
        return sol_info_[sol.idx].ref_dist;
    }

    inline std::vector<RefLine*> NSGA3::Impl::referenceSetOf(ParetoFronts::const_iterator first, ParetoFronts::const_iterator last)
    {
        std::vector<RefLine*> refs(last - first);
        std::transform(first, last, refs.begin(), [this](const FrontInfo& sol) { return &refPointOf(sol); });

        detail::erase_duplicates(refs);
        std::sort(refs.begin(), refs.end(), [](const RefLine* lhs, const RefLine* rhs)
        {
            return lhs->niche_count < rhs->niche_count;
        });

        return refs;
    }


    /* NSGA3 INTERFACE FUNCTIONS */

    void NSGA3::initializeImpl(const GaInfo& ga)
    {
        assert(ga.population_size() != 0);

        if (ga.num_objectives() <= 1)
        {
            GA_THROW(std::logic_error, "The number of objectives must be greater than 1 for the NSGA-III algorithm.");
        }

        auto& fitness_matrix = ga.fitness_matrix();

        pimpl_->ideal_point_ = detail::maxFitness(fitness_matrix.begin(), fitness_matrix.end());
        pimpl_->extreme_points_ = {};

        auto ref_lines = dtl::generateReferencePoints(ga.num_objectives(), ga.population_size());

        pimpl_->ref_lines_ = detail::ConeTree(std::make_move_iterator(ref_lines.begin()),
                                              std::make_move_iterator(ref_lines.end()),
                                              RefProjection);

        auto pfronts = nonDominatedSort(fitness_matrix.begin(), fitness_matrix.end());

        pimpl_->sol_info_.resize(ga.population_size());
        std::for_each(pfronts.begin(), pfronts.end(), [this](const FrontInfo& sol) { pimpl_->sol_info_[sol.idx].rank = sol.rank; });

        pimpl_->associatePopWithRefs(fitness_matrix.begin(), fitness_matrix.end(), pfronts.begin(), pfronts.end());
        pimpl_->recalcNicheCounts(pfronts.begin(), pfronts.end());
    }

    std::vector<size_t> NSGA3::nextPopulationImpl(const GaInfo& ga,
                                              FitnessMatrix::const_iterator parents_first,
                                              FitnessMatrix::const_iterator children_first,
                                              FitnessMatrix::const_iterator children_last)
    {
        const size_t popsize = ga.population_size();

        assert(ga.num_objectives() > 1);
        assert(size_t(children_first - parents_first) == popsize);
        assert(std::all_of(parents_first, children_last, [&ga](const FitnessVector& f) { return f.size() == ga.num_objectives(); }));

        GA_UNUSED(children_first);

        pimpl_->sol_info_.resize(children_last - parents_first);

        auto pfronts = dtl::nonDominatedSort(parents_first, children_last);
        std::for_each(pfronts.begin(), pfronts.end(), [this](const FrontInfo& sol) { pimpl_->sol_info_[sol.idx].rank = sol.rank; });

        auto [partial_front_first, partial_front_last] = findPartialFront(pfronts.begin(), pfronts.end(), popsize);

        /* The ref lines of the candidates after partial_front_last are irrelevant, as they can never be part of the next population. */
        pimpl_->associatePopWithRefs(parents_first, children_last, pfronts.begin(), partial_front_last);
        /* The niche counts should be calculated excluding the partial front for now. */
        pimpl_->recalcNicheCounts(pfronts.begin(), partial_front_first);

        /* Keep track of the details of the candidates added to new_pop to avoid a second sort in prepareSelections(). */
        std::vector<size_t> new_pop;
        std::vector<Impl::CandidateInfo> new_info;
        new_pop.reserve(popsize);
        new_info.reserve(popsize);

        for (auto sol = pfronts.begin(); sol != partial_front_first; ++sol)
        {
            new_pop.push_back(sol->idx);
            new_info.push_back(pimpl_->sol_info_[sol->idx]);
        }

        /* Add remaining candidates from the partial front if there is one. */
        std::vector<RefLine*> refs = pimpl_->referenceSetOf(partial_front_first, partial_front_last);

        while (new_pop.size() != ga.population_size())
        {
            /* Choose a random reference direction with minimal niche count. */
            const auto last_minimal = std::find_if(refs.begin(), refs.end(), [&](const RefLine* ref) { return ref->niche_count != refs[0]->niche_count; });
            RefLine* ref = rng::randomElement(refs.begin(), last_minimal);

            /* Find the idx of the closest sol in the partial front associated with this ref direction. */
            auto candidates = detail::find_all(partial_front_first, partial_front_last,
                                               [this, ref](const FrontInfo& sol) { return &pimpl_->refPointOf(sol) == ref; });

            auto& selected = *std::min_element(candidates.begin(), candidates.end(),
                                               [this](const auto& lhs, const auto& rhs) { return pimpl_->refDistOf(*lhs) < pimpl_->refDistOf(*rhs); });

            new_pop.push_back(selected->idx);
            new_info.push_back(pimpl_->sol_info_[selected->idx]);
            /* Remove the selected candidate from the partial front so it can't be selected again. */
            std::iter_swap(selected, partial_front_first++);

            /* If the selected candidate was the only one in the partial front associated with this reference direction,
               the reference direction needs to also be removed. */
            if (candidates.size() == 1)
            {
                detail::erase_first_stable(refs, ref);
            }
            else /* Otherwise just increase its niche count, and make sure the refs are still sorted. */
            {
                incrementNicheCount(refs, ref);
            }
        }
        pimpl_->sol_info_ = std::move(new_info);

        return new_pop;
    }

    size_t NSGA3::selectImpl(const GaInfo&, const FitnessMatrix& pop) const
    {
        assert(!pop.empty());

        const size_t idx1 = rng::randomIdx(pop);
        const size_t idx2 = rng::randomIdx(pop);

        return pimpl_->nichedCompare(idx1, idx2) ? idx1 : idx2;
    }

    std::optional<std::vector<size_t>> NSGA3::optimalSolutionsImpl(const GaInfo&) const
    {
        return detail::find_indices(pimpl_->sol_info_,
                                    detail::compose(&Impl::CandidateInfo::rank,
                                                    detail::equal_to(0_sz)));
    }

} // namespace genetic_algorithm::algorithm