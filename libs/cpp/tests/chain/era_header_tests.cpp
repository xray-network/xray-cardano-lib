#include <cardano/chain/era/allegra.hpp>
#include <cardano/chain/era/alonzo.hpp>
#include <cardano/chain/era/babbage.hpp>
#include <cardano/chain/era/byron.hpp>
#include <cardano/chain/era/conway.hpp>
#include <cardano/chain/era/mary.hpp>
#include <cardano/chain/era/shared.hpp>
#include <cardano/chain/era/shelley.hpp>
#include <cardano/chain/era_models.hpp>
#include <concepts>

namespace {

using namespace cardano;

static_assert(std::derived_from<chain::ByronTx,
                                chain::EraCborModel<chain::ByronTx, chain::EraWireShape::array>>);
static_assert(
    std::derived_from<chain::ShelleyBlock,
                      chain::EraCborModel<chain::ShelleyBlock, chain::EraWireShape::array>>);
static_assert(
    std::derived_from<chain::AllegraBlock,
                      chain::EraCborModel<chain::AllegraBlock, chain::EraWireShape::array>>);
static_assert(std::derived_from<chain::MaryBlock,
                                chain::EraCborModel<chain::MaryBlock, chain::EraWireShape::array>>);
static_assert(
    std::derived_from<chain::AlonzoBlock,
                      chain::EraCborModel<chain::AlonzoBlock, chain::EraWireShape::array>>);
static_assert(
    std::derived_from<chain::BabbageBlock,
                      chain::EraCborModel<chain::BabbageBlock, chain::EraWireShape::array>>);
static_assert(
    std::derived_from<chain::Block, chain::EraCborModel<chain::Block, chain::EraWireShape::array>>);

}  // namespace
