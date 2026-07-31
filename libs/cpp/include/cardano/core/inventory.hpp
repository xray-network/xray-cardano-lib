#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "cardano/core/bytes.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/error.hpp"

namespace cardano::core {

using ByteArray = Bytes;
using CardanoErrorCode = ErrorCode;

struct CardanoErrorOptions {
  std::optional<std::size_t> offset;
  std::string path;
};

template <typename T>
using CardanoResult = Result<T>;
using DeserializeFailure = CardanoError;
using DeserializeError = CardanoError;
using CardanoBoundsError = CardanoError;

template <typename T>
concept Cloneable = std::copy_constructible<T>;

template <typename T>
using Comparator = std::function<int(const T&, const T&)>;

template <typename T>
concept Equatable = std::equality_comparable<T>;

using CborByteChunk = cbor::ByteChunk;
using CborDecoderLimits = cbor::DecoderLimits;
using CborHeadEncoding = cbor::HeadWidth;
using CborHeadWidth = cbor::HeadWidth;
using CborLengthEncoding = cbor::LengthEncoding;
using CborMode = cbor::Mode;
using CborSpan = cbor::Span;
using CborStringEncoding = cbor::ByteStringEncoding;
using CborTextChunk = cbor::TextChunk;
using CborValue = cbor::Value;
using DecodeCborOptions = cbor::DecodeOptions;
using EncodeCborOptions = cbor::EncodeOptions;

}  // namespace cardano::core
