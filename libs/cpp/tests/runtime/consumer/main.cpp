#include <cardano/cardano.hpp>

int main() {
  cardano::enforce_linkage();
  cardano::core::enforce_linkage();
  cardano::crypto::enforce_linkage();
  cardano::chain::enforce_linkage();
  cardano::cip::enforce_linkage();
  cardano::plutus::enforce_linkage();
  return 0;
}
