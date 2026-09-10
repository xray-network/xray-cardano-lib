# XRAY Cardano Lib for C++

Status: Concept — not maintained or supported.

This directory is retained as a C++23 concept for Cardano wire formats, cryptography, ledger
models, CIPs, and Plutus evaluation. It is available for experimentation and design exploration,
but it is not kept in feature parity with the maintained TypeScript implementation. It does not
build, load, or depend on the TypeScript workspace.

The public component targets are `cardano::core`, `cardano::crypto`, `cardano::chain`,
`cardano::cip`, `cardano::plutus`, and aggregate `cardano::lib`. Dependencies are resolved from the
exact vcpkg baseline recorded in `vcpkg-configuration.json`; test-only dependencies are enabled
by the presets.

Ledger-era models are organized below `include/cardano/chain/era/` and
`src/chain/era/`. Existing consumers can keep including
`cardano/chain/era_models.hpp`; it is the compatibility umbrella. Focused public entry points are
available as `shared.hpp`, `byron.hpp`, `shelley.hpp`, `allegra.hpp`, `mary.hpp`, `alonzo.hpp`,
`babbage.hpp`, and `conway.hpp` below `cardano/chain/era/`.

## Host prerequisites

Install the following tools on the host:

- A supported matrix host: macOS 26/ARM64 with Apple Clang 21, Ubuntu 24.04/x86-64 with
  GCC 14 or Clang 18, or Windows Server 2025/x86-64 with MSVC 19.4x.
- CMake 3.28 or newer.
- Ninja 1.11 or newer.
- Git.
- pkg-config, Autoconf, Automake, GNU libtool, and GNU M4 for dependency ports that use
  Autotools.

Configuration probes the C++23 library facilities actually used instead of accepting a host from
an OS or compiler version string. Other hosts may be buildable, but are not in the supported
validation matrix. Static archives and C++ ABI are not portable between toolchains.

Install the Xcode command-line tools:

```sh
xcode-select --install
```

Then install the remaining build tools with Homebrew:

```sh
brew install cmake ninja git pkgconf autoconf automake libtool m4
```

Homebrew installs GNU libtool commands with a `g` prefix on macOS. Keep the Homebrew binary
directory on `PATH` so vcpkg can find `glibtool` and `glibtoolize`.

## Bootstrap vcpkg

Do not install Boost.Multiprecision, Botan, libsodium, secp256k1, nlohmann/json, BLST, or Catch2
from the host package manager. The manifest resolves their exact versions from the baseline in
[`vcpkg.json`](vcpkg.json); Catch2 is enabled only for test presets.

```sh
git clone https://github.com/microsoft/vcpkg.git /path/to/vcpkg
git -C /path/to/vcpkg checkout cd61e1e26a038e82d6550a3ebbe0fbbfe7da78e3
/path/to/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=/path/to/vcpkg
```

`VCPKG_ROOT` must remain set while configuring, building, installing, or testing this workspace.
The first configure downloads and builds the pinned dependencies and can take several minutes.

## Build the library

From `libs/cpp/`, configure and build a release without the test-only dependency:

```sh
cmake -S . -B build/release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=cmake/cardano-vcpkg-toolchain.cmake
cmake --build build/release
```

Optionally install the package to a chosen prefix:

```sh
cmake --install build/release --prefix /path/to/install
```

Consumers can then use `find_package(XRAYCardanoLib CONFIG REQUIRED)` with that prefix on
`CMAKE_PREFIX_PATH`. Components can be requested independently; `core` has no external crypto
discovery, while every other component loads its exact transitive static-link dependency closure.

## Benchmarks

Benchmarks are opt-in and never affect semantic test results:

```sh
cmake --preset benchmarks
cmake --build --preset benchmarks
```

Benchmark inputs must be explicit captured-snapshot paths in the build tree. Reports are
schema-versioned JSON, remain untracked, and are not installed. Timing is descriptive only; it is
never a correctness or release threshold.

## Build and run tests

The CI workflow configures the pinned test feature, treats owned-source warnings as errors, builds
the library, runs all registered tests, validates captured-provider integrity, and verifies a
clean installed-package consumer. From the repository root, enter the C++ workspace and ensure
the pinned vcpkg checkout is selected:

```sh
cd libs/cpp
export VCPKG_ROOT=/path/to/vcpkg
```

Then run the complete workflow:

```sh
cmake --workflow --preset ci
```

Individual stages can be rerun with:

```sh
cmake --preset ci
cmake --build --preset ci
ctest --preset ci
```

If `VCPKG_ROOT`, the compiler, or the target architecture changes, start a fresh configuration:

```sh
cmake --preset ci --fresh
```

See [`tests/README.md`](tests/README.md) for test discovery, label and name filtering, quick
reruns, and the sanitizer and hardening workflows.

The library provides source and wire compatibility only; no stable binary ABI is promised.

Public byte-returning APIs own their storage, byte inputs use immutable spans where appropriate,
and fallible untrusted-input APIs use `cardano::core::Result`.

## Tests

Tests are independent semantic equivalents authored from the frozen implementation contract and
captured evidence; TypeScript source and tests are not implementation inputs. See
[`tests/README.md`](tests/README.md) for the mirrored domain layout, local CMake ownership, and
current coverage.
