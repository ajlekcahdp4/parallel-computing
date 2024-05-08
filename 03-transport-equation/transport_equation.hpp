// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#pragma once

namespace ppr {

// equation:
// du/dt + du/dx = f(t, x), where u = u(t, x), x in (0; X), t in (0; T),
// a in R.
// u(0, x) = phi(x), x in [0; X] u(t, 0) = psi(t), t in [0; T]

template <typename T>
  requires std::is_arithmetic_v<T>
class transport_equation final {
  std::function<T(T, T)> rhs_func;
  std::function<T(T)> phi_func;
  std::function<T(T)> psi_func;
  T max_x_val;
  T max_t_val;
  T h;
  T tau;

public:
  transport_equation(std::function<T(T, T)> rhs, std::function<T(T)> phi,
                     std::function<T(T)> psi, T max_x, T max_t, T grid_size)
      : rhs_func(std::move(rhs)), phi_func(std::move(phi)),
        psi_func(std::move(psi)), max_x_val(max_x), max_t_val(max_t),
        h(max_x / grid_size), tau(max_t / grid_size) {}

  template <typename Self> auto rhs(this Self &self, T x, T t) {
    return self.rhs_func(x, t);
  }

  template <typename Self> auto psi(this Self &self, T x) {
    return self.psi_func(x);
  }

  template <typename Self> auto phi(this Self &self, T x) {
    return self.phi_func(x);
  }

  auto max_x() const -> T { return max_x_val; }
  auto max_t() const -> T { return max_t_val; }

  auto get_h() const -> T { return h; }
  auto get_tau() const -> T { return tau; }
};

} // namespace ppr
