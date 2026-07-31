#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace cardano::core {

enum class ErrorCode {
  invalid_argument,
  invalid_length,
  out_of_range,
  invalid_cbor,
  trailing_data,
  truncated_input,
  invalid_structure,
  invalid_utf8,
  depth_limit_exceeded,
  resource_limit_exceeded,
  duplicate_key,
  checksum_mismatch,
  invalid_encoding,
  cryptography,
  crypto_failure,
  authentication_failed,
  random_unavailable,
  disposed,
  unsupported,
  evaluation,
  balance,
  missing_witness,
  integrity,
  internal
};

using ErrorPathComponent = std::variant<std::size_t, std::string>;

class CardanoError {
 public:
  CardanoError(ErrorCode code, std::string message);
  CardanoError(ErrorCode code, std::string message, std::vector<ErrorPathComponent> path,
               std::optional<std::size_t> offset = std::nullopt,
               std::shared_ptr<const CardanoError> cause = nullptr);

  [[nodiscard]] ErrorCode code() const noexcept;
  [[nodiscard]] const std::string& message() const noexcept;
  [[nodiscard]] const std::vector<ErrorPathComponent>& path() const noexcept;
  [[nodiscard]] std::optional<std::size_t> offset() const noexcept;
  [[nodiscard]] const std::shared_ptr<const CardanoError>& cause() const noexcept;
  [[nodiscard]] CardanoError at(ErrorPathComponent component) const;

 private:
  ErrorCode code_;
  std::string message_;
  std::vector<ErrorPathComponent> path_;
  std::optional<std::size_t> offset_;
  std::shared_ptr<const CardanoError> cause_;
};

template <typename T>
using Result = std::expected<T, CardanoError>;

using VoidResult = Result<std::monostate>;

}  // namespace cardano::core
