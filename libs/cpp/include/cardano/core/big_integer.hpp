#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "cardano/core/error.hpp"

namespace cardano::core {

class BigInteger {
 public:
  BigInteger();
  BigInteger(std::int64_t value);
  BigInteger(std::uint64_t value);
  BigInteger(const BigInteger& other);
  BigInteger(BigInteger&& other) noexcept;
  BigInteger& operator=(const BigInteger& other);
  BigInteger& operator=(BigInteger&& other) noexcept;
  ~BigInteger();

  [[nodiscard]] static Result<BigInteger> from_decimal(std::string_view decimal);
  [[nodiscard]] static BigInteger from_unsigned_bytes_be(std::span<const std::byte> bytes);

  [[nodiscard]] std::string to_decimal() const;
  [[nodiscard]] std::vector<std::byte> to_unsigned_bytes_be() const;
  [[nodiscard]] bool is_negative() const noexcept;
  [[nodiscard]] bool is_zero() const noexcept;
  [[nodiscard]] bool fits_uint64() const noexcept;
  [[nodiscard]] bool fits_int64() const noexcept;
  [[nodiscard]] Result<std::uint64_t> to_uint64() const;
  [[nodiscard]] Result<std::int64_t> to_int64() const;

  BigInteger& operator+=(const BigInteger& rhs);
  BigInteger& operator-=(const BigInteger& rhs);
  BigInteger& operator*=(const BigInteger& rhs);
  BigInteger& operator/=(const BigInteger& rhs);
  BigInteger& operator%=(const BigInteger& rhs);

  friend BigInteger operator+(BigInteger left, const BigInteger& right) { return left += right; }
  friend BigInteger operator-(BigInteger left, const BigInteger& right) { return left -= right; }
  friend BigInteger operator*(BigInteger left, const BigInteger& right) { return left *= right; }
  friend BigInteger operator/(BigInteger left, const BigInteger& right) { return left /= right; }
  friend BigInteger operator%(BigInteger left, const BigInteger& right) { return left %= right; }
  friend BigInteger operator-(const BigInteger& value);
  friend bool operator==(const BigInteger& left, const BigInteger& right) noexcept;
  friend std::strong_ordering operator<=>(const BigInteger& left, const BigInteger& right) noexcept;

 private:
  struct Impl;
  explicit BigInteger(std::unique_ptr<Impl> impl);
  std::unique_ptr<Impl> impl_;
};

inline constexpr std::int64_t INT64_MIN_VALUE = std::numeric_limits<std::int64_t>::min();
inline constexpr std::int64_t INT64_MAX_VALUE = std::numeric_limits<std::int64_t>::max();
inline constexpr std::uint64_t UINT64_MAX_VALUE = std::numeric_limits<std::uint64_t>::max();

[[nodiscard]] Result<std::int64_t> as_int64(const BigInteger& value);
[[nodiscard]] Result<std::uint64_t> as_uint64(const BigInteger& value);

}  // namespace cardano::core
