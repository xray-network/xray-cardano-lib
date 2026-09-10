#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "cardano/plutus/data.hpp"
#include "cardano/plutus/uplc.hpp"
namespace cardano::plutus {
struct BlueprintLimits {
  std::size_t max_depth{128};
  std::size_t max_nodes{100'000};
  std::size_t max_string_bytes{16U * 1024U * 1024U};
  std::size_t max_declarations{10'000};
  std::size_t max_evaluation_steps{1'000'000};
};
struct BlueprintDiagnostic {
  std::string code;
  std::string path;
  std::string message;
};
struct BlueprintCompiler {
  std::string name;
  std::optional<std::string> version;
};
struct BlueprintPreamble {
  std::string title;
  std::string version;
  std::string plutus_version;
  std::optional<std::string> description;
  std::optional<BlueprintCompiler> compiler;
  std::optional<std::string> license;
};
struct BlueprintSchema {
  std::string json;
};
struct BlueprintArgument {
  BlueprintSchema schema;
  std::vector<std::string> purposes;
  std::optional<std::string> title;
};
using BlueprintParameter = BlueprintArgument;
struct BlueprintValidator {
  std::string title;
  BlueprintArgument redeemer;
  std::optional<BlueprintArgument> datum;
  std::vector<BlueprintParameter> parameters;
  std::optional<std::string> description;
  std::optional<std::string> compiled_code;
  std::optional<std::string> hash;
};
class ContractBlueprint {
 public:
  [[nodiscard]] static core::Result<ContractBlueprint> parse(std::string_view json,
                                                             BlueprintLimits limits = {});
  [[nodiscard]] const BlueprintPreamble& preamble() const noexcept;
  [[nodiscard]] const std::vector<BlueprintValidator>& validators() const noexcept;
  [[nodiscard]] const std::vector<std::pair<std::string, BlueprintSchema>>& definitions()
      const noexcept;
  [[nodiscard]] std::string to_json() const;
  [[nodiscard]] std::vector<BlueprintDiagnostic> validate_data(const BlueprintSchema& schema,
                                                               const Data& value,
                                                               BlueprintLimits limits = {}) const;
  [[nodiscard]] std::vector<BlueprintDiagnostic> validate_constant(
      const BlueprintSchema& schema, const UplcConstant& value, BlueprintLimits limits = {}) const;

 private:
  ContractBlueprint(BlueprintPreamble preamble, std::vector<BlueprintValidator> validators,
                    std::vector<std::pair<std::string, BlueprintSchema>> definitions,
                    std::string json);
  BlueprintPreamble preamble_;
  std::vector<BlueprintValidator> validators_;
  std::vector<std::pair<std::string, BlueprintSchema>> definitions_;
  std::string json_;
};
}  // namespace cardano::plutus
