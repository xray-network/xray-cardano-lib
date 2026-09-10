#include <catch2/catch_test_macros.hpp>

#include "cardano/cip/cip129.hpp"

TEST_CASE("provisional CIP-129 identifiers validate headers and bounded action indexes") {
  using namespace cardano;
  const auto key_hash = *crypto::Ed25519KeyHash::from_bytes(core::Bytes(28));
  const auto credential = cip::experimental::cip129::CredentialId::key(
      cip::experimental::cip129::CredentialRole::drep, key_hash);
  const auto encoded = credential.to_bech32();
  const auto decoded = cip::experimental::cip129::CredentialId::from_bech32(encoded);
  REQUIRE(decoded);
  CHECK(decoded->role() == cip::experimental::cip129::CredentialRole::drep);
  CHECK(decoded->kind() == cip::experimental::cip129::CredentialKind::key_hash);

  const auto transaction = *crypto::TransactionHash::from_bytes(core::Bytes(32));
  CHECK(cip::experimental::cip129::GovernanceActionId::make(transaction, 0));
  CHECK(cip::experimental::cip129::GovernanceActionId::make(transaction, 255));
  CHECK_FALSE(cip::experimental::cip129::GovernanceActionId::make(transaction, 256));
  const auto action = *cip::experimental::cip129::GovernanceActionId::make(transaction, 17);
  CHECK(cip::experimental::cip129::GovernanceActionId::from_bech32(action.to_bech32())->index() ==
        17);
  CHECK_FALSE(cip::experimental::cip129::parse_legacy_cip105(encoded));
}
