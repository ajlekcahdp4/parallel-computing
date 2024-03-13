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

#include <fmt/core.h>

static constexpr int root_communicator_rank = 0;

namespace mpi = boost::mpi;
namespace po = boost::program_options;
auto main(int argc, char **argv) -> int {
  unsigned n = 0;
  po::options_description desc("Allowed options");
  desc.add_options()("number", po::value(&n), "N sum number");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  mpi::environment env(argc, argv);
  mpi::communicator world;
  auto per_proc = n / world.size();
  double local_res = 0.0;
  if (world.rank() != world.size() - 1)
    for (unsigned i = 1; i <= per_proc; ++i)
      local_res += 1.0 / (i * world.rank() + 1);
  else
    for (unsigned i = per_proc * world.size(); i <= n; ++i)
      local_res += 1.0 / i;
  double reduced = 0.0;
  mpi::reduce(world, local_res, reduced, std::plus{}, 0);
  double result = reduced * n;
  if (world.rank() == root_communicator_rank)
    fmt::print("Rank: {}, Sum over N: {}, result: {:.2f}\n", world.rank(), n,
               result);
}
