/**
 * @file CommandTypeId.ixx
 * @brief Unique type identifier for command types.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.command.types:CommandTypeId;

import helios.core.common;

export namespace helios::ecs::command::types {

    struct helios_ecs_tag_CommandTypes{};
    using CommandTypeId = helios::core::common::types::TypeId<helios_ecs_tag_CommandTypes>;

};