/**
 * @file Inactive.ixx
 * @brief Tag component indicating an inactive entity.
 */
module;

export module helios.ecs.common.components.Inactive;

export namespace helios::ecs::common::components {

    /**
     * @brief Tag component indicating that an entity is inactive.
     *
     * @tparam THandle The entity handle type used for domain-specific typing.
     */
    template<typename THandle>
    struct Inactive {
        using Handle_type = THandle;
    };

}