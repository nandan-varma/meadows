#ifndef MEADOWS_LARGE_STACK_H
#define MEADOWS_LARGE_STACK_H

#include <cstddef>
#include <functional>

#include "../runtime/Limits.h"

namespace meadows {

constexpr size_t LARGE_STACK_SIZE = MEADOWS_STACK_SIZE;
constexpr size_t STACK_RESERVE = MEADOWS_STACK_RESERVE;

void runOnLargeStack(const std::function<void()>& task);
bool stackNearlyExhausted();

} // namespace meadows

#endif
