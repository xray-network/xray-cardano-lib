#pragma once

#include <cstddef>

#include "cardano/core/bytes.hpp"
#include "cardano/core/error.hpp"

namespace cardano::core {

class SecureRandomSource {
 public:
  virtual ~SecureRandomSource() = default;
  [[nodiscard]] virtual Result<Bytes> random_bytes(std::size_t length) = 0;
};

}  // namespace cardano::core
