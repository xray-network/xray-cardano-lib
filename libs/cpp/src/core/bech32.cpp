#include "cardano/core/bech32.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace cardano::core {
namespace {

constexpr std::string_view alphabet = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
constexpr std::array<std::uint32_t, 5> generators{0x3b6a57b2U, 0x26508e6dU, 0x1ea119faU,
                                                  0x3d4233ddU, 0x2a1462b3U};

[[nodiscard]] std::uint32_t polymod(std::span<const std::uint8_t> values) {
  std::uint32_t checksum = 1U;
  for (const auto value : values) {
    const std::uint32_t top = checksum >> 25U;
    checksum = ((checksum & 0x1ffffffU) << 5U) ^ value;
    for (std::size_t index = 0; index < generators.size(); ++index) {
      if (((top >> index) & 1U) != 0U) {
        checksum ^= generators[index];
      }
    }
  }
  return checksum;
}

[[nodiscard]] std::vector<std::uint8_t> expand(std::string_view prefix) {
  std::vector<std::uint8_t> output;
  output.reserve(prefix.size() * 2U + 1U);
  for (const auto character : prefix) {
    output.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(character) >> 5U));
  }
  output.push_back(0U);
  for (const auto character : prefix) {
    output.push_back(static_cast<std::uint8_t>(static_cast<unsigned char>(character) & 31U));
  }
  return output;
}

[[nodiscard]] Result<std::vector<std::uint8_t>> convert_bits(std::span<const std::uint8_t> values,
                                                             unsigned from, unsigned to, bool pad) {
  std::uint32_t accumulator = 0;
  unsigned bits = 0;
  const std::uint32_t mask = (1U << to) - 1U;
  std::vector<std::uint8_t> output;
  for (const auto value : values) {
    if ((static_cast<unsigned>(value) >> from) != 0U) {
      return std::unexpected(CardanoError(ErrorCode::invalid_encoding,
                                          "Bech32 data value exceeds its source bit width"));
    }
    accumulator = (accumulator << from) | value;
    bits += from;
    while (bits >= to) {
      bits -= to;
      output.push_back(static_cast<std::uint8_t>((accumulator >> bits) & mask));
    }
  }
  if (pad && bits > 0U) {
    output.push_back(static_cast<std::uint8_t>((accumulator << (to - bits)) & mask));
  } else if (!pad && (bits >= from || ((accumulator << (to - bits)) & mask) != 0U)) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "invalid Bech32 zero padding"));
  }
  return output;
}

[[nodiscard]] int alphabet_index(char character) noexcept {
  const auto position = alphabet.find(character);
  return position == std::string_view::npos ? -1 : static_cast<int>(position);
}

}  // namespace

Result<std::string> encode_bech32(std::string_view prefix, ByteSpan bytes) {
  if (prefix.empty()) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_argument, "Bech32 prefix cannot be empty"));
  }
  for (const char character : prefix) {
    const auto value = static_cast<unsigned char>(character);
    if (value < 0x21U || value > 0x7eU || (character >= 'A' && character <= 'Z')) {
      return std::unexpected(CardanoError(ErrorCode::invalid_argument,
                                          "Bech32 prefix must be lowercase printable ASCII"));
    }
  }

  std::vector<std::uint8_t> raw;
  raw.reserve(bytes.size());
  for (const auto byte : bytes) {
    raw.push_back(static_cast<std::uint8_t>(std::to_integer<unsigned int>(byte)));
  }
  auto words = convert_bits(raw, 8U, 5U, true);
  if (!words) {
    return std::unexpected(words.error());
  }
  auto checksum_input = expand(prefix);
  checksum_input.insert(checksum_input.end(), words->begin(), words->end());
  checksum_input.insert(checksum_input.end(), 6U, 0U);
  const std::uint32_t checksum = polymod(checksum_input) ^ 1U;

  std::string output(prefix);
  output.push_back('1');
  for (const auto word : *words) {
    output.push_back(alphabet[word]);
  }
  for (std::size_t index = 0; index < 6U; ++index) {
    const auto shift = static_cast<unsigned>(5U * (5U - index));
    output.push_back(alphabet[(checksum >> shift) & 31U]);
  }
  return output;
}

Result<Bech32Value> decode_bech32(std::string_view value) {
  bool has_lower = false;
  bool has_upper = false;
  for (const char character : value) {
    has_lower = has_lower || (character >= 'a' && character <= 'z');
    has_upper = has_upper || (character >= 'A' && character <= 'Z');
  }
  if (has_lower && has_upper) {
    return std::unexpected(CardanoError(ErrorCode::invalid_encoding, "mixed-case Bech32 string"));
  }

  std::string normalized(value);
  std::transform(
      normalized.begin(), normalized.end(), normalized.begin(),
      [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
  const auto separator = normalized.rfind('1');
  if (separator == std::string::npos || separator < 1U || separator + 7U > normalized.size()) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "invalid Bech32 separator or checksum length"));
  }

  const std::string prefix = normalized.substr(0, separator);
  std::vector<std::uint8_t> words;
  words.reserve(normalized.size() - separator - 1U);
  for (std::size_t index = separator + 1U; index < normalized.size(); ++index) {
    const int word = alphabet_index(normalized[index]);
    if (word < 0) {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_encoding, "invalid Bech32 alphabet character"));
    }
    words.push_back(static_cast<std::uint8_t>(word));
  }

  auto checksum_input = expand(prefix);
  checksum_input.insert(checksum_input.end(), words.begin(), words.end());
  if (polymod(checksum_input) != 1U) {
    return std::unexpected(CardanoError(ErrorCode::checksum_mismatch, "invalid Bech32 checksum"));
  }

  const auto payload_end = words.end() - 6;
  std::vector<std::uint8_t> payload(words.begin(), payload_end);
  auto converted = convert_bits(payload, 5U, 8U, false);
  if (!converted) {
    return std::unexpected(converted.error());
  }
  Bytes bytes;
  bytes.reserve(converted->size());
  for (const auto byte : *converted) {
    bytes.push_back(static_cast<Byte>(byte));
  }
  return Bech32Value{prefix, std::move(bytes)};
}

}  // namespace cardano::core
