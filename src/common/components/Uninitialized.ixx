/**
 * @file Uninitialized.ixx
 * @brief Lifecycle tag component that marks entities not initialized yet.
 */
module;

export module helios.ecs.common.components.Uninitialized;

export namespace helios::ecs::common::components {

    /**
     * @brief Tag component indicating that entity initialization is still pending.
     *
     * @tparam THandle Entity handle type associated with the owning world.
     */
    template<typename THandle>
    struct Uninitialized{};

}