#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "cardano/core/error.hpp"

namespace cardano::core {

using Byte = std::byte;
using Bytes = std::vector<Byte>;
using ByteSpan = std::span<const Byte>;

[[nodiscard]] Bytes copy_bytes(ByteSpan input);
[[nodiscard]] bool bytes_equal(ByteSpan left, ByteSpan right) noexcept;
[[nodiscard]] std::string bytes_to_hex(ByteSpan input);
[[nodiscard]] Result<Bytes> hex_to_bytes(std::string_view input);
[[nodiscard]] bool is_valid_utf8(ByteSpan input) noexcept;
[[nodiscard]] VoidResult assert_byte_length(ByteSpan input, std::size_t expected,
                                            std::string_view name = "byte string");

template <std::size_t Size>
class FixedBytes {
 public:
  static constexpr std::size_t size = Size;

  FixedBytes() = default;

  [[nodiscard]] static Result<FixedBytes> from_bytes(ByteSpan bytes) {
    if (bytes.size() != Size) {
      return std::unexpected(
          CardanoError(ErrorCode::out_of_range, "fixed byte string has an invalid length"));
    }
    FixedBytes result;
    for (std::size_t index = 0; index < Size; ++index) {
      result.bytes_[index] = bytes[index];
    }
    return result;
  }

  [[nodiscard]] static Result<FixedBytes> from_hex(std::string_view hex) {
    auto bytes = hex_to_bytes(hex);
    if (!bytes) {
      return std::unexpected(bytes.error());
    }
    return from_bytes(*bytes);
  }

  [[nodiscard]] std::array<Byte, Size> bytes() const noexcept { return bytes_; }
  [[nodiscard]] ByteSpan view() const noexcept { return bytes_; }
  [[nodiscard]] std::string hex() const { return bytes_to_hex(bytes_); }

  friend bool operator==(const FixedBytes&, const FixedBytes&) = default;
  friend auto operator<=>(const FixedBytes&, const FixedBytes&) = default;

 private:
  std::array<Byte, Size> bytes_{};
};

}  // namespace cardano::core
