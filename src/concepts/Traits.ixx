/**
 * @file Traits.ixx
 * @brief Compile-time traits for ECS component lifecycle hooks.
 */
module;

#include <concepts>
#include <utility>

export module helios.ecs.concepts:Traits;

import helios.ecs.common.commands;
import helios.ecs.common.components;

using namespace helios::ecs::common::commands;
using namespace helios::ecs::common::components;
export namespace helios::ecs::concepts::traits {

    /**
     * @brief Trait that allows components to be trackable.
     *
     * @param T the type to check.
     *
     * @see Entity::setTrackedValue
     */
    template<typename TComponent, typename TValue>
    concept IsComponentDirtyTrackable = requires(TComponent& component, const TValue& value) {
        typename TComponent::Value_type;
        {component.setValue(value)} -> std::same_as<void>;
    };


    /**
     * @brief Type trait – `true` for `Active<Thandle>` specialisations.
     */
    template<typename T>
    struct IsActiveComponent : std::false_type {};

    template<typename THandle>
    struct IsActiveComponent<Active<THandle>> : std::true_type {};

    /**
     * @brief Convenience variable template for `IsActiveComponent`.
     */
    template<typename T>
    inline constexpr bool IsActiveComponent_v = IsActiveComponent<std::remove_cvref_t<T>>::value;

    /**
     * @brief Type trait – `true` for `DirtyComponentSpec<TComponent>` specialisations.
     */
    template<typename T>
    struct IsDirtyComponentSpec : std::false_type {};

    template<typename TComponent>
    struct IsDirtyComponentSpec<DirtyComponentSpec<TComponent>> : std::true_type {};

    /**
     * @brief Convenience variable template for `IsDirtyComponentSpec`.
     */
    template<typename T>
    inline constexpr bool IsDirtyComponentSpec_v = IsDirtyComponentSpec<std::remove_cvref_t<T>>::value;

    /**
     * @brief Type trait – `true` for `AddComponentCommand<TComponent>` specialisations.
     */
    template<typename T>
    struct IsAddComponentCommand : std::false_type {};

    template<typename TComponent>
    struct IsAddComponentCommand<AddComponentCommand<TComponent>> : std::true_type {};

    /**
     * @brief Convenience variable template for `IsAddComponentCommand`.
     */
    template<typename T>
    inline constexpr bool IsAddComponentCommand_v = IsAddComponentCommand<std::remove_cvref_t<T>>::value;

    /**
     * @brief Type trait – `true` for `RemoveComponentCommand<TComponent>` specialisations.
     */
    template<typename T>
    struct IsRemoveComponentCommand : std::false_type {};

    template<typename TComponent>
    struct IsRemoveComponentCommand<RemoveComponentCommand<TComponent>> : std::true_type {};

    /**
     * @brief Convenience variable template for `IsRemoveComponentCommand`.
     */
    template<typename T>
    inline constexpr bool IsRemoveComponentCommand_v = IsRemoveComponentCommand<std::remove_cvref_t<T>>::value;

}