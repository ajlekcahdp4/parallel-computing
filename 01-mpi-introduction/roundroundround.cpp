// ----------------------------------------------------------------------------
//   "THE BEER-WARE LICENSE" (Revision 42):
//   <alex.rom23@mail.ru> wrote this file. As long as you retain this
//   notice you can do whatever you want with this stuff. If we meet some day,
//   and you think this stuff is worth it, you can buy me a beer in return.
// ----------------------------------------------------------------------------

#include <boost/mpi.hpp>

#include <fmt/core.h>

static constexpr int root_communicator_rank = 0;

int change_value(int val, int rank) {
  fmt::print("Rank: {}, current_value: {}\n", rank, val);
  return ++val;
}

namespace mpi = boost::mpi;
auto main(int argc, char **argv) -> int {
  unsigned n = 0;
  mpi::environment env(argc, argv);
  mpi::communicator world;
  int val = 0;
  if (world.rank() == root_communicator_rank) {
    val = change_value(val, world.rank());
    world.send(world.rank() + 1, /* tag */ 0, &val, /* num */ 1);
  } else {
    world.recv(world.rank() - 1, /* tag */ 0, &val, /* num */ 1);
    val = change_value(val, world.rank());
    world.send((world.rank() + 1) % world.size(), /* tag */ 0, &val,
               /* num */ 1);
  }
  if (world.rank() == root_communicator_rank) {
    world.recv(world.rank() - 1, /* tag */ 0, &val, /* num */ 1);
    val = change_value(val, world.rank());
  }
}
