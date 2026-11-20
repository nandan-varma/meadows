#ifndef MEADOWS_LARGE_STACK_H
#define MEADOWS_LARGE_STACK_H

#include <cstddef>
#include <functional>

namespace meadows {

constexpr size_t LARGE_STACK_SIZE = size_t{256} * 1024 * 1024;
constexpr size_t STACK_RESERVE = size_t{1} * 1024 * 1024;

void runOnLargeStack(const std::function<void()>& task);
bool stackNearlyExhausted();

} // namespace meadows

#endif
