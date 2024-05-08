// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#pragma once
#include <range/v3/all.hpp>

#include <concepts>
#include <mdspan>
#include <type_traits>

namespace ppr {

template <typename T>
  requires std::is_arithmetic_v<T>
class grid final {
public:
  using extents_t = std::dextents<std::size_t, 2>;
  using mdspan_t = std::mdspan<T, extents_t, std::layout_right>;

private:
  std::vector<T> storage;
  mdspan_t matrix;

public:
  grid(std::size_t dimx, std::size_t dimt)
      : storage(dimx * dimt), matrix(storage.data(), dimx, dimt) {}

  grid(ranges::input_range auto &&range, std::size_t dimx, std::size_t dimt)
      : grid(dimx, dimt) {
    ranges::copy(range, storage.begin());
  }

  template <typename Self>
  auto &operator[](this Self &self, std::size_t x_idx, std::size_t t_idx) {
    return self.matrix[x_idx, t_idx];
  }

  auto dim_x() const { return matrix.extent(0); }

  auto dim_t() const { return matrix.extent(1); }

  template <typename Self> auto data(this Self &self) {
    return self.storage.data();
  }
};
} // namespace ppr
