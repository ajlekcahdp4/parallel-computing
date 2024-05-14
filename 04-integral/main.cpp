#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/lockfree/stack.hpp>
#include <boost/thread/concurrent_queues/queue_adaptor.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <range/v3/all.hpp>

#include <cmath>
#include <concepts>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

template <typename F, typename T>
concept integrable_function = requires(F f, T x) {
  { f(x) } -> std::convertible_to<T>;
};

static double eps = 1e-5;
constexpr unsigned max_depth = 10;

template <std::floating_point T> struct integrate_task final {
  T start;
  T stop;
  T val_start;
  T val_stop;
  T approximation;
};

template <std::floating_point T>
bool needs_step_adjustment(T old_area, T new_area) {
  return std::abs(old_area - new_area) > eps * std::abs(old_area);
}

template <std::floating_point T>
auto integrate_trapeze(T left, T right, T step) {
  return (left + right) * step / 2;
}

template <std::floating_point T, integrable_function<T> F>
auto integrate(F f, T start, T stop, unsigned n = 0) -> T {
  if (start > stop)
    throw std::runtime_error("start should be less then stop");
  auto mid = std::midpoint(start, stop);
  auto val_left = f(start);
  auto val_mid = f(mid);
  auto val_right = f(stop);
  auto whole = integrate_trapeze(val_left, val_right, stop - start);
  auto left = integrate_trapeze(val_left, val_mid, mid - start);
  auto right = integrate_trapeze(val_mid, val_right, stop - mid);
  if (n < 100)
    return (needs_step_adjustment(whole, left + right))
               ? (integrate(f, start, mid, n + 1) +
                  integrate(f, mid, stop, n + 1))
               : whole;
  return whole;
}

template <std::floating_point T, integrable_function<T> F>
auto integrate_parallel_impl(F f, const integrate_task<T> &task,
                             unsigned depth = 0) -> T {
  auto mid = std::midpoint(task.start, task.stop);
  auto val_mid = f(mid);
  auto left = integrate_trapeze(task.val_start, val_mid, mid - task.start);
  auto right = integrate_trapeze(val_mid, task.val_stop, task.stop - mid);
  auto new_approximation = right + left;
  if (needs_step_adjustment(task.approximation, new_approximation)) {
    if (depth < max_depth) {
      auto handle =
          std::async(std::launch::async, integrate_parallel_impl<T, F>, f,
                     integrate_task{.start = task.start,
                                    .stop = mid,
                                    .val_start = task.val_start,
                                    .val_stop = val_mid,
                                    .approximation = left},
                     depth + 1);
      auto right_val =
          integrate_parallel_impl(f,
                                  integrate_task{.start = mid,
                                                 .stop = task.stop,
                                                 .val_start = mid,
                                                 .val_stop = task.val_stop,
                                                 .approximation = right},
                                  depth + 1);
      return right_val + handle.get();
    }
    return integrate(f, task.start, mid) + integrate(f, mid, task.stop);
  }
  return task.approximation;
}

template <std::floating_point T, integrable_function<T> F>
auto integrate_parallel(F f, T start, T stop) {
  if (start > stop)
    throw std::runtime_error("start should be less then stop");
  auto val_start = f(start);
  auto val_stop = f(stop);
  integrate_task task{.start = start,
                      .stop = stop,
                      .val_start = val_start,
                      .val_stop = val_stop,
                      .approximation =
                          integrate_trapeze(val_start, val_stop, stop - start)};
  return integrate_parallel_impl(f, task);
}

template <std::floating_point T, integrable_function<T> F>
auto parallel_integrate(F f, T start_arg, T stop_arg, unsigned num_threads) {
  boost::lockfree::stack<integrate_task<T>> global_stack(num_threads * 2);
  auto res = std::atomic<T>{};
  auto thread_action = [&](integrate_task<T> task) {
    auto local_res = T{};
    std::vector<integrate_task<T>> local_tasks;
    local_tasks.emplace_back(std::move(task));
    auto process_interval = [&](const integrate_task<T> &task) {
      auto mid = std::midpoint(task.start, task.stop);
      auto val_left = f(task.start);
      auto val_mid = f(mid);
      auto val_right = f(task.stop);
      auto whole = task.approximation;
      auto left = integrate_trapeze(val_left, val_mid, mid - task.start);
      auto right = integrate_trapeze(val_mid, val_right, task.stop - mid);
      auto sum = left + right;
      if (needs_step_adjustment(whole, sum)) {
        local_tasks.push_back(integrate_task{.start = task.start,
                                             .stop = mid,
                                             .val_start = task.val_start,
                                             .val_stop = val_mid,
                                             .approximation = left});
        local_tasks.push_back(integrate_task{.start = mid,
                                             .stop = task.stop,
                                             .val_start = mid,
                                             .val_stop = task.val_stop,
                                             .approximation = right});
        return;
      }
      local_res += whole;
    };

    while (local_tasks.size() <= 32) {
      if (local_tasks.empty()) {
        res += local_res;
        return;
      }
      auto curr_task = std::move(local_tasks.back());
      local_tasks.pop_back();
      process_interval(curr_task);
    }
    for (auto &&e : local_tasks)
      global_stack.push(std::move(e));
    res += local_res;
  };
  if (start_arg > stop_arg)
    throw std::runtime_error("start should be less then stop");
  auto val_start = f(start_arg);
  auto val_stop = f(stop_arg);
  global_stack.push(
      integrate_task{.start = start_arg,
                     .stop = stop_arg,
                     .val_start = val_start,
                     .val_stop = val_stop,
                     .approximation = integrate_trapeze(val_start, val_stop,
                                                        stop_arg - start_arg)});
  auto threads = std::vector<std::thread>{};

  auto activate_thread = [&] {
    unsigned failed_attempts = 0;
    for (;;) {
      integrate_task<T> tsk;
      auto consumed = global_stack.pop(tsk);
      if (!consumed) {
        ++failed_attempts;
        if (failed_attempts > 20)
          return;
      } else {
        failed_attempts = 0;
        thread_action(tsk);
      }
    }
  };

  for (auto i = 0uz; i < num_threads; ++i)
    threads.emplace_back(activate_thread);
  activate_thread();
  for (auto &&thread : threads)
    thread.join();
  return res.load();
}

namespace po = boost::program_options;

auto main(int argc, char **argv) -> int {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;
  using std::chrono::milliseconds;
  po::options_description desc("Allowed options");
  bool measure = false, print = true, parallel = true;
  double start = 0.0;
  double stop = 1.0;
  unsigned n_threads = 1;
  desc.add_options()("help", "Show this message")(
      "parallel", po::value(&parallel),
      "parallel")("measure", po::value(&measure), "Measure")(
      "start", po::value(&start)->required(), "left bound of the interval")(
      "stop", po::value(&stop)->required(), "right bound of the interval")(
      "print", po::value(&print), "print result")("epsilon", po::value(&eps),
                                    "accuracy")(
      "n_threads", po::value(&n_threads), "number of threads");
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << "\n";
    return EXIT_FAILURE;
  }

  auto f = [](double x) { return std::sin(1 / x); };

  auto start_parallel = high_resolution_clock::now();
  auto res_parallel = [&] {
    if (parallel)
      return parallel_integrate<double>(f, start, stop, n_threads);
    return integrate<double>(f, start, stop);
  }();
  auto stop_parallel = high_resolution_clock::now();
  if (measure) {
    duration<double, std::milli> ms_double = stop_parallel - start_parallel;
    fmt::println("elapsed time: {}", ms_double.count());
  }
  if (print)
    fmt::println("result: {:.10f}", res_parallel);
}
