#pragma once
#include <vector>

int get_tid();

// the declaration of run threads can be changed as you like
int run_threads(int num_consumers = 2, int sleep_limit = 0, bool debug = false,
                const std::vector<int>& numbers = {});