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
     * @see IsRuntimeSystemLike
     * @see HasTag
     */
    struct TypedSystemRole{};

    /**
     * @brief Compile-time tag identifying a class as a CallableSystem.
     *
     * @see IsRuntimeSystemLike
     * @see HasTag
     */
    struct CallableSystemRole{};
}