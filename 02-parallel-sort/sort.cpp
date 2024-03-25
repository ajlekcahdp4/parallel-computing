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

#include <random>
#include <bit>

static constexpr int root_communicator_rank = 0;

std::vector<unsigned> get_random_sequence(size_t len) {
	std::random_device rdev;
	std::default_random_engine reng(rdev());
	std::uniform_int_distribution<unsigned> uniform_dist (0, 10000);
	std::vector<unsigned> res;
	std::generate_n(std::back_inserter(res), len, [&]{ return uniform_dist(reng); });
	return res;
}

template <typename iter_t>
void merge(iter_t beg, iter_t end) {
	auto size = std::distance(beg, end);
	if (size == 1) return;
	auto middle = beg + size / 2;
	merge(beg, middle);
	merge(middle, end);
	std::inplace_merge(beg, middle, end);
}

namespace mpi = boost::mpi;
namespace po = boost::program_options;
auto main(int argc, char **argv) -> int {
  size_t n = 0;
  po::options_description desc("Allowed options");
  desc.add_options()("number", po::value(&n), "The length of sequence to sort");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
	auto sequence = get_random_sequence(n);

  mpi::environment env(argc, argv);
  mpi::communicator world;
  auto per_proc_len = n / world.size();
  
	std::vector in = {3, 2, 4, 10, 0, 3, 5, 8};
	merge(in.begin(), in.end());
	for (auto&& e: in)
		fmt::print("{} ", e);
	fmt::print("\n");
	
#if 0	
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
#endif
}
