/**
 * @file IsSequentialSystemGroupContext.ixx
 * @brief Concepts and helpers for grouping and constraining ECS systems by handle type.
 */
module;

#include <concepts>

export module helios.ecs.system.concepts:IsSequentialSystemGroupContext;

export namespace helios::ecs::system::concepts {

/**
 * @brief Checks that a system exposes a `HandleType` matching the given owner handle.
 *
 * Satisfied when `TSystem::HandleType` is the same type as `TOwnerHandle`.
 * Used to enforce that all systems within a `Sequential` group operate on the same entity domain.
 *
 * @tparam TSystem     System type to check. cv-ref qualifiers are stripped before inspection.
 * @tparam TOwnerHandle Expected entity handle type.
 */
template <typename TSystem, typename TOwnerHandle>
concept SystemUsesHandle = requires { typename std::remove_cvref_t<TSystem>::HandleType; } &&
                           std::same_as<TOwnerHandle, typename std::remove_cvref_t<TSystem>::HandleType>;

/**
 * @brief Tag type representing a sequential group of systems sharing one entity handle type.
 *
 * All systems in the pack must expose a `HandleType` that matches the handle type of the
 * first system (`TFirstSystem::HandleType`). This is enforced via the `SystemUsesHandle`
 * concept. The resulting `Sequential` struct re-exports that shared handle type as `HandleType`.
 *
 * @tparam TFirstSystem Leading system; its `HandleType` defines the group's handle type.
 * @tparam TSystems     Remaining systems in the serial group, all constrained to the same handle.
 */
template <typename TFirstSystem, typename... TSystems>
    requires requires { typename std::remove_cvref_t<TFirstSystem>::HandleType; } &&
             (SystemUsesHandle<TSystems, typename std::remove_cvref_t<TFirstSystem>::HandleType> && ...)
struct Sequential {
    /** @brief Shared entity handle type of all systems in this group. */
    using HandleType = std::remove_cvref_t<TFirstSystem>::HandleType;
};

/**
 * @brief Primary template — evaluates to `std::false_type` for any non-`Sequential` type.
 *
 * @tparam T Type to inspect.
 */
template <typename... T>
struct IsSequential : std::false_type {};

/**
 * @brief Partial specialisation — evaluates to `std::true_type` for any `Sequential<...>` instance.
 *
 * @tparam TFirstSystem Leading system of the serial group.
 * @tparam TSystems     Remaining systems of the serial group.
 */
template <typename TFirstSystem, typename... TSystems>
struct IsSequential<Sequential<TFirstSystem, TSystems...>> : std::true_type {};

/**
 * @brief Concept satisfied by any `Sequential<...>` specialisation.
 *
 * @tparam T Type to check.
 */
template <typename T>
concept IsSequentialLike = IsSequential<std::remove_cvref_t<T>>::value;

}; // namespace helios::ecs::system::concepts