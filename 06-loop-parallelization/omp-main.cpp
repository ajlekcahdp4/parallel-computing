#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/vector.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <omp.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <mdspan>

namespace {

using extents_t = std::dextents<std::size_t, 2>;
using mdspan = std::mdspan<double, extents_t, std::layout_right>;

double do_parallel(mdspan amat, mdspan bmat, unsigned n_threads, unsigned isize,
                   unsigned jsize) {
  omp_set_dynamic(0);
  omp_set_num_threads(n_threads);
  auto start = omp_get_wtime();
  auto num_threads = omp_get_num_threads();
  auto local_rows = isize / num_threads;
  auto rank = omp_get_thread_num();
  auto local_start = rank * local_rows;
  auto local_end = local_start + local_rows - 4;
  if (rank == num_threads - 1)
    local_rows = isize - local_start;
#pragma omp parallel for schedule(static, 8)
  for (auto i = local_start; i < local_end; ++i) {
    for (auto j = 1u; j < jsize; ++j) {
      bmat[i, j] = 1.5 * std::sin(0.002 * amat[i + 4, j - 1]);
    }
  }
  auto finish = omp_get_wtime();
  return finish - start;
}

double do_sequential(mdspan amat, mdspan bmat, unsigned isize, unsigned jsize) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  auto start = high_resolution_clock::now();
  for (auto i = 0u; i < isize - 4; ++i)
    for (auto j = 1u; j < jsize; ++j)
      bmat[i, j] = 1.5 * std::sin(0.002 * amat[i + 4, j - 1]);
  auto finish = high_resolution_clock::now();
  duration<double, std::milli> ms_double = finish - start;
  return ms_double.count();
}
} // namespace

namespace po = boost::program_options;
auto main(int argc, const char **argv) -> int {
  po::options_description desc("Allowed options");
  bool parallel = false;
  unsigned n_threads = 1;
  unsigned isize = 1000;
  unsigned jsize = 1000;
  desc.add_options()("parallel", po::value(&parallel), "Use MPI")(
      "num-threads,n", po::value(&n_threads), "Number of threads to use");
  desc.add_options()("dump-a", "Dump array a");
  desc.add_options()("dump-b", "Dump array b");
  desc.add_options()("isize", po::value(&isize), "isize");
  desc.add_options()("jsize", po::value(&jsize), "jsize");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_SUCCESS;
  }
  std::vector<double> adata(isize * jsize, 0.0);
  std::vector<double> bdata(isize * jsize, 0.0);
  mdspan amat(adata.data(), isize, jsize);
  mdspan bmat(bdata.data(), isize, jsize);
  for (auto i = 0u; i < isize; ++i)
    for (auto j = 0u; j < jsize; ++j)
      amat[i, j] = 10 * i + j;

  auto elapsed = [&] {
    if (!parallel)
      return do_sequential(amat, bmat, isize, jsize);
    return do_parallel(amat, bmat, n_threads, isize, jsize);
  }();
  fmt::println("elapsed time: {:.4f} ms", elapsed);
  if (vm.count("dump-a"))
    fmt::println("{}", fmt::join(adata, "\n"));
  if (vm.count("dump-b"))
    fmt::println("{}", fmt::join(bdata, "\n"));
}
