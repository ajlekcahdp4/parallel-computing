// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#pragma once

#include "grid.hpp"
#include "transport_equation.hpp"

#include <type_traits>

namespace boost::mpi {
class communicator;
}

namespace ppr {

grid<double> solve_sequential(const transport_equation<double> &eq);
grid<double> solve_parallel(const transport_equation<double> &eq,
                            const boost::mpi::communicator &world);

} // namespace ppr
