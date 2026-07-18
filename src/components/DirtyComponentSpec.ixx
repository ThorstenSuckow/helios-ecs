/**
 * @file DirtyComponentSpec.ixx
 * @brief Component template for dirty component specifications.
 */
module;

export module helios.ecs.components.DirtyComponentSpec;

export namespace helios::ecs::components {

    /**
     * @brief Template for dirty component specifications.
     *
     * @tparam TComponent The component type that is marked as dirty.
     */
    template<typename TComponent>
    struct DirtyComponentSpec {

        using Component_type = TComponent;
    };


}