/**
 * @file Active.ixx
 * @brief Tag component indicating an active entity.
 */
module;

export module helios.ecs.common.components.Active;

export namespace helios::ecs::common::components {

    /**
     * @brief Tag component indicating that an entity is active.
     *
     * @tparam THandle The entity handle type used for domain-specific typing.
     */
    template<typename THandle>
    struct Active {

        using Handle_type = THandle;
    };

}