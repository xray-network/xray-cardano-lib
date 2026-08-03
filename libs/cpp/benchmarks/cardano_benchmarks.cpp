#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "cardano/cardano.hpp"

namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] cardano::core::Bytes read_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input) throw std::runtime_error("cannot open explicit benchmark fixture: " + path);
  const auto size = input.tellg();
  if (size < 0) throw std::runtime_error("cannot size benchmark fixture: " + path);
  cardano::core::Bytes output(static_cast<std::size_t>(size));
  input.seekg(0);
  input.read(reinterpret_cast<char*>(output.data()), size);
  if (!input) throw std::runtime_error("cannot read benchmark fixture: " + path);
  return output;
}

template <typename Workload>
[[nodiscard]] std::vector<std::uint64_t> measure(std::size_t iterations, Workload workload) {
  workload();  // One required warm-up.
  std::vector<std::uint64_t> samples;
  samples.reserve(iterations);
  for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
    const auto start = Clock::now();
    workload();
    samples.push_back(static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count()));
  }
  std::ranges::sort(samples);
  return samples;
}

[[nodiscard]] std::uint64_t percentile(const std::vector<std::uint64_t>& samples,
                                       std::size_t numerator, std::size_t denominator) {
  const auto index = std::min(samples.size() - 1U,
                              (samples.size() * numerator + denominator - 1U) / denominator - 1U);
  return samples[index];
}

void emit(std::string_view workload, std::string_view identity, std::size_t iterations,
          const std::vector<std::uint64_t>& samples, bool first) {
  if (!first) std::cout << ',';
  std::cout << "{\"identity\":\"" << identity << "\",\"iterations\":" << iterations
            << ",\"medianNanoseconds\":" << percentile(samples, 1U, 2U)
            << ",\"p95Nanoseconds\":" << percentile(samples, 95U, 100U)
            << ",\"ownedAllocations\":null,\"peakResidentBytes\":null,\"workload\":\"" << workload
            << "\"}";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    std::string block_path;
    std::size_t iterations = 10U;
    for (int index = 1; index < argc; ++index) {
      const std::string_view argument(argv[index]);
      if (argument == "--block" && index + 1 < argc)
        block_path = argv[++index];
      else if (argument == "--iterations" && index + 1 < argc)
        iterations = std::stoull(argv[++index]);
      else if (argument == "--smoke")
        iterations = 10U;
      else
        throw std::invalid_argument(
            "usage: cardano_benchmarks --block PATH [--iterations N|--smoke]");
    }
    if (block_path.empty() || iterations < 10U)
      throw std::invalid_argument(
          "an explicit block fixture and at least ten iterations are required");
    const auto block_bytes = read_file(block_path);
    const auto block_identity =
        cardano::core::bytes_to_hex(cardano::crypto::blake2b256(block_bytes));
    const auto block_samples = measure(iterations, [&] {
      auto block = cardano::chain::MultiEraBlock::from_cbor(block_bytes);
      if (!block) throw std::runtime_error(block.error().message());
      if (!block->to_cbor(cardano::core::cbor::Mode::preserve) ||
          !block->to_cbor(cardano::core::cbor::Mode::canonical)) {
        throw std::runtime_error("block encode benchmark failed");
      }
    });
    const cardano::plutus::UplcProgram program{
        cardano::plutus::UplcVersion::v1_0_0(),
        cardano::plutus::UplcTerm::apply(
            cardano::plutus::UplcTerm::apply(
                cardano::plutus::UplcTerm::builtin(cardano::plutus::Builtin::add_integer),
                cardano::plutus::UplcTerm::constant(cardano::plutus::UplcConstant::integer(
                    cardano::core::BigInteger(std::int64_t{1})))),
            cardano::plutus::UplcTerm::constant(cardano::plutus::UplcConstant::integer(
                cardano::core::BigInteger(std::int64_t{2}))))};
    const auto flat = cardano::plutus::encode_flat_program(program);
    if (!flat) throw std::runtime_error(flat.error().message());
    const auto uplc_identity = cardano::core::bytes_to_hex(cardano::crypto::blake2b256(*flat));
    const auto uplc_samples = measure(iterations, [&] {
      auto decoded = cardano::plutus::decode_flat_program(*flat);
      if (!decoded) throw std::runtime_error(decoded.error().message());
      auto evaluated = cardano::plutus::evaluate_program(*decoded, {10'000'000, 10'000'000});
      if (!evaluated) throw std::runtime_error(evaluated.error().message());
    });
#if defined(_WIN32)
    constexpr std::string_view host = "windows";
#elif defined(__APPLE__)
    constexpr std::string_view host = "macos";
#elif defined(__linux__)
    constexpr std::string_view host = "linux";
#else
    constexpr std::string_view host = "unknown";
#endif
#if defined(__aarch64__) || defined(_M_ARM64)
    constexpr std::string_view architecture = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    constexpr std::string_view architecture = "x86_64";
#else
    constexpr std::string_view architecture = "unknown";
#endif
    std::cout << "{\"architecture\":\"" << architecture << "\",\"buildType\":\""
              << CARDANO_BENCHMARK_BUILD_TYPE << "\",\"compiler\":\"" << CARDANO_BENCHMARK_COMPILER
              << "\",\"dependencyVersions\":\"vcpkg-lock\""
              << ",\"dirty\":" << CARDANO_BENCHMARK_DIRTY << ",\"gitRevision\":\""
              << CARDANO_BENCHMARK_REVISION << "\",\"host\":\"" << host
              << "\",\"schemaVersion\":1,\"standardLibrary\":\"c++23\",\"workloads\":[";
    emit("block-decode-preserve-canonical", block_identity, iterations, block_samples, true);
    emit("uplc-flat-decode-evaluate", uplc_identity, iterations, uplc_samples, false);
    std::cout << "]}\n";
    return 0;
  } catch (const std::exception& exception) {
    std::cerr << exception.what() << '\n';
    return 2;
  }
}
