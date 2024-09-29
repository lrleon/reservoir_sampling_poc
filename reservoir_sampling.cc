//
// Created by lrleon on 9/28/24.
//

# include <gsl/gsl_rng.h>

# include <limits>
#include <iostream>
#include <vector>
#include <algorithm>
# include <algorithm>
# include <chrono>

# include <tpl_dynList.H>
# include <ah-string-utils.H>
# include <ahSort.H>

# include <tclap/CmdLine.h>

using namespace std;
using namespace TCLAP;
using namespace Aleph;
using namespace chrono;

CmdLine cmd("Reservoir Sampling", ' ', "0.1");
ValueArg<int> reservoir_size_arg("r", "reservoir_size", "Reservoir size", false, 213, "int");
ValueArg<unsigned int> seed_arg("s", "seed", "Seed", false, 0, "unsigned int");
ValueArg<int> min_arg("m", "min", "Min value", false, 0, "int");
ValueArg<int> max_arg("M", "max", "Max value", false, 128, "int");
ValueArg<int> NumSamples("n", "num_samples", "Number of samples", false, 100000, "int");

class VectorReservoirSampling {
  Array<int> reservoir;
  const size_t reservoir_size = 0;
  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
  int min_value = numeric_limits<int>::max();
  int max_value = numeric_limits<int>::min();

public:

  VectorReservoirSampling(const int reservoir_size, unsigned int seed = 0)
      : reservoir_size(
      reservoir_size % 2 ? reservoir_size + 1 : reservoir_size), reservoir(reservoir_size) {
    gsl_rng_set(this->rng, seed);
    reservoir.reserve(reservoir_size);
  }

  VectorReservoirSampling() {
    gsl_rng_free(this->rng);
  }

  void add_sample(int value) {
    min_value = ::min(min_value, value);
    max_value = ::max(max_value, value);
    const size_t &num_samples = reservoir.size();
    if (num_samples < reservoir_size) {
      reservoir.append(value);
      return;
    }

    const unsigned long random_index = gsl_rng_uniform_int(rng, reservoir_size + 1);
    if (random_index < reservoir_size)
      reservoir[random_index] = value;
  }

  int min() const { return min_value; }

  int max() const { return max_value; }

  int median()  {
    quicksort_op(reservoir);
    //insertion_sort(&reservoir.base(), 0, reservoir.size());
    //quicksort_insertion(&reservoir.base(), 0, reservoir.size());
    //heapsort(&reservoir.base(), 0, reservoir.size());
    //ranges::sort(sorted_reservoir);

    return reservoir[reservoir_size / 2];
  }
};

// Return the min and max value among 100k random numbers. Use reservoir sampling to select 100k random numbers.
// The reservoir sampling algorithm is used to select k random numbers from a stream of numbers of unknown lengths.
template<typename ReservoirType>
tuple<int, int, int>
median_reservoir_sampling(int min, int max, const int reservoir_size, unsigned int seed) {
  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, seed);

  int min_value = numeric_limits<int>::max();
  int max_value = numeric_limits<int>::min();

  ReservoirType reservoir_sampling(reservoir_size, seed);

  for (int i = 0; i < NumSamples; ++i) {
    const int random_number = gsl_rng_uniform_int(rng, max - min) + min;
    reservoir_sampling.add_sample(random_number);
    min_value = std::min(min_value, random_number);
    max_value = std::max(max_value, random_number);
  }

  gsl_rng_free(rng);

  return {min_value, reservoir_sampling.median(), max_value};
}

// return the min and max value among 100k random numbers
tuple<int, int, int> median_classic(int min, int max, unsigned int seed) {
  vector<int> random_numbers;
  random_numbers.reserve(100001);
  int min_value = numeric_limits<int>::max();
  int max_value = numeric_limits<int>::min();

  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, seed);

  for (int i = 0; i < NumSamples; ++i) {
    const int random_number = gsl_rng_uniform_int(rng, max - min) + min;
    random_numbers.push_back(random_number);
    min_value = std::min(min_value, random_number);
    max_value = std::max(max_value, random_number);
  }

  gsl_rng_free(rng);

  ranges::sort(random_numbers);

  return {min_value, random_numbers[random_numbers.size() / 2], max_value};
}

int main(int argc, char **argv) {
  cmd.add(reservoir_size_arg);
  cmd.add(seed_arg);
  cmd.add(min_arg);
  cmd.add(max_arg);
  cmd.parse(argc, argv);

  // durations in microseconds
  microseconds classic_duration;
  microseconds reservoir_duration;
  tuple<int, int, int> classic_result;
  tuple<int, int, int> reservoir_result;

  {
    auto start = high_resolution_clock::now();
    classic_result = median_classic(0, max_arg, seed_arg);
    auto end = high_resolution_clock::now();
    classic_duration = duration_cast<microseconds>(end - start);
  }

  {
    auto start = high_resolution_clock::now();
    reservoir_result = median_reservoir_sampling<VectorReservoirSampling>(min_arg,
                                                                          max_arg,
                                                                          reservoir_size_arg,
                                                                          seed_arg);
    auto end = high_resolution_clock::now();
    reservoir_duration = duration_cast<microseconds>(end - start);
  }
//
//  DynList<DynList<string>> table = {
//      {"Method ", "Min ", "Median ", "Max ", " Time in us ", " Samples ser size ", " Seed"},
//      {
//          "Classic", to_string(get<0>(classic_result)),
//          to_string(get<1>(classic_result)),
//          to_string(get<2>(classic_result)),
//          to_string(classic_duration.count()),
//          to_string(NumSamples),
//          to_string(seed_arg)
//      },
//      {
//          "Reservoir", to_string(get<0>(reservoir_result)),
//          to_string(get<1>(reservoir_result)),
//          to_string(get<2>(reservoir_result)),
//          to_string(reservoir_duration.count()),
//          to_string(reservoir_size_arg),
//          to_string(seed_arg)
//      }
//  };
//
//  cout << to_string(Aleph::format_string(table)) << endl;

  cout << to_string(Aleph::format_string(
      {
      {"Method ", "Min ", "Median ", "Max ", " Time in us ", " Samples ser size ", " Seed"},
      {
          "Classic", to_string(get<0>(classic_result)),
          to_string(get<1>(classic_result)),
          to_string(get<2>(classic_result)),
          to_string(classic_duration.count()),
          to_string(NumSamples),
          to_string(seed_arg)
      },
      {
          "Reservoir", to_string(get<0>(reservoir_result)),
          to_string(get<1>(reservoir_result)),
          to_string(get<2>(reservoir_result)),
          to_string(reservoir_duration.count()),
          to_string(reservoir_size_arg),
          to_string(seed_arg)
      }
})) << endl;
}