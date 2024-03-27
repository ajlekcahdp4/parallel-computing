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

#include <fmt/core.h>

#include <range/v3/all.hpp>

#include <bit>
#include <random>

namespace {
static constexpr int root_communicator_rank = 0;

std::vector<unsigned> get_random_sequence(size_t len) {
  std::random_device rdev;
  std::default_random_engine reng(rdev());
  std::uniform_int_distribution<unsigned> uniform_dist(0, 1e+8);
  std::vector<unsigned> res;
  std::generate_n(std::back_inserter(res), len,
                  [&] { return uniform_dist(reng); });
  return res;
}

template <typename iter_t> void merge_sort(iter_t beg, iter_t end) {
  auto size = std::distance(beg, end);
  if (size == 1)
    return;
  auto middle = beg + size / 2;
  merge_sort(beg, middle);
  merge_sort(middle, end);
  std::inplace_merge(beg, middle, end);
}

template <typename T>
std::vector<T> merge_sorted(const std::vector<std::vector<T>> &vv) {
  auto size =
      ranges::accumulate(vv.begin(), vv.end(), 0,
                         [](auto init, auto &&v) { return init + v.size(); });
  std::vector<T> sorted;
  sorted.reserve(size);
  auto middle = sorted.end();
  for (auto &&v : vv) {
    if (middle == sorted.begin()) {
      sorted.insert(sorted.end(), v.begin(), v.end());
      middle = sorted.end();
      continue;
    }
    sorted.insert(sorted.end(), v.begin(), v.end());
    std::inplace_merge(sorted.begin(), middle, sorted.end());
    middle = sorted.end();
  }
  return sorted;
}

namespace mpi = boost::mpi;
template <typename T>
void parallel_merge(std::vector<T> &sequence, const mpi::communicator &world) {
  auto beg = sequence.begin(), end = sequence.end();
  auto size = std::distance(beg, end);
  auto per_proc_len = size / world.size();
  std::vector<std::vector<T>> vv;
  if (world.rank() == root_communicator_rank) {
    vv = ranges::views::chunk(sequence, per_proc_len) |
         ranges::views::transform(
             [](auto &&range) { return range | ranges::to<std::vector>; }) |
         ranges::to<std::vector>;
    while (vv.size() > world.size()) {
      auto &&tail = std::move(vv.back());
      vv.pop_back();
      ranges::copy(tail, std::back_inserter(vv.back()));
    }
  }
  std::vector<unsigned> mine;
  mpi::scatter(world, vv, mine, root_communicator_rank);
  merge_sort(mine.begin(), mine.end());
  mpi::gather(world, mine, vv, root_communicator_rank);
  if (world.rank() == root_communicator_rank) {
    sequence = merge_sorted(vv);
  }
}
} // namespace

namespace po = boost::program_options;
auto main(int argc, char **argv) -> int {
  size_t n = 0;
  bool print = false, check = false, parallel = false;
  po::options_description desc("Allowed options");
  desc.add_options()("number", po::value(&n), "The length of sequence to sort")(
      "print", po::value(&print), "Print sorted sequence")(
      "check", po::value(&check), "Check id sequence is sorted correctly")(
      "parallel", po::value(&parallel), "Use parallel merge sort");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  auto seq = get_random_sequence(n);
  mpi::environment env(argc, argv);
  mpi::communicator world;
  if (parallel)
    parallel_merge(seq, world);
  else
    merge_sort(seq.begin(), seq.end());
  if (world.rank() == root_communicator_rank) {
    if (print) {
      for (auto e : seq)
        fmt::print("{} ", e);
      fmt::print("\n");
    }
    if (check) {
      if (!std::is_sorted(seq.begin(), seq.end())) {
        fmt::print("Sequence is not sorted\n");
        return 1;
      }
      if (seq.size() != n) {
        fmt::print("Sequnce has wrong size\n");
        return 1;
      }
    }
  }
}
