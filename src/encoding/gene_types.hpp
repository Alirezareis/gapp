/* Copyright (c) 2022 Kriszti�n Rug�si. Subject to the MIT License. */

#ifndef GA_ENCODING_GENE_TYPES_HPP
#define GA_ENCODING_GENE_TYPES_HPP

#include <cstddef>

namespace genetic_algorithm
{
    using BinaryGene      = int8_t;
    using RealGene        = double;
    using PermutationGene = size_t;
    using IntegerGene     = int64_t;

} // namespace genetic_algorithm

#endif // !GA_ENCODING_GENE_TYPES_HPP