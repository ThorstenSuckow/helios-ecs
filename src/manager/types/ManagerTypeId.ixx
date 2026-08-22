/**
 * @file ManagerTypeId.ixx
 * @brief Unique type identifier for managers.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.manager.types:ManagerTypeId;

import helios.core.common;

export namespace helios::ecs::manager::types {

    struct helios_ecs_tag_ManagerTypes{};
    using ManagerTypeId = helios::core::common::types::TypeId<helios_ecs_tag_ManagerTypes>;


};