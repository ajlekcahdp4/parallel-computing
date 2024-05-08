// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include "grid.hpp"
#include "transport_equation.hpp"
#include "transport_equation_solver.hpp"

#include <boost/mpi.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/vector.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <range/v3/all.hpp>

#include <chrono>
#include <cmath>
#include <numbers>

namespace mpi = boost::mpi;
namespace {
static constexpr int root_communicator_rank = 0;

template <std::floating_point T> auto rhs(T x, T t) { return x + t; }

auto psi(std::floating_point auto x) { return std::cos(x * std::numbers::pi); }

auto phi(std::floating_point auto x) { return std::exp(-x); }

constexpr static double max_x = 1.0;
constexpr static double max_t = 1.0;

template <typename T>
  requires std::is_arithmetic_v<T>
void print_grid_simple(ppr::grid<T> data) {
  for (auto m = 0uz; m < data.dim_x(); ++m) {
    auto time_slice =
        ranges::views::iota(0uz, data.dim_t()) |
        ranges::views::transform([&](auto k) { return data[m, k]; });
    fmt::println("{:6.3f}", fmt::join(time_slice, " "));
  }
  fmt::println("");
}

} // namespace

namespace po = boost::program_options;
auto main(int argc, char **argv) -> int {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  po::options_description desc("Allowed options");
  bool parallel = false;
  std::size_t dim_x = 100;
  bool should_print = false, measure = false;
  desc.add_options()("help", "Show this message")(
      "grid-size", po::value(&dim_x), "Size of the computational grid")(
      "parallel", po::value(&parallel), "Perform parallel computations")(
      "print", po::value(&should_print), "Print result matrix")(
      "measure", po::value(&measure), "Measure performance");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  auto eq = ppr::transport_equation<double>(rhs<double>, phi<double>,
                                            psi<double>, max_x, max_t, dim_x);

  mpi::environment env;
  mpi::communicator world;
  auto start = high_resolution_clock::now();
  auto grid = [&] {
    if (!parallel)
      return ppr::solve_sequential(eq);
    return ppr::solve_parallel(eq, world);
  }();
  auto finish = high_resolution_clock::now();
  if (world.rank() == root_communicator_rank && measure) {
    duration<double, std::milli> ms_double = finish - start;
    fmt::println("elapsed time: {}ms", ms_double.count());
  }
  if (world.rank() == root_communicator_rank && should_print)
    print_grid_simple(grid);
}
