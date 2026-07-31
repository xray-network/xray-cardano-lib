#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cardano/core/big_integer.hpp"
#include "cardano/core/cbor.hpp"
#include "cardano/core/error.hpp"

namespace cardano::cip::cip8 {

enum class AlgorithmId : std::int32_t { ed_dsa = -8, chacha20_poly1305 = 24 };

enum class KeyType : std::int32_t { okp = 1, ec2 = 2, symmetric = 4 };

enum class ECKey : std::int32_t { crv = -1, x = -2, y = -3, d = -4 };

enum class CurveType : std::int32_t {
  p256 = 1,
  p384 = 2,
  p521 = 3,
  x25519 = 4,
  x448 = 5,
  ed25519 = 6,
  ed448 = 7
};

enum class KeyOperation : std::int32_t {
  sign = 1,
  verify = 2,
  encrypt = 3,
  decrypt = 4,
  wrap_key = 5,
  unwrap_key = 6,
  derive_key = 7,
  derive_bits = 8
};

enum class LabelKind { integer, text };

class Label {
 public:
  explicit Label(core::BigInteger integer);
  explicit Label(std::string text);
  explicit Label(std::int64_t integer);
  explicit Label(AlgorithmId value);
  explicit Label(KeyType value);
  explicit Label(ECKey value);
  explicit Label(CurveType value);
  explicit Label(KeyOperation value);

  [[nodiscard]] LabelKind kind() const noexcept;
  [[nodiscard]] const core::BigInteger* as_integer() const noexcept;
  [[nodiscard]] const std::string* as_text() const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value() const;
  [[nodiscard]] static core::Result<Label> from_cbor_value(const core::cbor::Value& value);
  friend bool operator==(const Label&, const Label&) = default;

 private:
  std::variant<core::BigInteger, std::string> value_;
};

using Labels = std::vector<Label>;

class HeaderMap {
 public:
  HeaderMap();
  [[nodiscard]] static core::Result<HeaderMap> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<HeaderMap> from_cbor_value(const core::cbor::Value& value);

  [[nodiscard]] std::optional<core::cbor::Value> header(const Label& label) const;
  [[nodiscard]] core::VoidResult set_header(Label label, core::cbor::Value value);
  [[nodiscard]] Labels keys() const;

  void set_algorithm_id(Label value);
  [[nodiscard]] std::optional<Label> algorithm_id() const;
  void set_criticality(Labels value);
  [[nodiscard]] std::optional<Labels> criticality() const;
  void set_content_type(Label value);
  [[nodiscard]] std::optional<Label> content_type() const;
  void set_key_id(core::Bytes value);
  [[nodiscard]] std::optional<core::Bytes> key_id() const;

  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;

 private:
  explicit HeaderMap(core::cbor::Value value);
  core::cbor::Value value_;
};

class ProtectedHeaderMap {
 public:
  ProtectedHeaderMap();
  explicit ProtectedHeaderMap(const HeaderMap& headers);
  [[nodiscard]] static core::Result<ProtectedHeaderMap> from_bytes(core::ByteSpan serialized_map);
  [[nodiscard]] const core::Bytes& bytes() const noexcept;
  [[nodiscard]] core::Result<HeaderMap> deserialized_headers() const;
  friend bool operator==(const ProtectedHeaderMap&, const ProtectedHeaderMap&) = default;

 private:
  explicit ProtectedHeaderMap(core::Bytes bytes, bool);
  core::Bytes bytes_;
};

struct Headers {
  ProtectedHeaderMap protected_headers;
  HeaderMap unprotected;
};

enum class SigContext { signature, signature1, counter_signature };

class SigStructure {
 public:
  SigStructure(SigContext context, ProtectedHeaderMap body_protected, core::Bytes external_aad,
               core::Bytes payload);
  void set_sign_protected(ProtectedHeaderMap protected_headers);

  [[nodiscard]] SigContext context() const noexcept;
  [[nodiscard]] const ProtectedHeaderMap& body_protected() const noexcept;
  [[nodiscard]] const std::optional<ProtectedHeaderMap>& sign_protected() const noexcept;
  [[nodiscard]] const core::Bytes& external_aad() const noexcept;
  [[nodiscard]] const core::Bytes& payload() const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value() const;
  [[nodiscard]] core::Result<core::Bytes> to_bytes() const;
  [[nodiscard]] static core::Result<SigStructure> from_bytes(core::ByteSpan bytes);

 private:
  SigContext context_;
  ProtectedHeaderMap body_protected_;
  std::optional<ProtectedHeaderMap> sign_protected_;
  core::Bytes external_aad_;
  core::Bytes payload_;
};

struct COSESignature {
  Headers headers;
  core::Bytes signature;
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] static core::Result<COSESignature> from_cbor_value(const core::cbor::Value& value);
};

using COSESignatures = std::vector<COSESignature>;

class CounterSignature {
 public:
  explicit CounterSignature(COSESignature signature);
  explicit CounterSignature(COSESignatures signatures);
  [[nodiscard]] const COSESignatures& signatures() const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] static core::Result<CounterSignature> from_cbor_value(
      const core::cbor::Value& value);

 private:
  COSESignatures signatures_;
};

class COSESign1 {
 public:
  COSESign1(Headers headers, std::optional<core::Bytes> payload, core::Bytes signature);
  [[nodiscard]] static core::Result<COSESign1> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<COSESign1> from_cbor_value(const core::cbor::Value& value);

  [[nodiscard]] const Headers& headers() const noexcept;
  [[nodiscard]] const std::optional<core::Bytes>& payload() const noexcept;
  [[nodiscard]] const core::Bytes& signature() const noexcept;
  [[nodiscard]] core::Result<SigStructure> signed_data(
      core::ByteSpan external_aad = {},
      std::optional<core::Bytes> external_payload = std::nullopt) const;
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;

 private:
  Headers headers_;
  std::optional<core::Bytes> payload_;
  core::Bytes signature_;
  std::optional<core::cbor::Value> preserved_;
};

class COSESign {
 public:
  COSESign(Headers headers, std::optional<core::Bytes> payload, COSESignatures signatures);
  [[nodiscard]] static core::Result<COSESign> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<COSESign> from_cbor_value(const core::cbor::Value& value);

  [[nodiscard]] const Headers& headers() const noexcept;
  [[nodiscard]] const std::optional<core::Bytes>& payload() const noexcept;
  [[nodiscard]] const COSESignatures& signatures() const noexcept;
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;

 private:
  Headers headers_;
  std::optional<core::Bytes> payload_;
  COSESignatures signatures_;
  std::optional<core::cbor::Value> preserved_;
};

class COSEKey {
 public:
  explicit COSEKey(Label key_type);
  [[nodiscard]] static core::Result<COSEKey> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<COSEKey> from_cbor_value(const core::cbor::Value& value);
  [[nodiscard]] std::optional<core::cbor::Value> header(const Label& label) const;
  [[nodiscard]] core::VoidResult set_header(Label label, core::cbor::Value value);
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::cbor::Value to_cbor_value(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;

 private:
  HeaderMap map_;
};

class EdDSA25519Key {
 public:
  explicit EdDSA25519Key(core::Bytes public_key);
  void is_for_signing() noexcept;
  void is_for_verifying() noexcept;
  [[nodiscard]] core::Result<COSEKey> build() const;

 private:
  core::Bytes public_key_;
  bool for_signing_{false};
  bool for_verifying_{false};
};

class COSESign1Builder {
 public:
  COSESign1Builder(Headers headers, core::Bytes payload, bool payload_external);
  void hash_payload();
  void set_external_aad(core::Bytes external_aad);
  [[nodiscard]] SigStructure make_data_to_sign() const;
  [[nodiscard]] COSESign1 build(core::Bytes signature) const;

 private:
  Headers headers_;
  core::Bytes payload_;
  core::Bytes external_aad_;
  bool payload_external_;
  bool hashed_{false};
};

class COSESignBuilder {
 public:
  COSESignBuilder(Headers headers, core::Bytes payload, bool payload_external);
  void hash_payload();
  void set_external_aad(core::Bytes external_aad);
  [[nodiscard]] SigStructure make_data_to_sign() const;
  [[nodiscard]] COSESign build(COSESignatures signatures) const;

 private:
  Headers headers_;
  core::Bytes payload_;
  core::Bytes external_aad_;
  bool payload_external_;
  bool hashed_{false};
};

enum class SignedMessageKind { cose_sign, cose_sign1 };

class SignedMessage {
 public:
  explicit SignedMessage(COSESign sign);
  explicit SignedMessage(COSESign1 sign1);
  [[nodiscard]] static core::Result<SignedMessage> from_bytes(core::ByteSpan bytes);
  [[nodiscard]] static core::Result<SignedMessage> from_user_facing_encoding(
      std::string_view encoded);
  [[nodiscard]] SignedMessageKind kind() const noexcept;
  [[nodiscard]] const COSESign* as_cose_sign() const noexcept;
  [[nodiscard]] const COSESign1* as_cose_sign1() const noexcept;
  [[nodiscard]] core::Result<core::Bytes> to_bytes(
      core::cbor::Mode mode = core::cbor::Mode::preserve) const;
  [[nodiscard]] core::Result<std::string> to_user_facing_encoding() const;

 private:
  std::variant<COSESign, COSESign1> value_;
};

}  // namespace cardano::cip::cip8
