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

# include <tclap/CmdLine.h>

# include <tpl_dynSetTree.H>

using namespace std;
using namespace Aleph;
using namespace TCLAP;

class ReservoirSampling
{
  DynSetTreapRk<int> reservoir;
  int reservoir_size;
  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);

 public:

  ReservoirSampling(int reservoir_size, unsigned int seed = 0) : reservoir_size(reservoir_size)
  {
    gsl_rng_set(this->rng, seed);
  }

  ~ReservoirSampling()
  {
    gsl_rng_free(this->rng);
  }

  void add_sample(int value)
  {
    const size_t &num_samples = reservoir.size();
    if (num_samples < reservoir_size)
      {
        reservoir.insert_dup(value);
        return;
      }

    const unsigned long random_index = gsl_rng_uniform_int(rng, num_samples);
    if (random_index < reservoir_size)
      {
        reservoir.remove_pos(random_index);
        reservoir.insert(value);
      }
  }

  int min() const
  {
    return reservoir.min();
  }

  int max() const
  {
    return reservoir.max();
  }

  int median() const
  {
    if (reservoir_size % 2 == 1)
      return reservoir.select(reservoir_size / 2);

    return (reservoir.select(reservoir_size / 2) + reservoir.select(reservoir_size / 2 - 1)) / 2;
  }
};

class VectorReservoirSampling
{
  vector<int> reservoir;
  const size_t reservoir_size = 0;
  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
  int min_value = numeric_limits<int>::max();
  int max_value = numeric_limits<int>::min();

 public:

  VectorReservoirSampling(const int reservoir_size, unsigned int seed = 0)
      : reservoir_size(reservoir_size), reservoir(reservoir_size)
  {
    gsl_rng_set(this->rng, seed);
  }

  VectorReservoirSampling()
  {
    gsl_rng_free(this->rng);
  }

  void add_sample(int value)
  {
    const size_t &num_samples = reservoir.size();
    if (num_samples < reservoir_size)
      {
        reservoir.push_back(value);
        min_value = ::min(min_value, value);
        max_value = ::max(max_value, value);
        return;
      }

    const unsigned long random_index = gsl_rng_uniform_int(rng, num_samples);
    if (random_index < reservoir_size)
      {
        reservoir[random_index] = value;
        min_value = ::min(min_value, value);
        max_value = ::max(max_value, value);
      }
  }

  int min() const
  {
    return min_value;
  }

  int max() const
  {
    return max_value;
  }

  int median() const
  {
    vector<int> sorted_reservoir = reservoir;
    ranges::sort(sorted_reservoir);
    if (reservoir_size % 2 == 1)
      return sorted_reservoir[reservoir_size / 2];

    return (sorted_reservoir[reservoir_size / 2] + sorted_reservoir[reservoir_size / 2 - 1]) / 2;
  }
};

// Return the min and max value among 100k random numbers. Use reservoir sampling to select 100k random numbers.
// The reservoir sampling algorithm is used to select k random numbers from a stream of numbers of unknown lengths.
template<typename ReservoirType>
tuple<int, int, int> median_reservoir_sampling(int min, int max, const int reservoir_size, unsigned int seed)
{
  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, seed);

  int min_value = numeric_limits<int>::max();
  int max_value = numeric_limits<int>::min();

  const auto start = chrono::high_resolution_clock::now();
  ReservoirType reservoir_sampling(reservoir_size, seed);

  for (int i = 0; i < 100000; ++i)
    {
      const int random_number = gsl_rng_uniform_int(rng, max - min) + min;
      reservoir_sampling.add_sample(random_number);
      min_value = std::min(min_value, random_number);
      max_value = std::max(max_value, random_number);
    }

  gsl_rng_free(rng);

  const auto end = chrono::high_resolution_clock::now();
  cout << "Time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms\n";

  return {min_value, reservoir_sampling.median(), max_value};
}

// return the min and max value among 100k random numbers
tuple<int, int, int> median_classic(int min, int max, unsigned int seed)
{
  vector<int> random_numbers;
  random_numbers.reserve(100001);
  int min_value = numeric_limits<int>::max();
  int max_value = numeric_limits<int>::min();

  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
  gsl_rng_set(rng, seed);

  const auto start = chrono::high_resolution_clock::now();
  for (int i = 0; i < 100000; ++i)
    {
      const int random_number = gsl_rng_uniform_int(rng, max - min) + min;
      random_numbers.push_back(random_number);
      min_value = std::min(min_value, random_number);
      max_value = std::max(max_value, random_number);
    }

  gsl_rng_free(rng);

  ranges::sort(random_numbers);
  const auto end = chrono::high_resolution_clock::now();
  cout << "Time: " << chrono::duration_cast<chrono::milliseconds>(end - start).count() << "ms\n";

  return {min_value, random_numbers[random_numbers.size() / 2], max_value};
}

CmdLine cmd("Reservoir Sampling", ' ', "0.1");
ValueArg<int> reservoir_size_arg("r", "reservoir_size", "Reservoir size", false, 619, "int");
ValueArg<unsigned int> seed_arg("s", "seed", "Seed", false, 0, "unsigned int");
ValueArg<int> min_arg("m", "min", "Min value", false, 0, "int");
ValueArg<int> max_arg("M", "max", "Max value", false, 127, "int");

int main(int argc, char **argv)
{
  cmd.add(reservoir_size_arg);
  cmd.add(seed_arg);
  cmd.add(min_arg);
  cmd.add(max_arg);
  cmd.parse(argc, argv);

  {
    cout << "Using classic algorithm" << endl << endl;
    const auto [min_value, median_value, max_value] = median_classic(0, max_arg, seed_arg);
    cout << min_value << " " << median_value << " " << max_value << endl << "====================" << endl << endl;
  }

  {
    cout << "Using reservoir sampling algorithm with vector" << endl << endl;
    const auto [min_value, median_value, max_value] = median_reservoir_sampling<VectorReservoirSampling>(min_arg, max_arg, reservoir_size_arg, seed_arg);
    cout << min_value << " " << median_value << " " << max_value << endl << "====================" << endl << endl;
  }

  {
    cout << "Using reservoir sampling algorithm with ranked treaps" << endl << endl;
    const auto [min_value, median_value, max_value] = median_reservoir_sampling<ReservoirSampling>(min_arg, max_arg, reservoir_size_arg, seed_arg);
    cout << min_value << " " << median_value << " " << max_value << endl << "====================" << endl << endl;
  }

}
