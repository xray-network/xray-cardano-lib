#include "cardano/core/bytes.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace cardano::core {
namespace {

[[nodiscard]] int hex_nibble(char value) noexcept {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

}  // namespace

Bytes copy_bytes(ByteSpan input) { return Bytes(input.begin(), input.end()); }

bool bytes_equal(ByteSpan left, ByteSpan right) noexcept {
  return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin());
}

std::string bytes_to_hex(ByteSpan input) {
  constexpr std::array<char, 16> alphabet{'0', '1', '2', '3', '4', '5', '6', '7',
                                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::string output;
  output.resize(input.size() * 2U);
  for (std::size_t index = 0; index < input.size(); ++index) {
    const auto value = std::to_integer<unsigned int>(input[index]);
    output[index * 2U] = alphabet[value >> 4U];
    output[index * 2U + 1U] = alphabet[value & 0x0fU];
  }
  return output;
}

Result<Bytes> hex_to_bytes(std::string_view input) {
  if ((input.size() & 1U) != 0U) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding,
                     "hexadecimal input must contain an even number of characters"));
  }
  Bytes output(input.size() / 2U);
  for (std::size_t index = 0; index < output.size(); ++index) {
    const int high = hex_nibble(input[index * 2U]);
    const int low = hex_nibble(input[index * 2U + 1U]);
    if (high < 0 || low < 0) {
      return std::unexpected(CardanoError(
          ErrorCode::invalid_encoding, "hexadecimal input contains a non-hexadecimal character"));
    }
    output[index] = static_cast<Byte>((high << 4) | low);
  }
  return output;
}

bool is_valid_utf8(ByteSpan input) noexcept {
  std::size_t index = 0;
  while (index < input.size()) {
    const auto first = std::to_integer<std::uint8_t>(input[index]);
    if (first <= 0x7fU) {
      ++index;
      continue;
    }
    std::size_t count = 0;
    std::uint32_t codepoint = 0;
    std::uint32_t minimum = 0;
    if ((first & 0xe0U) == 0xc0U) {
      count = 1;
      codepoint = first & 0x1fU;
      minimum = 0x80U;
    } else if ((first & 0xf0U) == 0xe0U) {
      count = 2;
      codepoint = first & 0x0fU;
      minimum = 0x800U;
    } else if ((first & 0xf8U) == 0xf0U) {
      count = 3;
      codepoint = first & 0x07U;
      minimum = 0x10000U;
    } else {
      return false;
    }
    if (index + count >= input.size()) {
      return false;
    }
    for (std::size_t continuation = 1; continuation <= count; ++continuation) {
      const auto byte = std::to_integer<std::uint8_t>(input[index + continuation]);
      if ((byte & 0xc0U) != 0x80U) {
        return false;
      }
      codepoint = (codepoint << 6U) | (byte & 0x3fU);
    }
    if (codepoint < minimum || codepoint > 0x10ffffU ||
        (codepoint >= 0xd800U && codepoint <= 0xdfffU)) {
      return false;
    }
    index += count + 1;
  }
  return true;
}

VoidResult assert_byte_length(ByteSpan input, std::size_t expected, std::string_view name) {
  if (input.size() != expected) {
    return std::unexpected(CardanoError(
        ErrorCode::out_of_range,
        std::string(name) + " must contain exactly " + std::to_string(expected) + " bytes"));
  }
  return std::monostate{};
}

}  // namespace cardano::core
