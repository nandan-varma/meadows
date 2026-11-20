#include "LargeStack.h"

#include <exception>
#include <stdexcept>

#ifdef __EMSCRIPTEN__
#include <emscripten/stack.h>
#else
#include <pthread.h>
#endif

namespace meadows {

#ifdef __EMSCRIPTEN__

void runOnLargeStack(const std::function<void()>& task) { task(); }

bool stackNearlyExhausted() { return emscripten_stack_get_free() < STACK_RESERVE; }

#else

namespace {

thread_local const char* stackLimit = nullptr;

struct Job {
  const std::function<void()>* task;
  std::exception_ptr failure;
};

void* enter(void* argument) {
  Job& job = *static_cast<Job*>(argument);
  char marker = 0;
  stackLimit = &marker - (LARGE_STACK_SIZE - STACK_RESERVE);
  try {
    (*job.task)();
  } catch (...) {
    job.failure = std::current_exception();
  }
  stackLimit = nullptr;
  return nullptr;
}

} // namespace

void runOnLargeStack(const std::function<void()>& task) {
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);
  pthread_attr_setstacksize(&attributes, LARGE_STACK_SIZE);
  Job job{&task, nullptr};
  pthread_t thread;
  int status = pthread_create(&thread, &attributes, enter, &job);
  pthread_attr_destroy(&attributes);
  if (status != 0) {
    throw std::runtime_error("cannot allocate the interpreter stack");
  }
  pthread_join(thread, nullptr);
  if (job.failure) {
    std::rethrow_exception(job.failure);
  }
}

bool stackNearlyExhausted() {
  char marker = 0;
  return stackLimit != nullptr && &marker < stackLimit;
}

#endif

} // namespace meadows
