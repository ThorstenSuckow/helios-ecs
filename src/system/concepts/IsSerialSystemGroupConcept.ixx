/**
 * @file IsSerialSystemGroupContext.ixx
 * @brief Concepts and helpers for grouping and constraining ECS systems by handle type.
 */
module;

#include <concepts>

export module helios.ecs.system.concepts:IsSerialSystemGroupContext;

export namespace helios::ecs::system::concepts {

/**
 * @brief Checks that a system exposes a `Handle_type` matching the given owner handle.
 *
 * Satisfied when `TSystem::Handle_type` is the same type as `TOwnerHandle`.
 * Used to enforce that all systems within a `Serial` group operate on the same entity domain.
 *
 * @tparam TSystem     System type to check. cv-ref qualifiers are stripped before inspection.
 * @tparam TOwnerHandle Expected entity handle type.
 */
template <typename TSystem, typename TOwnerHandle>
concept SystemUsesHandle = requires { typename std::remove_cvref_t<TSystem>::Handle_type; } &&
                           std::same_as<TOwnerHandle, typename std::remove_cvref_t<TSystem>::Handle_type>;

/**
 * @brief Tag type representing a sequential group of systems sharing one entity handle type.
 *
 * All systems in the pack must expose a `Handle_type` that matches the handle type of the
 * first system (`TFirstSystem::Handle_type`). This is enforced via the `SystemUsesHandle`
 * concept. The resulting `Serial` struct re-exports that shared handle type as `Handle_type`.
 *
 * @tparam TFirstSystem Leading system; its `Handle_type` defines the group's handle type.
 * @tparam TSystems     Remaining systems in the serial group, all constrained to the same handle.
 */
template <typename TFirstSystem, typename... TSystems>
    requires requires { typename std::remove_cvref_t<TFirstSystem>::Handle_type; } &&
             (SystemUsesHandle<TSystems, typename std::remove_cvref_t<TFirstSystem>::Handle_type> && ...)
struct Serial {
    /** @brief Shared entity handle type of all systems in this group. */
    using Handle_type = std::remove_cvref_t<TFirstSystem>::Handle_type;
};

/**
 * @brief Primary template — evaluates to `std::false_type` for any non-`Serial` type.
 *
 * @tparam T Type to inspect.
 */
template <typename... T>
struct IsSerial : std::false_type {};

/**
 * @brief Partial specialisation — evaluates to `std::true_type` for any `Serial<...>` instance.
 *
 * @tparam TFirstSystem Leading system of the serial group.
 * @tparam TSystems     Remaining systems of the serial group.
 */
template <typename TFirstSystem, typename... TSystems>
struct IsSerial<Serial<TFirstSystem, TSystems...>> : std::true_type {};

/**
 * @brief Concept satisfied by any `Serial<...>` specialisation.
 *
 * @tparam T Type to check.
 */
template <typename T>
concept IsSerialLike = IsSerial<std::remove_cvref_t<T>>::value;

}; // namespace helios::ecs::system::concepts