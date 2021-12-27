#include "math.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cassert>

namespace genetic_algorithm::detail
{
    bool floatIsEqual(double lhs, double rhs, double eps)
    {
        assert(0.0 <= eps && eps <= 1.0);

        if (lhs == 0.0 || rhs == 0.0)
        {
            return std::abs(lhs - rhs) <= eps;
        }
        else
        {
            return std::abs(lhs - rhs) <= std::max(std::abs(lhs), std::abs(rhs)) * eps;
        }
    }

    bool floatIsLess(double lhs, double rhs, double eps)
    {
        assert(0.0 <= eps && eps <= 1.0);

        return (rhs - lhs) > std::max(std::abs(lhs), std::abs(rhs)) * eps;
    }

    bool floatIsEqual(const std::vector<double>& lhs, const std::vector<double>& rhs, double eps)
    {
        assert(0.0 <= eps && eps <= 1.0);
        assert(lhs.size() == rhs.size());

        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), [eps](double lhs, double rhs) { return floatIsEqual(lhs, rhs, eps); });
    }

    bool paretoCompareLess(const std::vector<double>& lhs, const std::vector<double>& rhs, double eps)
    {
        assert(0.0 <= eps && eps <= 1.0);
        assert(lhs.size() == rhs.size());

        bool has_lower = false;
        for (size_t i = 0; i < lhs.size(); i++)
        {
            if (floatIsLess(rhs[i], lhs[i], eps)) return false;
            if (floatIsLess(lhs[i], rhs[i], eps)) has_lower = true;
        }

        return has_lower;
    }

    double euclideanDistanceSq(const std::vector<double>& v1, const std::vector<double>& v2)
    {
        assert(v1.size() == v2.size());

        double dist = 0.0;
        for (size_t i = 0; i < v1.size(); i++)
        {
            dist += (v1[i] - v2[i]) * (v1[i] - v2[i]);
        }

        return dist;
    }

    double perpendicularDistanceSq(const std::vector<double>& line, const std::vector<double>& point)
    {
        assert(line.size() == point.size());
        assert(!line.empty());

        double num = 0.0, den = 0.0;
        for (size_t i = 0; i < line.size(); i++)
        {
            num += line[i] * point[i];
            den += line[i] * line[i];
        }
        double k = num / den;

        double dist = 0.0;
        for (size_t i = 0; i < line.size(); i++)
        {
            dist += (point[i] - k * line[i]) * (point[i] - k * line[i]);
        }

        return dist;
    }
}