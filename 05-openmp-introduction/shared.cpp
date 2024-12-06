#include <fmt/core.h>

#include <omp.h>

auto main() -> int {
  unsigned long shared_var = 1;
  int num_threads = 0;
#pragma omp parallel shared(shared_var, num_threads)
  {
#pragma omp single
    {
      num_threads = omp_get_num_threads();
      fmt::println("Number of threads: {}", num_threads);
    }

    int thread_num = omp_get_thread_num();

    for (int i = 0; i < num_threads; ++i) {
#pragma omp barrier
      if (i == thread_num) {
#pragma omp critical
        {
          shared_var += shared_var;
          fmt::println("Thread {}: updated value to: {}", thread_num,
                       shared_var);
        }
      }
    }
  }
}
