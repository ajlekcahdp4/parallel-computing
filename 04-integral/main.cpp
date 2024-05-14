#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/vector.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <range/v3/all.hpp>

#include <cmath>
#include <concepts>

template <typename F, typename T>
concept integrable_function = requires(F f, T x) {
  { f(x) } -> std::convertible_to<T>;
};

template <std::floating_point T>
auto integrate_trapeze(T left, T right, T step) {
  return (left + right) * step / 2;
}

template <std::floating_point T, integrable_function<T> F>
auto integrate(F f, T start, T stop, std::size_t n) {
  auto len = stop - start;
  auto step = len / n;
  auto res = T{};
  for (auto x = start; x < stop; x += step) {
    auto half_step = step / 2;
    auto val_left = f(x);
    auto val_mid = f(x + half_step);
    auto val_right = f(x + step);
    auto whole = integrate_trapeze(val_left, val_right, step);
    auto left = integrate_trapeze(val_left, val_mid, half_step);
    auto right = integrate_trapeze(val_mid, val_right, half_step);
    res += (std::abs(whole - left - right) > 1e-3) ? left + right : whole;
  }
  return res;
}

auto main(int argc, char **argv) -> int {
  auto res = integrate([](double x) { return std::sin(x) / x; }, 1.0, 2.0, 100);
  fmt::println("{:.3f}", res);
}
