#include <iostream>
#include <omp.h>
#include <sstream>

auto main() -> int {
#pragma omp parallel
  {
    auto ss = std::stringstream{};
    ss << "Thread: " << omp_get_thread_num() << "\n";
    std::cout << ss.str() << std::flush;
  }
}
