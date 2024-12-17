#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/mpi.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <range/v3/all.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <mdspan>

namespace mpi = boost::mpi;
namespace {

static constexpr unsigned isize = 1000;
static constexpr unsigned jsize = 1000;

using extents_t = std::dextents<std::size_t, 2>;
using mdspan = std::mdspan<double, extents_t, std::layout_right>;

double do_parallel(mdspan matrix, mpi::communicator &world) {
  auto size = world.size();
  auto rank = world.rank();
  auto jstep = jsize / size;
  auto jstart = jstep * rank;
  auto jfinish = jstep * (rank + 1);
  // handle remainder
  if (jsize % size) {
    if (rank < jsize % size) {
      jstart += rank;
      jfinish += rank + 1;
    } else {
      jstart += jsize % size;
      jfinish += jsize % size;
    }
  }
  if (rank == size - 1)
    jfinish = jsize - 3;

  for (auto i = 0u; i < isize; ++i) {
    for (auto j = 0; j < jsize; ++j) {
      matrix[i, j] = 10 * i + j;
    }
  }
  std::vector<double> local_array(jfinish - jstart, 0.0);
  std::vector<int> recieved(size, 0);
  std::vector<int> displs(size, 0);
  for (auto i = 0u; i < size; ++i) {
    auto loc_start = jstep * i;
    auto loc_end = jstep * (i + 1);
    if (jsize % size) {
      if (rank < jsize % size) {
        loc_start += i;
        loc_end += i + 1;
      } else {
        loc_start += jsize % size;
        loc_end += jsize % size;
      }
    }
    if (i == size - 1)
      loc_end = jsize - 3;
    recieved[i] = loc_end - loc_start;
    displs[i] = (i == 0) ? 0 : displs[i - 1] + recieved[i - 1];
  }
  world.barrier();

  auto timer = mpi::timer();
  for (auto i = 8u; i < isize; ++i) {
    for (auto j = jstart; j < jfinish; ++j)
      local_array[j - jstart] = sin(4 * matrix[i - 8, j + 3]);
    // Unfortunately boost::mpi does not have all_gatherv.
    // Falling back to C-API
    MPI_Allgatherv(local_array.data(), local_array.size(), MPI_DOUBLE,
                   matrix.data_handle() + i * matrix.stride(0), recieved.data(),
                   displs.data(), MPI_DOUBLE, world);
  }
  world.barrier();
  return timer.elapsed();
}

double do_sequential(mdspan matrix) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  for (auto i = 0u; i < isize; i++) {
    for (auto j = 0u; j < jsize; j++) {
      matrix[i, j] = 10 * i + j;
    }
  }
  auto start = high_resolution_clock::now();
  for (int i = 8; i < isize; i++) {
    for (int j = 0; j < jsize - 3; j++) {
      matrix[i, j] = sin(4 * matrix[i - 8, j + 3]);
    }
  }
  auto finish = high_resolution_clock::now();
  duration<double, std::milli> ms_double = finish - start;
  return ms_double.count();
}
} // namespace

namespace po = boost::program_options;
auto main(int argc, const char **argv) -> int {

  po::options_description desc("Allowed options");
  bool parallel = false;
  desc.add_options()("parallel", po::value(&parallel), "Use MPI");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_SUCCESS;
  }
  std::array<double, isize * jsize> data;
  mdspan matrix(data.data(), isize, jsize);

  mpi::environment env;
  mpi::communicator world;
  auto elapsed = [&] {
    if (!parallel)
      return do_sequential(matrix);
    return do_parallel(matrix, world);
  }();
  if (world.rank() == 0) {
    // fmt::println("{}", fmt::join(data, ", "));
    fmt::println("elapsed time: {:.4f} ms", elapsed);
  }
}
