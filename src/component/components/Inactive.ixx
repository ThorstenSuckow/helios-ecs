/**
 * @file Inactive.ixx
 * @brief Tag component indicating an inactive entity.
 */
module;

export module helios.ecs.component.components:Inactive;

export namespace helios::ecs::components {

/**
 * @brief Tag component indicating that an entity is inactive.
 *
 * @tparam THandle The entity handle type used for domain-specific typing.
 */
template <typename THandle>
struct Inactive {
    using HandleType = THandle;
};

} // namespace helios::ecs::components