/**
 * @file DestroyedComponent.ixx
 * @brief Lifecycle tag component that marks an entity as destroyed.
 */
module;

export module helios.ecs.component.components:Destroyed;

export namespace helios::ecs::components {

/**
 * @brief Tag component indicating that the entity entered the destroyed lifecycle state.
 *
 * Systems can use this marker to skip updates or trigger cleanup/despawn behavior.
 */
struct Destroyed {
};

} // namespace helios::ecs::components
