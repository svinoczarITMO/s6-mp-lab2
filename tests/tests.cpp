#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <producer_consumer.h>

TEST_CASE("basic functionality") {
  std::vector<int> numbers = {1, 2, 3, 4, 5};
  int sum = run_threads(2, 0, false, numbers);
  CHECK(sum == 15);
}

TEST_CASE("single consumer") {
  std::vector<int> numbers = {10, 20, 30};
  int sum = run_threads(1, 0, false, numbers);
  CHECK(sum == 60);
}

TEST_CASE("empty input") {
  std::vector<int> numbers;
  int sum = run_threads(3, 0, false, numbers);
  CHECK(sum == 0);
}