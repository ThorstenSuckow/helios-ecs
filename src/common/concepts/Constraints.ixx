/**
 * @file Constraints.ixx
 * @brief Compile-time constraints for ECS component lifecycle hooks.
 */
module;

#include <concepts>

export module helios.ecs.common.concepts:Constraints;

import helios.ecs.command.commands;
import helios.ecs.component.components;

using namespace helios::ecs::commands;
using namespace helios::ecs::components;
export namespace helios::ecs::common::concepts::traits {

/**
 * @brief Trait that allows components to be trackable.
 *
 * @param T the type to check.
 *
 * @see Entity::setTrackedValue
 */
template <typename TComponent, typename TValue>
concept IsComponentDirtyTrackable = requires(TComponent& component, const TValue& value) {
    typename TComponent::Value_type;
    { component.setValue(value) } -> std::same_as<void>;
};

/**
 * @brief Type trait – `true` for `Active` specialisations.
 */
template <typename T>
struct IsActiveComponent : std::false_type {};

template <>
struct IsActiveComponent<Active> : std::true_type {};

/**
 * @brief Convenience variable template for `IsActiveComponent`.
 */
template <typename T>
inline constexpr bool IsActiveComponent_v = IsActiveComponent<std::remove_cvref_t<T>>::value;


/**
 * @brief Type trait – `true` for `AddComponentCommand<TComponent>` specialisations.
 */
template <typename T>
struct IsAddComponentCommand : std::false_type {};

template <typename THandle, typename TComponent>
struct IsAddComponentCommand<commands::AddComponentCommand<THandle, TComponent>> : std::true_type {};

/**
 * @brief Convenience variable template for `IsAddComponentCommand`.
 */
template <typename T>
inline constexpr bool IsAddComponentCommand_v = IsAddComponentCommand<std::remove_cvref_t<T>>::value;

/**
 * @brief Type trait – `true` for `RemoveComponentCommand<TComponent>` specialisations.
 */
template <typename T>
struct IsRemoveComponentCommand : std::false_type {};

template <typename THandle, typename TComponent>
struct IsRemoveComponentCommand<commands::RemoveComponentCommand<THandle, TComponent>> : std::true_type {};

/**
 * @brief Convenience variable template for `IsRemoveComponentCommand`.
 */
template <typename T>
inline constexpr bool IsRemoveComponentCommand_v = IsRemoveComponentCommand<std::remove_cvref_t<T>>::value;

} // namespace helios::ecs::common::concepts::traits