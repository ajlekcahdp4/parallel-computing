// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include <boost/mpi.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/vector.hpp>

#include <complex>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <range/v3/all.hpp>

#include <mdspan/mdspan.hpp>

#include <bit>
#include <cmath>
#include <concepts>
#include <random>
#include <span>

namespace {
static constexpr int root_communicator_rank = 0;

static constexpr double max_x = 1.0;
static constexpr double max_t = 1.0;
static constexpr size_t dim_x = 10;
static constexpr size_t dim_t = 15;
static constexpr double h = max_x / dim_x;
static constexpr double tau = max_t / dim_t;

using extent_t = Kokkos::extents<unsigned, dim_x, dim_t>;
template <typename T> using mdspan = Kokkos::mdspan<T, extent_t>;

template <std::floating_point T> auto f(T x, T t) { return x + t; }

auto psi(std::floating_point auto x) { return std::cos(x); }

auto phi(std::floating_point auto x) { return std::exp(x); }

template <typename T> auto get_initial() -> std::vector<T> {
  std::vector<T> data(dim_x * dim_t);
  mdspan<T> matrix(data.data());
  for (unsigned x = 0; x < matrix.extent(0); ++x)
    matrix[x, 0] = phi(x * h);
  for (unsigned t = 0; t < matrix.extent(1); ++t)
    matrix[0, t] = psi(t * tau);
  return data;
}

template <typename T> std::vector<T> solve_parallel() {}
} // namespace

namespace po = boost::program_options;
namespace mpi = boost::mpi;
auto main(int argc, char **argv) -> int {
  mpi::environment env(argc, argv);
  mpi::communicator world;
  auto initial_conditions = get_initial<double>();
  for (unsigned i = 0; i < dim_x; ++i) {
    auto start = std::next(initial_conditions.begin(), i * dim_t);
    fmt::print("{:4.2}\n",
               fmt::join(std::span(start, std::next(start, dim_t)), ", "));
  }
}
