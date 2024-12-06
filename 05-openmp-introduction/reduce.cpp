#include <boost/program_options.hpp>

#include <fmt/core.h>

#include <iostream>

namespace po = boost::program_options;

auto main(int argc, const char **argv) -> int {
  auto desc = po::options_description{"Allowed options"};
  auto n = 0u;

  desc.add_options()("help", "produce help message")(
      "num", po::value(&n)->default_value(1u << 28), "Number to sum");

  auto vm = po::variables_map{};
  po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  po::notify(vm);

  auto sum = 0.0;
#pragma omp parallel for reduction(+ : sum) schedule(static)
  for (auto i = 1u; i <= n; ++i) {
    auto val = 1.0 / i;
    sum += val;
  }

  fmt::println("{:.3}", sum);
}
