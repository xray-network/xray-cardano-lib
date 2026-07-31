#include "cardano/core/error.hpp"

#include <utility>

namespace cardano::core {

void enforce_linkage() noexcept {}

CardanoError::CardanoError(ErrorCode code, std::string message)
    : code_(code), message_(std::move(message)) {}

CardanoError::CardanoError(ErrorCode code, std::string message,
                           std::vector<ErrorPathComponent> path, std::optional<std::size_t> offset,
                           std::shared_ptr<const CardanoError> cause)
    : code_(code),
      message_(std::move(message)),
      path_(std::move(path)),
      offset_(offset),
      cause_(std::move(cause)) {}

ErrorCode CardanoError::code() const noexcept { return code_; }

const std::string& CardanoError::message() const noexcept { return message_; }

const std::vector<ErrorPathComponent>& CardanoError::path() const noexcept { return path_; }

std::optional<std::size_t> CardanoError::offset() const noexcept { return offset_; }

const std::shared_ptr<const CardanoError>& CardanoError::cause() const noexcept { return cause_; }

CardanoError CardanoError::at(ErrorPathComponent component) const {
  auto updated = path_;
  updated.insert(updated.begin(), std::move(component));
  return CardanoError(code_, message_, std::move(updated), offset_, cause_);
}

}  // namespace cardano::core
