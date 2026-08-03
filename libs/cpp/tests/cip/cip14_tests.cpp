#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "cardano/cardano.hpp"
#include "cardano/cip/cip14.hpp"

TEST_CASE("CIP-14 matches captured asset fingerprint vectors") {
  struct Vector {
    std::string_view policy;
    std::string_view name;
    std::string_view fingerprint;
  };
  const std::array vectors{
      Vector{"7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "",
             "asset1rjklcrnsdzqp65wjgrg55sy9723kw09mlgvlc3"},
      Vector{"7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc37e", "",
             "asset1nl0puwxmhas8fawxp8nx4e2q3wekg969n2auw3"},
      Vector{"1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209", "",
             "asset1uyuxku60yqe57nusqzjx38aan3f2wq6s93f6ea"},
      Vector{"7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373", "504154415445",
             "asset13n25uv0yaf5kus35fm2k86cqy60z58d9xmde92"},
      Vector{"1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209", "504154415445",
             "asset1hv4p5tv2a837mzqrst04d0dcptdjmluqvdx9k3"},
      Vector{"1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209",
             "7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373",
             "asset1aqrdypg669jgazruv5ah07nuyqe0wxjhe2el6f"},
      Vector{"7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373",
             "1e349c9bdea19fd6c147626a5260bc44b71635f398b67c59881df209",
             "asset17jd78wukhtrnmjh3fngzasxm8rck0l2r4hhyyt"},
      Vector{"7eae28af2208be856f7a119668ae52a49b73725e326dc16579dcc373",
             "0000000000000000000000000000000000000000000000000000000000000000",
             "asset1pkpwyknlvul7az0xx8czhl60pyel45rpje4z8w"},
  };
  for (const auto& vector : vectors) {
    const auto policy = *cardano::crypto::ScriptHash::from_hex(vector.policy);
    const auto name = *cardano::chain::AssetName::from_hex(vector.name);
    const auto fingerprint = cardano::cip::AssetFingerprint::from_asset(policy, name);
    CHECK(fingerprint.to_bech32() == vector.fingerprint);
    CHECK(*cardano::cip::AssetFingerprint::from_bech32(fingerprint.to_bech32()) == fingerprint);
    CHECK(fingerprint.to_bytes().size() == 20);
  }
  CHECK_FALSE(cardano::cip::AssetFingerprint::from_bech32("stake1qqqqqq"));
  CHECK_FALSE(
      cardano::cip::AssetFingerprint::from_bech32("ASSET1RJKLCRNSDZQP65WJGRG55SY9723KW09MLGVLC3"));
}
