/**
 * @file EcsDataContainer.ixx
 * @brief EcsDataContainer for storing arbitrary data in a TypeMap.
 */
module;

export module helios.ecs.common.container:EcsDataContainer;

import helios.core.common.container;

namespace helios::ecs::common::container::__detail {
    struct EcsDataContainerTag{};
}

export namespace helios::ecs::common::container {
    using EcsDataContainer = core::common::container::TypeMap<helios::ecs::common::container::__detail::EcsDataContainerTag>;
}