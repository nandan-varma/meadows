#include "ErrorHandler.h"

namespace meadows {

ErrorHandler::ErrorHandler() = default;

void ErrorHandler::addProcessor(std::unique_ptr<ErrorProcessor> processor) {
  if (!chain_) {
    chain_ = std::move(processor);
  } else {
    appendProcessor(std::move(processor));
  }
}

void ErrorHandler::appendProcessor(std::unique_ptr<ErrorProcessor> processor) {
  ErrorProcessor *current = chain_.get();
  while (current->getNext()) {
    current = current->getNext();
  }
  current->setNext(std::move(processor));
}

bool ErrorHandler::handle(const CompilationError &error) {
  if (!chain_) {
    // No processors, just output to stderr
    std::cerr << error.format() << std::endl;
    return true;
  }

  return chain_->handle(error, context_);
}

void ErrorHandler::handle(const std::vector<CompilationError> &errors) {
  for (const auto &error : errors) {
    if (handle(error)) {
      // Error was handled
      if (context_.shouldStop()) {
        std::cerr << "error: too many errors emitted, stopping compilation"
                  << std::endl;
        break;
      }
    }
  }
}

void ErrorHandler::reset() {
  context_ = ErrorContext();
  chain_.reset();
}

} // namespace meadows
