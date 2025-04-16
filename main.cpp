#include <iostream>
#include <string>
#include <vector>
#include "producer_consumer.h"

void display_usage(const std::string& program_name) {
  std::cerr << "Usage: " << program_name
            << " <num_consumers> <sleep_limit> [--debug]\n";
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    display_usage(argv[0]);
    return EXIT_FAILURE;
  }

  try {
    int consumer_count = std::stoi(argv[1]);
    int sleep_limit = std::stoi(argv[2]);
    bool debug_mode = (argc > 3 && std::string(argv[3]) == "--debug");

    std::vector<int> numbers;
    int num;
    while (std::cin >> num) {
      numbers.push_back(num);
    }

    int total_sum =
        run_threads(consumer_count, sleep_limit, debug_mode, numbers);
    std::cout << total_sum << std::endl;

  } catch (const std::invalid_argument& e) {
    std::cerr << "Invalid argument: " << e.what() << "\n";
    return EXIT_FAILURE;
  } catch (const std::out_of_range& e) {
    std::cerr << "Number out of range: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
