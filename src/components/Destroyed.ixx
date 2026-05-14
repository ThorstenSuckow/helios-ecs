/**
* @file DestroyedComponent.ixx
 * @brief Lifecycle tag component that marks an entity as destroyed.
 */
module;

export module helios.ecs.components.Destroyed;

export namespace helios::ecs::components {

/**
 * @brief Tag component indicating that the entity entered the destroyed lifecycle state.
 *
 * Systems can use this marker to skip updates or trigger cleanup/despawn behavior.
 *
 * @tparam THandle Entity handle type associated with the owning world.
 */
template<typename THandle>
struct Destroyed{};

}