#pragma once

#include <string>
#include <string_view>

#include "cardano/core/bytes.hpp"

namespace cardano::core {

struct Bech32Value {
  std::string prefix;
  Bytes bytes;
};

[[nodiscard]] Result<std::string> encode_bech32(std::string_view prefix, ByteSpan bytes);
[[nodiscard]] Result<Bech32Value> decode_bech32(std::string_view value);

}  // namespace cardano::core
