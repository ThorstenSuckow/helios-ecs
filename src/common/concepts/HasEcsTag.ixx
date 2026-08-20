/**
 * @file HasTag.ixx
 * @brief Concept for detecting a compile-time engine role tag on a type.
 */
module;

#include <concepts>

export module helios.ecs.common.concepts:HasEcsTag;

export namespace helios::ecs::common::concepts {

    /**
     * @brief Detects whether T declares a nested `EcsRoleTag` alias equal to Tag.
     *
     * @tparam T The type to inspect.
     * @tparam Tag The expected tag type (e.g. ManagerRole, SystemRole).
     */
    template<class T, class Tag>
    concept HasEcsTag = std::same_as<typename T::EcsRoleTag, Tag>;

}