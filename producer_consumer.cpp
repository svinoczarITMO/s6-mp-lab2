#include <pthread.h>
#include <unistd.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

static __thread int* tid_ptr = nullptr;

int get_tid() {
  if (tid_ptr == nullptr) return 0;
  return *tid_ptr;
}

struct SharedData {
  pthread_mutex_t mutex;
  pthread_cond_t producer_cond;
  pthread_cond_t consumer_cond;
  int value;
  bool value_available;
  bool producer_done;

  SharedData() : value(0), value_available(false), producer_done(false) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&producer_cond, nullptr);
    pthread_cond_init(&consumer_cond, nullptr);
  }

  ~SharedData() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&producer_cond);
    pthread_cond_destroy(&consumer_cond);
  }
};

struct ThreadArgs {
  SharedData* shared;
  int tid;
  int sleep_limit;
  bool debug;
  std::vector<int> numbers;
  pthread_t* consumers;
  int num_consumers;
};

void* producer_routine(void* arg) {
  ThreadArgs* args = static_cast<ThreadArgs*>(arg);
  int* tid_storage = new int(args->tid);
  tid_ptr = tid_storage;

  for (int num : args->numbers) {
    pthread_mutex_lock(&args->shared->mutex);
    while (args->shared->value_available) {
      pthread_cond_wait(&args->shared->producer_cond, &args->shared->mutex);
    }
    args->shared->value = num;
    args->shared->value_available = true;
    pthread_cond_broadcast(&args->shared->consumer_cond);
    pthread_mutex_unlock(&args->shared->mutex);
  }

  pthread_mutex_lock(&args->shared->mutex);
  args->shared->producer_done = true;
  pthread_cond_broadcast(&args->shared->consumer_cond);
  pthread_mutex_unlock(&args->shared->mutex);

  delete tid_storage;
  delete args;
  return nullptr;
}

void* consumer_routine(void* arg) {
  ThreadArgs* args = static_cast<ThreadArgs*>(arg);
  int* tid_storage = new int(args->tid);
  tid_ptr = tid_storage;
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

  int partial_sum = 0;
  while (true) {
    pthread_mutex_lock(&args->shared->mutex);
    while (!args->shared->value_available && !args->shared->producer_done) {
      pthread_cond_wait(&args->shared->consumer_cond, &args->shared->mutex);
    }

    if (args->shared->producer_done && !args->shared->value_available) {
      pthread_mutex_unlock(&args->shared->mutex);
      break;
    }

    if (args->shared->value_available) {
      partial_sum += args->shared->value;
      args->shared->value_available = false;
      if (args->debug) {
        std::cerr << get_tid() << " " << partial_sum << "\n";
      }
      pthread_cond_signal(&args->shared->producer_cond);
    }
    pthread_mutex_unlock(&args->shared->mutex);

    if (args->sleep_limit > 0) {
      usleep((rand() % args->sleep_limit) * 1000);
    }
  }

  delete tid_storage;
  delete args;
  return new int(partial_sum);
}

void* interruptor_routine(void* arg) {
  ThreadArgs* args = static_cast<ThreadArgs*>(arg);
  int* tid_storage = new int(args->tid);
  tid_ptr = tid_storage;

  while (true) {
    pthread_mutex_lock(&args->shared->mutex);
    bool done = args->shared->producer_done;
    pthread_mutex_unlock(&args->shared->mutex);
    if (done) break;

    int idx = rand() % args->num_consumers;
    pthread_cancel(args->consumers[idx]);
  }

  delete tid_storage;
  delete args;
  return nullptr;
}

int run_threads(int num_consumers, int sleep_limit, bool debug,
                const std::vector<int>& numbers) {
  srand(time(nullptr));
  SharedData shared;
  pthread_t producer, interruptor;
  pthread_t* consumers = new pthread_t[num_consumers];
  int next_tid = 2;

  ThreadArgs* pargs =
      new ThreadArgs{&shared, next_tid++, 0, debug, numbers, nullptr, 0};
  pthread_create(&producer, nullptr, producer_routine, pargs);

  for (int i = 0; i < num_consumers; ++i) {
    ThreadArgs* cargs =
        new ThreadArgs{&shared, next_tid++, sleep_limit, debug, {}, nullptr, 0};
    pthread_create(&consumers[i], nullptr, consumer_routine, cargs);
  }

  ThreadArgs* iargs = new ThreadArgs{&shared,   next_tid++,   0, false, {},
                                     consumers, num_consumers};
  pthread_create(&interruptor, nullptr, interruptor_routine, iargs);

  pthread_join(producer, nullptr);
  pthread_join(interruptor, nullptr);

  int total_sum = 0;
  for (int i = 0; i < num_consumers; ++i) {
    void* result;
    pthread_join(consumers[i], &result);
    total_sum += *static_cast<int*>(result);
    delete static_cast<int*>(result);
  }

  delete[] consumers;
  return total_sum;
}
