#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <fstream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <string_view>

#include "cardano/plutus/uplc.hpp"

using namespace cardano;

namespace {

std::string decode_base64(std::string_view input) {
  static constexpr std::string_view alphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string output;
  std::uint32_t buffer = 0U;
  unsigned bits = 0U;
  for (const char character : input) {
    if (character == '=') break;
    const auto position = alphabet.find(character);
    if (position == std::string_view::npos) continue;
    buffer = (buffer << 6U) | static_cast<std::uint32_t>(position);
    bits += 6U;
    if (bits >= 8U) {
      bits -= 8U;
      output.push_back(static_cast<char>((buffer >> bits) & 0xffU));
    }
  }
  return output;
}

std::map<std::string, std::string> conformance_files() {
  const auto path = std::string(CARDANO_REPOSITORY_ROOT) +
                    "/updates/providers/uplc/0001-uplc/"
                    "artifacts/conformance/corpus.json";
  std::ifstream stream(path);
  REQUIRE(stream.good());
  nlohmann::json corpus;
  stream >> corpus;
  std::map<std::string, std::string> files;
  for (const auto& entry : corpus.at("entries")) {
    files.emplace(entry.at("path").get<std::string>(),
                  decode_base64(entry.at("contentBase64").get<std::string>()));
  }
  return files;
}

}  // namespace

TEST_CASE("official UPLC conformance corpus matches all applicable programs",
          "[plutus][uplc][conformance][provider]") {
  const auto files = conformance_files();
  std::size_t programs = 0U;
  std::size_t budgets = 0U;
  for (const auto& [path, source] : files) {
    if (!std::string_view(path).ends_with(".uplc")) continue;
    ++programs;
    CAPTURE(path);
    const auto expected = files.find(path + ".expected");
    REQUIRE(expected != files.end());

    const auto program = plutus::parse_uplc_text(source);
    if (expected->second == "parse error") {
      CHECK_FALSE(program);
      continue;
    }
    REQUIRE(program);
    const auto evaluated = plutus::evaluate_program(
        *program,
        {std::numeric_limits<std::int64_t>::max(), std::numeric_limits<std::int64_t>::max()});
    if (expected->second == "evaluation failure") {
      CHECK_FALSE(evaluated);
      continue;
    }
    REQUIRE(evaluated);
    const auto expected_program = plutus::parse_uplc_text(expected->second);
    REQUIRE(expected_program);
    CHECK(evaluated->result == expected_program->term);

    const auto expected_budget = files.find(path + ".budget.expected");
    if (expected_budget != files.end()) {
      const std::regex pattern(R"(\(\{cpu: ([0-9]+)\n\| mem: ([0-9]+)\}\))");
      std::smatch match;
      REQUIRE(std::regex_match(expected_budget->second, match, pattern));
      const auto cpu = std::stoll(match[1].str());
      const auto memory = std::stoll(match[2].str());
      CHECK(evaluated->spent.cpu == cpu);
      CHECK(evaluated->spent.memory == memory);
      ++budgets;
    }
  }
  CHECK(programs == 1'003U);
  CHECK(budgets == 721U);
}
