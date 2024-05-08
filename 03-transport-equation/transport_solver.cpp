// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include "transport_equation_solver.hpp"

#include <boost/mpi.hpp>

namespace ppr {

namespace mpi = boost::mpi;

template <typename T>
  requires std::is_arithmetic_v<T>
void explicit_left_corner_impl(grid<T> &data, const transport_equation<T> &eq,
                               std::size_t k, std::size_t m, T upper) {
  auto h = eq.get_h();
  auto tau = eq.get_tau();
  data[m, k + 1] = eq.rhs(m * h, k * tau) * tau + upper * tau / h -
                   data[m, k] * (tau / h - 1);
}
template <typename T>
  requires std::is_arithmetic_v<T>
void explicit_left_corner(grid<T> &data, const transport_equation<T> &eq,
                          std::size_t k, std::size_t m) {
  explicit_left_corner_impl(data, eq, k, m, data[m - 1, k]);
}

template <typename T>
  requires std::is_arithmetic_v<T>
void explicit_cross_impl(grid<T> &data, const transport_equation<T> &eq,
                         std::size_t k, std::size_t m, T upper) {
  auto h = eq.get_h();
  auto tau = eq.get_tau();
  data[m, k + 1] = 2 * tau * eq.rhs(m * h, k * tau) -
                   (data[m + 1, k] - upper) * tau / h + data[m, k - 1];
}
template <typename T>
  requires std::is_arithmetic_v<T>
void explicit_cross(grid<T> &data, const transport_equation<T> &eq,
                    std::size_t k, std::size_t m) {
  explicit_cross_impl(data, eq, k, m, data[m - 1, k]);
}

template <typename T>
  requires std::is_arithmetic_v<T>
void init_grid(grid<T> &data, const transport_equation<T> &eq,
               std::size_t start_x) {
  if (start_x == 0)
    for (auto t = 0uz; t < data.dim_t(); ++t)
      data[0, t] = eq.psi(t * eq.get_tau());

  for (auto x = 0uz; x < data.dim_x(); ++x)
    data[x, 0] = eq.phi((x + start_x) * eq.get_h());
}

grid<double> solve_sequential(const transport_equation<double> &eq) {
  grid<double> data(eq.max_x() / eq.get_h(), eq.max_t() / eq.get_tau());
  init_grid(data, eq, 0);
  for (auto m = 1uz; m < data.dim_x(); ++m)
    explicit_left_corner(data, eq, 0, m);

  for (auto k = 1uz; k < data.dim_t() - 1; ++k) {
    for (auto m = 1uz; m < data.dim_x() - 1; ++m)
      explicit_cross(data, eq, k, m);
    explicit_left_corner(data, eq, k, data.dim_x() - 1);
  }

  return data;
}

grid<double> solve_parallel(const transport_equation<double> &eq,
                            const mpi::communicator &world) {
  auto dim_x = eq.max_x() / eq.get_h();
  auto dim_t = eq.max_t() / eq.get_tau();
  auto lines_per_proc = dim_x / world.size();
  auto start_x = world.rank() * lines_per_proc;
  grid<double> data(lines_per_proc, dim_t);
  init_grid(data, eq, start_x);
  bool should_recv = world.rank() != 0;
  bool should_send = world.rank() != world.size() - 1;
  auto start_idx = should_recv ? 0uz : 1uz;
  auto recieve = [&](auto t) {
    auto val = 0.0;
    world.recv(world.rank() - 1, t, val);
    return val;
  };

  auto send = [&](double data, auto t) {
    world.send(world.rank() + 1, t, data);
  };

  // init second line
  if (should_send) {
    send(data[data.dim_x() - 1, 0], 0);
  }
  for (auto m = start_idx; m < data.dim_x(); ++m) {
    auto upper = (m == 0) ? recieve(0) : data[m - 1, 0];
    explicit_left_corner_impl(data, eq, 0, m, upper);
  }

  for (auto k = 1uz; k < data.dim_t() - 1; ++k) {
    if (should_send) {
      send(data[data.dim_x() - 1, k], k);
    }
    for (auto m = start_idx; m < data.dim_x() - 1; ++m) {
      auto upper = (m == 0) ? recieve(k) : data[m - 1, k];
      explicit_cross_impl(data, eq, k, m, upper);
    }
    explicit_left_corner(data, eq, k, data.dim_x() - 1);
  }
  std::vector<double> gathered;
  gathered.reserve(world.size() * data.dim_x() * data.dim_t());
  mpi::gather(world, data.data(), data.dim_x() * data.dim_t(), gathered, 0);
  if (world.rank() == 0) {
    grid<double> whole(gathered, dim_x, data.dim_t());
    return whole;
  }
  return data;
}

} // namespace ppr
