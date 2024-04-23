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


#include <bit>
#include <cmath>
#include <concepts>
#include <random>
#include <span>
#include <mdspan>

namespace mpi = boost::mpi;
namespace {
static constexpr int root_communicator_rank = 0;

static constexpr double max_x = 1.0;
static constexpr double max_t = 1.0;
static constexpr std::size_t dim_x = 2;
static constexpr std::size_t dim_t = dim_x;
static constexpr double h = max_x / dim_x;
static constexpr double tau = max_t / dim_t;

using extents_t = std::extents<std::size_t, dim_x, dim_t>;
template <typename T>
using right_major_mdspan = std::mdspan<T, extents_t>;

template <std::floating_point T> auto f(T x, T t) { return x + t; }

auto psi(std::floating_point auto x) { return std::cos(x); }

auto phi(std::floating_point auto x) { return std::exp(x); }

template <typename T> auto get_initial() -> std::vector<T> {
  std::vector<T> data(dim_x * dim_t);
  auto matrix = right_major_mdspan<T>(data.data());
  for (unsigned x = 0; x < matrix.extent(0); ++x)
    matrix[x, 0] = phi(x * h);
  for (unsigned t = 0; t < matrix.extent(1); ++t)
    matrix[0, t] = psi(t * tau);
  return data;
}

template <typename T>
std::vector<T> solve_parallel(right_major_mdspan<T> matrix,
                              const mpi::communicator &world) {
  auto rank = world.rank();
  auto world_size = world.size();

  if (rank == root_communicator_rank) {
    auto dst = (rank+1)%world_size;
    world.send(dst, 0, dst);
  }
  if (rank == world_size - 1) {
    assert(rank);
    auto src = rank - 1;
    world.recv(src, 0, src);
  }
  for (unsigned t = rank; t < matrix.extent(1); t += world_size) {
    for (unsigned x = 1; x < matrix.extent(0); ++x) {
      if (rank != root_communicator_rank) {
        auto src = rank ? rank - 1 : world_size - 1;
        fmt::println("RECV: rank {} from {} to {} tag = {} t = {}, x = {}", rank, src, rank , x, t, x);
        world.recv(src, /* tag */ 0, matrix[x, t - 1]);
        matrix[x, t] = matrix[x, t - 1] -
                       (tau / h) * (matrix[x, t - 1] - matrix[x - 1, t - 1]) +
                       tau * f(x * h, (t - 1) * tau);
      }
      if (rank != world_size - 1) {
        auto dst = (rank + 1) % world_size;
        fmt::println("SEND: rank {} from {} to {} tag = {} t = {}, x = {}", rank, rank, dst, x, t, x);
        world.send(dst, /* tag */ 0, matrix[x, t]);
      }
    }
  }
  world.barrier();
}

template<std::floating_point T>
void print_matrix(std::span<T> data, unsigned dim1, unsigned dim2) {
  fmt::println ("---");
  for (unsigned i = 0; i < dim1; ++i) {
    auto start = std::next(data.begin(), i * dim2);
    fmt::print("|{:4.2}|\n",
               fmt::join(std::span(start, std::next(start, dim2)), " "));
  }
  fmt::println("...");
}
} // namespace

namespace po = boost::program_options;
auto main(int argc, char **argv) -> int {
  mpi::environment env(argc, argv);
  mpi::communicator world;
  auto initial_conditions = get_initial<double>();
  if (world.rank() == root_communicator_rank)
    print_matrix(std::span(initial_conditions), dim_x, dim_t);
  solve_parallel<double>(right_major_mdspan<double>(initial_conditions.data()), world);
  if (world.rank() == root_communicator_rank)
    print_matrix(std::span(initial_conditions), dim_x, dim_t);

}
