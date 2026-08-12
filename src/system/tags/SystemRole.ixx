/**
 * @file SystemRole.ixx
 * @brief Tag type for identifying System-role classes at compile time.
 */
module;

export module helios.ecs.system.tags:SystemRole;

export namespace helios::ecs::system::tags {

    /**
     * @brief Compile-time tag identifying a class as a System.
     *
     * @details Declare `using EcsRoleTag = TypedSystemRole;` inside a class
     * to make it satisfy the `IsSystemLike` concept. This enables registration
     * via `SystemRegistry::add<T>()`.
     *
     * @see IsRuntimeSystemLike
     * @see HasTag
     */
    struct TypedSystemRole{};

    /**
     * @brief Compile-time tag identifying a class as a CallableSystem.
     *
     * @details Declare `using EcsRoleTag = CallableSystemRole;` inside a class
     * to make it satisfy the `IsCallableSystemLike` concept. This enables registration
     * via `SystemRegistry::add<T>()`.
     *
     * @see IsRuntimeSystemLike
     * @see HasTag
     */
    struct CallableSystemRole{};
}