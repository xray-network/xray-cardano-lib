#include <cardano/cardano.hpp>

int main() {
  const auto encoded = cardano::core::encode_bech32("addr", {});
  return encoded ? 0 : 1;
}
