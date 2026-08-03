#if CARDANO_COMPONENT_CORE
#include <cardano/core/core.hpp>
#elif CARDANO_COMPONENT_CRYPTO
#include <cardano/crypto/crypto.hpp>
#elif CARDANO_COMPONENT_CHAIN
#include <cardano/chain/chain.hpp>
#elif CARDANO_COMPONENT_CIP
#include <cardano/cip/cip.hpp>
#elif CARDANO_COMPONENT_PLUTUS
#include <cardano/plutus/plutus.hpp>
#else
#include <cardano/cardano.hpp>
#endif

int main() {
#if CARDANO_COMPONENT_CORE
  cardano::core::enforce_linkage();
#elif CARDANO_COMPONENT_CRYPTO
  cardano::crypto::enforce_linkage();
#elif CARDANO_COMPONENT_CHAIN
  cardano::chain::enforce_linkage();
#elif CARDANO_COMPONENT_CIP
  cardano::cip::enforce_linkage();
#elif CARDANO_COMPONENT_PLUTUS
  cardano::plutus::enforce_linkage();
#else
  cardano::enforce_linkage();
#endif
  return 0;
}
