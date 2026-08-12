/**
 * @file ManagerRole.ixx
 * @brief Tag type for identifying Manager-role classes at compile time.
 */
module;

export module helios.ecs.manager.tags:ManagerRole;

export namespace helios::ecs::manager::tags {

    /**
     * @brief Compile-time tag identifying a class as a Manager.
     */
    struct ManagerRole{};
}