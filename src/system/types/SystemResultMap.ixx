/**
 * @file SystemResultMap.ixx
 * @brief TypeMap for storing FrameResults.
 */
module;

export module helios.ecs.system.types:SystemResultMap;

import helios.core.container;

export namespace helios::ecs::system::types {

    struct SystemResultTag{};
    using SystemResultMap = core::container::TypeMap<SystemResultTag>;
}