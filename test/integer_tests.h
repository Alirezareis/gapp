/* Benchmark/test functions for the IntegerGA. */

#ifndef INTEGER_TEST_H
#define INTEGER_TEST_H

#include <vector>
#include <chrono>
#include <iostream>

#include "../src/algorithms/integer_ga.hpp"
#include "../src/selection/selection.hpp"
#include "../src/crossover/integer.hpp"
#include "../src/mutation/integer.hpp"
#include "fitness_functions.h"
#include "utils.h"

using namespace std;
using namespace genetic_algorithm;

void integerTest1()
{
    /* Init GA. */
    MatchString match("HELLO WORLD!");

    IntegerGA GA(match.num_vars(), match, 96);

    /* Set some optional parameters. */
    GA.population_size(100);
    GA.selection_method(selection::single_objective::Tournament{});
    GA.crossover_method(crossover::integer::TwoPoint{ 0.85 });
    GA.mutation_method(mutation::integer::Uniform{ 96, 0.01 });

    /* Run the GA with a timer. */
    auto tbegin = chrono::high_resolution_clock::now();
    auto sols = GA.run(500);
    auto tend = chrono::high_resolution_clock::now();

    auto duration = chrono::duration_cast<chrono::microseconds>(tend - tbegin).count();
    double time_spent = (double)duration / 1E+6;

    /* Print the results. */
    cout << "\n\nThe best strings found are (expected: " << match.optimal_x() << "): \n";
    for (const auto& sol : sols)
    {
        for (const auto& gene : sol.chromosome)
        {
            cout << char(gene + 32);
        }
        cout << "\n";
    }
    cout << "Fitness value: " << sols[0].fitness[0] << " (best is " << match.optimal_value() << ")\n";
    cout << "Number of fitness evals: " << GA.num_fitness_evals() << "\n";
    cout << "Time taken: " << time_spent << " s\n\n";
}

void integerTest2()
{
    /* Init GA. */
    MatchString match("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque gravida ut ipsum at tincidunt.");

    IntegerGA GA(match.num_vars(), match, 96);

    /* Set some optional parameters. */
    GA.population_size(250);
    GA.selection_method(selection::single_objective::Boltzmann{});
    GA.crossover_method(crossover::integer::Uniform{ 0.8 });
    GA.mutation_method(mutation::integer::Uniform{ 96, 5.0 / 250 });

    /* Run the GA with a timer. */
    auto tbegin = chrono::high_resolution_clock::now();
    auto sols = GA.run(1000);
    auto tend = chrono::high_resolution_clock::now();

    auto duration = chrono::duration_cast<chrono::microseconds>(tend - tbegin).count();
    double time_spent = (double)duration / 1E+6;

    /* Print the results. */
    cout << "\n\nThe best strings found are (expected: " << match.optimal_x() << "): \n";
    for (const auto& sol : sols)
    {
        for (const auto& gene : sol.chromosome)
        {
            cout << char(gene + 32);
        }
        cout << "\n";
    }
    cout << "Fitness value: " << sols[0].fitness[0] << " (best is " << match.optimal_value() << ")\n";
    cout << "Number of fitness evals: " << GA.num_fitness_evals() << "\n";
    cout << "Time taken: " << time_spent << " s\n\n";
}

#endif // !INTEGER_TEST_H