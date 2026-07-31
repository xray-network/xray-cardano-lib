#include "cardano/core/big_integer.hpp"

#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <iterator>
#include <limits>
#include <sstream>
#include <utility>

namespace cardano::core {

using NativeInteger = boost::multiprecision::cpp_int;

struct BigInteger::Impl {
  NativeInteger value;
};

BigInteger::BigInteger() : impl_(std::make_unique<Impl>()) {}

BigInteger::BigInteger(std::int64_t value) : impl_(std::make_unique<Impl>()) {
  impl_->value = value;
}

BigInteger::BigInteger(std::uint64_t value) : impl_(std::make_unique<Impl>()) {
  impl_->value = value;
}

BigInteger::BigInteger(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

BigInteger::BigInteger(const BigInteger& other) : impl_(std::make_unique<Impl>(*other.impl_)) {}

BigInteger::BigInteger(BigInteger&& other) noexcept = default;

BigInteger& BigInteger::operator=(const BigInteger& other) {
  if (this != &other) {
    impl_ = std::make_unique<Impl>(*other.impl_);
  }
  return *this;
}

BigInteger& BigInteger::operator=(BigInteger&& other) noexcept = default;

BigInteger::~BigInteger() = default;

Result<BigInteger> BigInteger::from_decimal(std::string_view decimal) {
  if (decimal.empty()) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "integer decimal string cannot be empty"));
  }
  std::size_t offset = decimal.front() == '-' ? 1U : 0U;
  if (offset == decimal.size()) {
    return std::unexpected(
        CardanoError(ErrorCode::invalid_encoding, "integer decimal string has no digits"));
  }
  NativeInteger parsed = 0;
  for (; offset < decimal.size(); ++offset) {
    const char character = decimal[offset];
    if (character < '0' || character > '9') {
      return std::unexpected(
          CardanoError(ErrorCode::invalid_encoding, "integer decimal string contains a non-digit"));
    }
    parsed *= 10;
    parsed += character - '0';
  }
  if (decimal.front() == '-') {
    parsed = -parsed;
  }
  auto impl = std::make_unique<Impl>();
  impl->value = std::move(parsed);
  return BigInteger(std::move(impl));
}

BigInteger BigInteger::from_unsigned_bytes_be(std::span<const std::byte> bytes) {
  auto impl = std::make_unique<Impl>();
  for (const auto byte : bytes) {
    impl->value <<= 8U;
    impl->value += std::to_integer<unsigned int>(byte);
  }
  return BigInteger(std::move(impl));
}

std::string BigInteger::to_decimal() const { return impl_->value.str(); }

std::vector<std::byte> BigInteger::to_unsigned_bytes_be() const {
  if (impl_->value < 0) {
    return {};
  }
  std::vector<unsigned char> native;
  export_bits(impl_->value, std::back_inserter(native), 8, true);
  std::vector<std::byte> output;
  output.reserve(native.size());
  std::transform(native.begin(), native.end(), std::back_inserter(output),
                 [](unsigned char value) { return static_cast<std::byte>(value); });
  return output;
}

bool BigInteger::is_negative() const noexcept { return impl_->value < 0; }

bool BigInteger::is_zero() const noexcept { return impl_->value == 0; }

bool BigInteger::fits_uint64() const noexcept {
  return impl_->value >= 0 && impl_->value <= std::numeric_limits<std::uint64_t>::max();
}

bool BigInteger::fits_int64() const noexcept {
  return impl_->value >= std::numeric_limits<std::int64_t>::min() &&
         impl_->value <= std::numeric_limits<std::int64_t>::max();
}

Result<std::uint64_t> BigInteger::to_uint64() const {
  if (!fits_uint64()) {
    return std::unexpected(CardanoError(ErrorCode::out_of_range, "integer does not fit uint64"));
  }
  return impl_->value.convert_to<std::uint64_t>();
}

Result<std::int64_t> BigInteger::to_int64() const {
  if (!fits_int64()) {
    return std::unexpected(CardanoError(ErrorCode::out_of_range, "integer does not fit int64"));
  }
  return impl_->value.convert_to<std::int64_t>();
}

BigInteger& BigInteger::operator+=(const BigInteger& rhs) {
  impl_->value += rhs.impl_->value;
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& rhs) {
  impl_->value -= rhs.impl_->value;
  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& rhs) {
  impl_->value *= rhs.impl_->value;
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& rhs) {
  impl_->value /= rhs.impl_->value;
  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& rhs) {
  impl_->value %= rhs.impl_->value;
  return *this;
}

BigInteger operator-(const BigInteger& value) {
  BigInteger result(value);
  result.impl_->value = -result.impl_->value;
  return result;
}

bool operator==(const BigInteger& left, const BigInteger& right) noexcept {
  return left.impl_->value == right.impl_->value;
}

std::strong_ordering operator<=>(const BigInteger& left, const BigInteger& right) noexcept {
  if (left.impl_->value < right.impl_->value) {
    return std::strong_ordering::less;
  }
  if (left.impl_->value > right.impl_->value) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

Result<std::int64_t> as_int64(const BigInteger& value) { return value.to_int64(); }

Result<std::uint64_t> as_uint64(const BigInteger& value) { return value.to_uint64(); }

}  // namespace cardano::core
