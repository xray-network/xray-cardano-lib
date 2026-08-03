#include "cardano/cip/cip21.hpp"

#include <algorithm>
#include <set>
#include <tuple>

#include "cardano/chain/era_models.hpp"
namespace cardano::cip {
namespace {
using namespace core::cbor;
HeadWidth shortest(const core::BigInteger& value) {
  if (!value.fits_uint64()) return HeadWidth::eight;
  const auto number = *value.to_uint64();
  return number < 24               ? HeadWidth::inline_value
         : number <= 0xff          ? HeadWidth::one
         : number <= 0xffff        ? HeadWidth::two
         : number <= 0xffffffffULL ? HeadWidth::four
                                   : HeadWidth::eight;
}
struct Walker {
  Cip21Report report;
  std::size_t maximum{};
  bool limited{};
  void add(Cip21Code code, std::string path, const Value& value, std::string message,
           Cip21Severity severity = Cip21Severity::error) {
    if (report.diagnostics.size() >= maximum) {
      limited = true;
      return;
    }
    report.diagnostics.push_back({code, severity, std::move(path),
                                  value.span() ? std::optional(value.span()->start) : std::nullopt,
                                  std::move(message)});
  }
  void walk(const Value& value, const std::string& path) {
    if (const auto* number = value.as_unsigned()) {
      if (number->width != shortest(number->value))
        add(Cip21Code::noncanonical_integer, path, value, "integer does not use its shortest head");
      if (!number->value.fits_uint64())
        add(Cip21Code::integer_range, path, value, "unsigned integer exceeds uint64");
    }
    if (const auto* number = value.as_negative()) {
      auto magnitude = -number->value - core::BigInteger(std::uint64_t{1});
      if (number->width != shortest(magnitude))
        add(Cip21Code::noncanonical_integer, path, value, "integer does not use its shortest head");
      if (!number->value.fits_int64())
        add(Cip21Code::integer_range, path, value, "signed integer exceeds int64");
    }
    if (const auto* bytes = value.as_byte_string()) {
      if (bytes->encoding.indefinite)
        add(Cip21Code::indefinite_item, path, value, "indefinite byte string");
      else if (bytes->encoding.width !=
               shortest(core::BigInteger(static_cast<std::uint64_t>(bytes->value.size()))))
        add(Cip21Code::noncanonical_length, path, value, "noncanonical byte-string length");
    }
    if (const auto* text = value.as_text_string()) {
      if (text->encoding.indefinite)
        add(Cip21Code::indefinite_item, path, value, "indefinite text string");
      else if (text->encoding.width !=
               shortest(core::BigInteger(static_cast<std::uint64_t>(text->value.size()))))
        add(Cip21Code::noncanonical_length, path, value, "noncanonical text length");
    }
    if (const auto* array = value.as_array()) {
      if (array->encoding.indefinite)
        add(Cip21Code::indefinite_item, path, value, "indefinite array");
      else if (array->encoding.width !=
               shortest(core::BigInteger(static_cast<std::uint64_t>(array->values.size()))))
        add(Cip21Code::noncanonical_length, path, value, "noncanonical array length");
      for (std::size_t i = 0; i < array->values.size(); ++i)
        walk(array->values[i], path + "/" + std::to_string(i));
    }
    if (const auto* map = value.as_map()) {
      if (map->encoding.indefinite)
        add(Cip21Code::indefinite_item, path, value, "indefinite map");
      else if (map->encoding.width !=
               shortest(core::BigInteger(static_cast<std::uint64_t>(map->entries.size()))))
        add(Cip21Code::noncanonical_length, path, value, "noncanonical map length");
      core::Bytes prior;
      for (std::size_t i = 0; i < map->entries.size(); ++i) {
        const auto encoded = *encode_cbor(map->entries[i].first, {.mode = Mode::canonical});
        if (!prior.empty() &&
            !(prior.size() < encoded.size() || (prior.size() == encoded.size() && prior < encoded)))
          add(Cip21Code::map_key_order, path + "/" + std::to_string(i), map->entries[i].first,
              "map key is not canonically ordered");
        prior = encoded;
        walk(map->entries[i].first, path + "/" + std::to_string(i) + "/key");
        walk(map->entries[i].second, path + "/" + std::to_string(i));
      }
    }
    if (const auto* tag = value.as_tag()) {
      if (tag->width != shortest(tag->tag))
        add(Cip21Code::noncanonical_integer, path, value, "tag does not use its shortest head");
      walk(*tag->value, path);
    }
  }
};

[[nodiscard]] std::optional<std::uint64_t> unsigned_number(const Value& value) {
  const auto* number = value.as_unsigned();
  if (number == nullptr || !number->value.fits_uint64()) return std::nullopt;
  return *number->value.to_uint64();
}

[[nodiscard]] const Value* body_field(const MapValue& map, std::uint64_t key) {
  for (const auto& [candidate, value] : map.entries) {
    if (unsigned_number(candidate) == key) return &value;
  }
  return nullptr;
}

struct CollectionView {
  const Value* value{};
  bool tagged{};
};

[[nodiscard]] CollectionView collection(const Value& value) {
  const auto* tag = value.as_tag();
  if (tag != nullptr && tag->value != nullptr && tag->tag == core::BigInteger(std::uint64_t{258})) {
    return {tag->value.get(), true};
  }
  return {&value, false};
}

[[nodiscard]] std::size_t collection_size(const Value& value) {
  if (const auto* array = value.as_array()) return array->values.size();
  if (const auto* map = value.as_map()) return map->entries.size();
  return 0U;
}

[[nodiscard]] bool empty_value(const Value& value) {
  if (const auto* array = value.as_array()) return array->values.empty();
  if (const auto* map = value.as_map()) return map->entries.empty();
  if (const auto* bytes = value.as_byte_string()) return bytes->value.empty();
  return false;
}

struct Analyzer {
  Walker& walker;
  std::vector<std::pair<std::string, bool>> set_tags;

  void limit(const Value& value, const std::string& path) {
    const auto view = collection(value);
    if (collection_size(*view.value) > 65'535U) {
      walker.add(Cip21Code::element_count, path, value, "collection exceeds 65535 elements");
    }
  }

  [[nodiscard]] const Value* semantic_set(const Value* value, const std::string& path) {
    if (value == nullptr) return nullptr;
    const auto view = collection(*value);
    set_tags.emplace_back(path, view.tagged);
    limit(*value, path);
    return view.value;
  }

  void duplicates(const Value& value, const std::string& path, Cip21Code code) {
    const auto* map = value.as_map();
    if (map == nullptr) return;
    std::set<std::string> seen;
    for (std::size_t index = 0; index < map->entries.size(); ++index) {
      auto encoded = encode_cbor(map->entries[index].first, {.mode = Mode::canonical});
      if (!encoded) continue;
      if (!seen.insert(core::bytes_to_hex(*encoded)).second) {
        walker.add(code, path + "/" + std::to_string(index), map->entries[index].first,
                   "duplicate map key");
      }
    }
  }

  void multiasset(const Value& value, const std::string& path) {
    const auto* policies = value.as_map();
    if (policies == nullptr) return;
    limit(value, path);
    duplicates(value, path, Cip21Code::duplicate_policy);
    for (std::size_t index = 0; index < policies->entries.size(); ++index) {
      const auto asset_path = path + "/" + std::to_string(index);
      limit(policies->entries[index].second, asset_path);
      duplicates(policies->entries[index].second, asset_path, Cip21Code::duplicate_asset);
    }
  }

  [[nodiscard]] bool output(const Value& value, const std::string& path) {
    bool conflict = false;
    if (const auto* fields = value.as_array()) {
      walker.add(Cip21Code::legacy_output_format, path, value,
                 "post-Alonzo map output is preferred", Cip21Severity::advisory);
      if (fields->values.size() > 1U) {
        const auto* amount = fields->values[1].as_array();
        if (amount != nullptr && amount->values.size() > 1U) {
          multiasset(amount->values[1], path + "/amount/assets");
          if (amount->values[1].as_map() != nullptr &&
              amount->values[1].as_map()->entries.empty()) {
            walker.add(Cip21Code::legacy_output_value_shape, path + "/amount", fields->values[1],
                       "legacy coin-only output must encode coin directly");
          }
        }
      }
      return fields->values.size() > 2U;
    }
    const auto* map = value.as_map();
    if (map == nullptr) return false;
    if (const auto* amount = body_field(*map, 1U); amount != nullptr) {
      const auto* pair = amount->as_array();
      if (pair != nullptr && pair->values.size() > 1U)
        multiasset(pair->values[1], path + "/amount/assets");
    }
    if (const auto* datum = body_field(*map, 2U); datum != nullptr) {
      conflict = true;
      const auto* option = datum->as_array();
      if (option != nullptr && option->values.size() > 1U &&
          unsigned_number(option->values[0]) == 1U && empty_value(option->values[1])) {
        walker.add(Cip21Code::empty_inline_datum, path + "/datum", *datum,
                   "inline datum content must be nonempty");
      }
    }
    if (const auto* script = body_field(*map, 3U); script != nullptr) {
      conflict = true;
      const auto view = script->as_tag() != nullptr && script->as_tag()->value
                            ? script->as_tag()->value.get()
                            : script;
      if (empty_value(*view))
        walker.add(Cip21Code::empty_reference_script, path + "/scriptRef", *script,
                   "reference script content must be nonempty");
    }
    return conflict;
  }

  void analyze(const Value& transaction) {
    const auto* tx = transaction.as_array();
    if (tx == nullptr || tx->values.empty() || tx->values[0].as_map() == nullptr) return;
    const auto& body = *tx->values[0].as_map();
    for (const auto& [key, value] : body.entries) {
      const auto number = unsigned_number(key);
      if (!number) continue;
      const auto path = "/body/" + std::to_string(*number);
      if (*number == 6U || *number == 20U)
        walker.add(Cip21Code::unsupported_body_entry, path, key, "unsupported CIP-21 body entry");
      static const std::set<std::uint64_t> optional{4, 5, 7, 8, 9, 11, 13, 14, 18, 19, 20};
      if (optional.contains(*number) && empty_value(*collection(value).value)) {
        walker.add(Cip21Code::empty_optional_collection, path, value,
                   "optional collection must be absent instead of empty");
      }
    }
    static_cast<void>(semantic_set(body_field(body, 0U), "/body/0"));
    const auto* certificates = semantic_set(body_field(body, 4U), "/body/4");
    static_cast<void>(semantic_set(body_field(body, 13U), "/body/13"));
    static_cast<void>(semantic_set(body_field(body, 14U), "/body/14"));
    static_cast<void>(semantic_set(body_field(body, 18U), "/body/18"));
    bool output_conflict = false;
    if (const auto* outputs = body_field(body, 1U); outputs != nullptr) {
      limit(*outputs, "/body/1");
      if (const auto* values = outputs->as_array())
        for (std::size_t index = 0; index < values->values.size(); ++index)
          output_conflict =
              output(values->values[index], "/body/1/" + std::to_string(index)) || output_conflict;
    }
    if (const auto* mint = body_field(body, 9U); mint != nullptr) multiasset(*mint, "/body/9");
    if (const auto* withdrawals = body_field(body, 5U); withdrawals != nullptr) {
      limit(*withdrawals, "/body/5");
      duplicates(*withdrawals, "/body/5", Cip21Code::duplicate_withdrawal);
    }
    if (const auto* votes = body_field(body, 19U); votes != nullptr && votes->as_map() != nullptr) {
      if (votes->as_map()->entries.size() > 1U)
        walker.add(Cip21Code::voting_cardinality, "/body/19", *votes,
                   "at most one voter is allowed");
      for (std::size_t index = 0; index < votes->as_map()->entries.size(); ++index)
        if (const auto* procedures = votes->as_map()->entries[index].second.as_map();
            procedures != nullptr && procedures->entries.size() > 1U)
          walker.add(Cip21Code::voting_cardinality, "/body/19/" + std::to_string(index),
                     votes->as_map()->entries[index].second, "at most one vote is allowed");
    }
    bool pool_registration = false;
    const auto* certificate_array = certificates == nullptr ? nullptr : certificates->as_array();
    if (certificate_array != nullptr)
      for (std::size_t index = 0; index < certificate_array->values.size(); ++index) {
        const auto& certificate = certificate_array->values[index];
        const auto* fields = certificate.as_array();
        if (fields == nullptr || fields->values.empty()) continue;
        const auto kind = unsigned_number(fields->values[0]);
        if (!kind) continue;
        const auto path = "/body/4/" + std::to_string(index);
        static const std::set<std::uint64_t> unsupported{5, 6, 10, 11, 12, 13};
        if (unsupported.contains(*kind))
          walker.add(Cip21Code::unsupported_certificate, path, certificate,
                     "certificate is unsupported by CIP-21");
        if (*kind == 3U) {
          pool_registration = true;
          if (fields->values.size() > 1U && fields->values[1].as_array() != nullptr) {
            const auto& parameters = fields->values[1].as_array()->values;
            if (parameters.size() > 6U)
              static_cast<void>(semantic_set(&parameters[6], path + "/owners"));
            if (parameters.size() > 7U) limit(parameters[7], path + "/relays");
          }
        }
      }
    if (pool_registration) {
      if (certificate_array != nullptr && certificate_array->values.size() != 1U)
        for (std::size_t index = 0; index < certificate_array->values.size(); ++index)
          walker.add(Cip21Code::pool_registration_combination, "/body/4/" + std::to_string(index),
                     certificate_array->values[index],
                     "pool registration must be the only certificate");
      for (const auto key : {5U, 9U, 11U, 13U, 14U, 16U, 17U, 18U, 19U, 21U, 22U})
        if (const auto* field = body_field(body, key); field != nullptr)
          walker.add(Cip21Code::pool_registration_combination, "/body/" + std::to_string(key),
                     *field, "body entry cannot accompany pool registration");
      if (output_conflict)
        walker.add(Cip21Code::pool_registration_combination, "/body/1", tx->values[0],
                   "output datum or reference script cannot accompany pool registration");
    }
    if (tx->values.size() > 1U && tx->values[1].as_map() != nullptr) {
      std::size_t witnesses = 0U;
      for (const auto& [key, value] : tx->values[1].as_map()->entries) {
        const auto number = unsigned_number(key);
        if (!number) continue;
        const auto* collection_value =
            *number == 5U ? collection(value).value
                          : semantic_set(&value, "/witnessSet/" + std::to_string(*number));
        witnesses += collection_size(*collection_value);
      }
      if (witnesses > 65'535U)
        walker.add(Cip21Code::element_count, "/witnessSet", tx->values[1],
                   "total witnesses exceed 65535");
    }
    if (std::ranges::any_of(set_tags, [](const auto& item) { return item.second; }) &&
        std::ranges::any_of(set_tags, [](const auto& item) { return !item.second; })) {
      for (const auto& [path, unused] : set_tags) {
        static_cast<void>(unused);
        walker.add(Cip21Code::inconsistent_set_tag, path, transaction,
                   "semantic sets must consistently use or omit tag 258");
      }
    }
  }
};
}  // namespace
bool Cip21Report::compatible() const noexcept {
  return std::ranges::none_of(
      diagnostics, [](const auto& item) { return item.severity == Cip21Severity::error; });
}
core::Result<Cip21Report> diagnose_cip21(Cip21Era era, core::ByteSpan transaction,
                                         Cip21Context context, Cip21Limits limits) {
  auto decoded = core::cbor::decode_cbor(transaction, {.limits = limits.cbor});
  if (!decoded) return std::unexpected(decoded.error());
  core::VoidResult valid = std::monostate{};
  switch (era) {
    case Cip21Era::shelley: {
      auto model = chain::ShelleyTransaction::from_value(*decoded);
      if (!model) valid = std::unexpected(model.error());
      break;
    }
    case Cip21Era::allegra: {
      auto model = chain::AllegraTransaction::from_value(*decoded);
      if (!model) valid = std::unexpected(model.error());
      break;
    }
    case Cip21Era::mary: {
      auto model = chain::MaryTransaction::from_value(*decoded);
      if (!model) valid = std::unexpected(model.error());
      break;
    }
    case Cip21Era::alonzo: {
      auto model = chain::AlonzoTransaction::from_value(*decoded);
      if (!model) valid = std::unexpected(model.error());
      break;
    }
    case Cip21Era::babbage: {
      auto model = chain::BabbageTransaction::from_value(*decoded);
      if (!model) valid = std::unexpected(model.error());
      break;
    }
    case Cip21Era::conway: {
      auto model = chain::Transaction::from_value(*decoded);
      if (!model) valid = std::unexpected(model.error());
      break;
    }
  }
  if (!valid) return std::unexpected(valid.error());
  Walker walker{{}, limits.max_diagnostics, false};
  walker.walk(*decoded, "");
  Analyzer analyzer{walker, {}};
  analyzer.analyze(*decoded);
  if (context.auxiliary_mode == Cip21AuxiliaryMode::catalyst_registration) {
    const auto* tx = decoded->as_array();
    const auto* auxiliary = tx && tx->values.size() > 3 ? tx->values[3].as_array() : nullptr;
    if (auxiliary == nullptr || auxiliary->values.size() != 2 ||
        auxiliary->values[1].as_array() == nullptr ||
        !auxiliary->values[1].as_array()->values.empty())
      walker.add(Cip21Code::catalyst_auxiliary_shape, "/auxiliary", *decoded,
                 "Catalyst auxiliary data must be [metadata, []]");
  }
  std::ranges::sort(walker.report.diagnostics, [](const auto& left, const auto& right) {
    return std::tie(left.byte_offset, left.path, left.code) <
           std::tie(right.byte_offset, right.path, right.code);
  });
  if (walker.limited)
    walker.report.diagnostics.push_back({Cip21Code::diagnostic_limit, Cip21Severity::error, "",
                                         std::nullopt, "diagnostic limit reached"});
  return walker.report;
}
}  // namespace cardano::cip
