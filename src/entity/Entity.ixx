/**
 * @file Entity.ixx
 * @brief High-level facade for entity manipulation in the ECS.
 */
module;

#include <cassert>
#include <type_traits>
#include <utility>
#include <mach/thread_info.h>

export module helios.ecs.entity.Entity;

import helios.ecs.component;
import helios.ecs.command.commands;
import helios.ecs.common.concepts;
import helios.ecs.common.types;

using namespace helios::ecs::common::types;
using namespace helios::ecs::common::concepts::traits;
using namespace helios::ecs::components;
export namespace helios::ecs::entity {

/**
 * @brief Lightweight facade for entity component manipulation.
 */
template <typename TEntityManager>
class Entity {

    // used to access const EntityManager spezialisation in resetTo
    template <typename>
    friend class Entity;

public:
    using HandleType = std::remove_const_t<TEntityManager>::HandleType;

private:
    /**
     * @brief The underlying entity identifier.
     */
    HandleType entityHandle_{0, 0};

    /**
     * @brief Non-owning pointer to the EntityManager.
     */
    TEntityManager* entityManager_;

    using ComponentTypeId = ComponentTypeId<HandleType>;

public:

    using ActiveComponent_type = Active;

    using InactiveComponent_type = Inactive;

    /**
     * @brief Constructs a Entity wrapper.
     *
     * @param entityHandle The entity handle to wrap.
     * @param entityManager Pointer to the EntityManager. Must not be null.
     */
    explicit Entity(
        const HandleType entityHandle, TEntityManager* entityManager

    ) noexcept
        : entityHandle_(entityHandle), entityManager_(entityManager) {
        assert(entityManager_ != nullptr && "EntityManager must not be null.");
    };

    /**
     * @brief Returns a generator over all component type IDs attached to this entity.
     *
     * @return Generator yielding ComponentTypeId for each attached component.
     */
    template <typename TFunc>
    void forEachComponentTypeId(TFunc&& func) const {
        entityManager_->forEachComponentTypeId(entityHandle_, std::forward<TFunc>(func));
    }

    /**
     * @brief Reset this entity to the component set of the specified entity
     *
     * @param sourceEntity The entity to use the component set from.
     *
     * @return True if the reset was successful, false otherwise.
     */
    bool resetTo(const Entity<const TEntityManager>& sourceEntity) {
        return entityManager_->resetTo(entityHandle_, *sourceEntity.entityManager_, sourceEntity.handle());
    }

    /**
     * @brief Returns the underlying entity handle.
     *
     * @return The EntityHandle for this Entity.
     */
    [[nodiscard]] HandleType handle() noexcept {
        return entityHandle_;
    }

    /**
     * @brief Returns the underlying entity handle (const).
     *
     * @return The EntityHandle for this Entity.
     */
    [[nodiscard]] HandleType handle() const noexcept {
        return entityHandle_;
    }

    /**
     * @brief Constructs and attaches a component to this entity.
     *
     * @tparam TComponent The component type to add.
     * @tparam Args Constructor argument types.
     *
     * @param args Arguments forwarded to the component constructor.
     *
     * @return Reference to the newly created component.
     */
    template <typename TComponent, typename... Args>
    TComponent& add(Args&&... args) {

        auto typeId = ComponentTypeId::template id<TComponent>();

        auto* cmp = entityManager_->template ensureAndEmplace<TComponent>(entityHandle_, std::forward<Args>(args)...);

        assert(cmp != nullptr && "Component emplace failed.");
        return *cmp;
    }

    /**
     * @brief Returns existing component or creates a new one.
     *
     * @tparam TComponent The component type.
     * @tparam Args Constructor argument types.
     *
     * @param args Arguments forwarded to the constructor if creating.
     *
     * @return Reference to the existing or newly created component.
     */
    template <typename TComponent, typename... Args>
    TComponent& getOrAdd(Args&&... args) {
        if (entityManager_->template has<TComponent>(entityHandle_)) {
            return *entityManager_->template get<TComponent>(entityHandle_);
        }
        return add<TComponent>(std::forward<Args>(args)...);
    }



    template <typename TComponent, typename TValue>
    void setTrackedValue(TComponent* component, const TValue& value) {
        assert(component != nullptr && "Unexpected nullptr for component.");
        component->setValue(value);
        auto* sp = entityManager_->template sparseSet<TComponent>();
        sp->markDirty(entityHandle_.entityId());
    }

    /**
     * @brief Removes a component from this entity.
     *
     * @tparam TComponent The component type to remove.
     *
     * @return True if the component was removed, false if not present.
     */
    template <typename TComponent>
    bool remove() {
        return entityManager_->template remove<TComponent>(entityHandle_);
    }

    /**
     * @brief Returns raw pointer to component by type ID.
     *
     * @param typeId The component type identifier.
     *
     * @return Raw void pointer to the component, or nullptr if not found.
     */
    void* raw(const ComponentTypeId typeId) {
        return entityManager_->raw(entityHandle_, typeId);
    }


    template <typename TComponent>
    TComponent* get() {
        return entityManager_->template get<TComponent>(entityHandle_);
    }

    template <typename TComponent>
    const TComponent* get() const {
        return entityManager_->template get<TComponent>(entityHandle_);
    }

    /**
     * @brief Checks if this entity has a specific component type.
     *
     * @tparam TComponent The component type to check.
     *
     * @return True if the component is attached, false otherwise.
     */
    template <typename TComponent>
    [[nodiscard]] bool has() const noexcept {
        return entityManager_->template has<TComponent>(entityHandle_);
    }

    /**
     * @brief Checks if this entity has a component by type ID.
     *
     * @param typeId The component type identifier.
     *
     * @return True if the component is attached, false otherwise.
     */
    bool has(ComponentTypeId typeId) const noexcept {
        return entityManager_->has(entityHandle_, typeId);
    }

    /**
     * @brief Sets the activation state of this Entity.
     *
     * @param active True to activate, false to deactivate.
     *
     * @see HierarchyComponent_type
     * @see HierarchyPropagationSystem
     */
    void setActive(const bool active) {
        const bool isActive = entityManager_->template has<ActiveComponent_type>(entityHandle_);
        const bool isInActive = !isActive;

        if (!isActive && active) {
            remove<InactiveComponent_type>();
            add<ActiveComponent_type>();
        }

        if (!isInActive && !active) {
            remove<ActiveComponent_type>();
            add<InactiveComponent_type>();
        }
    }

    /**
     * @brief Returns whether this Entity is active.
     *
     * @return True if the entity has the Active tag component.
     */
    [[nodiscard]] bool isActive() const {
        return entityManager_->template has<ActiveComponent_type>(entityHandle_);
    }
};

} // namespace helios::ecs
