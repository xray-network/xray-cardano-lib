#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "cardano/core/cbor.hpp"
namespace cardano::cip {
enum class Cip21Era : std::uint8_t { shelley, allegra, mary, alonzo, babbage, conway };
enum class Cip21AuxiliaryMode : std::uint8_t { hash_only, catalyst_registration };
struct Cip21Context {
  Cip21AuxiliaryMode auxiliary_mode{Cip21AuxiliaryMode::hash_only};
};
struct Cip21Limits {
  core::cbor::DecoderLimits cbor{};
  std::size_t max_diagnostics{100'000};
};
enum class Cip21Severity : std::uint8_t { error, advisory };
enum class Cip21Code : std::uint8_t {
  noncanonical_integer,
  noncanonical_length,
  indefinite_item,
  map_key_order,
  inconsistent_set_tag,
  unsupported_body_entry,
  integer_range,
  element_count,
  voting_cardinality,
  empty_optional_collection,
  legacy_output_value_shape,
  empty_inline_datum,
  empty_reference_script,
  duplicate_policy,
  duplicate_asset,
  unsupported_certificate,
  pool_registration_combination,
  duplicate_withdrawal,
  catalyst_auxiliary_shape,
  legacy_output_format,
  diagnostic_limit
};
struct Cip21Diagnostic {
  Cip21Code code;
  Cip21Severity severity;
  std::string path;
  std::optional<std::size_t> byte_offset;
  std::string message;
};
struct Cip21Report {
  std::vector<Cip21Diagnostic> diagnostics;
  [[nodiscard]] bool compatible() const noexcept;
};
[[nodiscard]] core::Result<Cip21Report> diagnose_cip21(Cip21Era era, core::ByteSpan transaction,
                                                       Cip21Context context = {},
                                                       Cip21Limits limits = {});
}  // namespace cardano::cip
