/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_ALGORITHM_NSGA3_HPP
#define GA_ALGORITHM_NSGA3_HPP

#include "algorithm_base.hpp"

namespace genetic_algorithm::algorithm
{
    /**
    * NSGA-III algorithm, used for many-objective optimization.
    * (This algorithm doesn't work for single-objective problems.) \n
    * The aim of the algorithm is to find a set of solutions which are well-spread out
    * along the entire pareto-front (in the objective-space). \n
    * 
    * The algorithm uses non-dominated sorting to sort the solutions into
    * pareto fronts, and then selects the candidates of the best fronts for the population
    * of the next generation. \n
    * Candidates that belong to the same front are ranked using a predefined set of reference
    * directions in the objective space. Candidates that are closest to reference directions
    * which have less candidates associated with them, and candidates closer to reference directions
    * are considered better. \n
    * The reference directions are generated at the start of the run, and don't change throughout it. \n
    * 
    * The algorithm uses a selection operator that selects solutions based on these same criteria
    * (their pareto ranks and their distances from the reference directions). \n
    * 
    * The selection and population update methods of this algorithm can't be changed. \n
    * Has no parameters.
    */
    class NSGA3 final : public Algorithm
    {
    public:
        void initialize(const GaInfo& ga) override;
        void prepareSelections(const GaInfo&, const FitnessMatrix&) override {}
        size_t select(const GaInfo& ga, const FitnessMatrix& fmat) override;
        std::vector<size_t> nextPopulation(const GaInfo& ga,
                                           FitnessMatrix::const_iterator first,
                                           FitnessMatrix::const_iterator children_first,
                                           FitnessMatrix::const_iterator last) override;

        using Point = std::vector<double>;

        /* A reference point/direction with its associated niche-count. */
        struct RefPoint
        {
            const Point point;
            size_t niche_count = 0;

            RefPoint(const Point& p) : point(p) {}
        };

    private:
        /* Stats associated with each of the solutions. */
        struct CandidateInfo
        {
            size_t rank = 0;
            size_t ref_idx = 0;
            double ref_dist = 0.0;
        };

        /* Achievement scalarization function type. */
        using ASF = std::function<double(const std::vector<double>&)>;

        std::vector<CandidateInfo> sol_info_;
        std::vector<RefPoint> ref_points_;

        Point ideal_point_;
        Point nadir_point_;
        std::vector<Point> extreme_points_;

        /* Generate n reference points on the unit simplex in dim dimensions from a uniform distribution. */
        static std::vector<Point> generateRefPoints(size_t n, size_t dim);

        /* Find the index and distance of the closest reference line to the point p. */
        static std::pair<size_t, double> findClosestRef(const std::vector<RefPoint>& refs, const Point& p);

        /* Create an achievement scalarization function. */
        static ASF getASF(std::vector<double> ideal_point, std::vector<double> weights) noexcept;

        /* Create a weight vector for the given axis (used in the ASF). */
        static std::vector<double> weightVector(size_t dimensions, size_t axis);

        /* Update the approximate ideal point using new points in fmat, assuming maximization. */
        static void updateIdealPoint(Point& ideal_point, FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last);

        /* Update the extreme points with the new points in fmat. */
        static void updateExtremePoints(std::vector<Point>& extreme_points, const Point& ideal_point, FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last);

        /* Find an approximation of the nadir point of the pareto front using the minimum of the extreme points. */
        static Point findNadirPoint(const std::vector<Point>& extreme_points);

        /* Map a fitness vector onto the unit simplex using the ideal and nadir points. */
        static FitnessVector normalize(const FitnessVector& fvec, const Point& ideal_point, const Point& nadir_point);

        /* Find the closest reference point to each candidate after normalization, and their distances. */
        void associatePopWithRefs(std::vector<CandidateInfo>& props, FitnessMatrix::const_iterator first, FitnessMatrix::const_iterator last, const std::vector<RefPoint>& refs);

        /* Return true if Pop[lhs] is better than Pop[rhs]. */
        bool nichedCompare(size_t lhs, size_t rhs) const noexcept;

        /* Return the niche counts of the ref points and assign niche counts to the candidates. */
        static void updateNicheCounts(std::vector<RefPoint>& refs, const std::vector<CandidateInfo>& props) noexcept;

        /* Return the niche counts of the given candidate. */
        size_t& nicheCountOf(const CandidateInfo& info) noexcept;
        size_t& nicheCountOf(size_t sol_idx) noexcept;
        const size_t& nicheCountOf(const CandidateInfo& info) const noexcept;
        const size_t& nicheCountOf(size_t sol_idx) const noexcept;
    };

} // namespace genetic_algorithm::algorithm

#endif // !GA_ALGORITHM_NSGA2_HPP