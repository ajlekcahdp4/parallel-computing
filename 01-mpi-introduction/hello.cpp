// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <fmt/core.h>

namespace mpi = boost::mpi;
int main(int argc, char **argv) {
  mpi::environment env(argc, argv);
  mpi::communicator world;
  fmt::print("Hello, World! Size: {}, Rank: {}\n", world.size(), world.rank());
}
