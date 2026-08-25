/**
 * @file Entity.ixx
 * @brief High-level facade for entity manipulation in the ECS.
 */
module;

#include <cassert>
#include <utility>
#include <type_traits>


export module helios.ecs.Entity;

import helios.ecs.EntityManager;
import helios.ecs.component;
import helios.ecs.command;
import helios.ecs.common.concepts;
import helios.ecs.common.types;

using namespace helios::ecs::common::types;
using namespace helios::ecs::command;
using namespace helios::ecs::common::concepts::traits;
using namespace helios::ecs::components;
export namespace helios::ecs {

    /**
     * @brief Lightweight facade for entity component manipulation.
     */
    template<typename TEntityManager>
    class Entity {

        // used to access const EntityManager spezialisation in resetTo
        template<typename>
        friend class Entity;

        using HandleType = typename std::remove_const_t<TEntityManager>::HandleType;


        /**
         * @brief The underlying entity identifier.
         */
        HandleType entityHandle_{0,0};

        /**
         * @brief Non-owning pointer to the EntityManager.
         */
        TEntityManager* entityManager_;

        /**
         * @brief Marks the specified component as dirty and registers the dirty set with the EntityManager.
         *
         * @tparam T The component type to mark as dirty.
         */
        template<typename T>
        void markDirty() {
            if (!entityManager_->template  managesDirty<T>()) {
                bool mg = entityManager_->template  managesDirty<T>();
                assert(mg && "Cannot mark component as dirty, not tracked by EntityManager." );

            }
            getOrAdd<DirtyComponentSpec<T>>();
        }

    public:

        using Handle_type = HandleType;

        using ComponentTypeId_type = ComponentTypeId<Handle_type>;

        using ActiveComponent_type = Active<Handle_type>;

        using InactiveComponent_type = Inactive<Handle_type>;
        
        /**
         * @brief Constructs a Entity wrapper.
         *
         * @param entityHandle The entity handle to wrap.
         * @param entityManager Pointer to the EntityManager. Must not be null.
         */
        explicit Entity(
            const Handle_type entityHandle,
            TEntityManager* entityManager

        ) noexcept : entityHandle_(entityHandle), entityManager_(entityManager) {
            assert(entityManager_ != nullptr && "EntityManager must not be null.");
        };

        /**
         * @brief Returns a generator over all component type IDs attached to this entity.
         *
         * @return Generator yielding ComponentTypeId for each attached component.
         */
        template<typename TFunc>
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
        [[nodiscard]] Handle_type handle() noexcept {
            return entityHandle_;
        }

        /**
         * @brief Returns the underlying entity handle (const).
         *
         * @return The EntityHandle for this Entity.
         */
        [[nodiscard]] Handle_type handle() const noexcept {
            return entityHandle_;
        }

        /**
         * @brief Constructs and attaches a component to this entity.
         *
         * @tparam T The component type to add.
         * @tparam Args Constructor argument types.
         *
         * @param args Arguments forwarded to the component constructor.
         *
         * @return Reference to the newly created component.
         */
        template<typename T, typename ...Args>
        T& add(Args&& ...args) {

            auto typeId = ComponentTypeId_type::template id<T>();

            auto* cmp = entityManager_->template emplace<T>(entityHandle_, std::forward<Args>(args)...);

            return *cmp;
        }

        /**
         * @brief Enqueues a deferred add-component command into `buffer`.
         *
         * The component is not attached immediately; the command is applied
         * when the buffer is flushed by the `EntityMutationManager`.
         *
         * @tparam TComponent Component type to add.
         * @tparam TBuffer    Command buffer type. Its `Handle_type` must match this entity's.
         * @tparam Args       Constructor argument types for `TComponent`.
         * @param  buffer     Target command buffer to enqueue into.
         * @param  args       Arguments forwarded to the `TComponent` constructor.
         */
        template<typename TComponent, typename TBuffer, typename ...Args>
        requires std::is_same_v<typename TEntityManager::Handle_type, typename TBuffer::Handle_type>
        void deferAdd(TBuffer& buffer, Args&& ...args) {
            buffer.template add<commands::AddComponentCommand<TComponent>>(entityHandle_, std::forward<Args>(args)...);
        }

        /**
         * @brief Enqueues a deferred remove-component command into `buffer`.
         *
         * The component is not detached immediately; the command is applied
         * when the buffer is flushed by the `EntityMutationManager`.
         *
         * @tparam TComponent Component type to remove.
         * @tparam TBuffer    Command buffer type. Its `Handle_type` must match this entity's.
         * @param  buffer     Target command buffer to enqueue into.
         */
        template<typename TComponent, typename TBuffer>
        requires std::is_same_v<typename TEntityManager::Handle_type, typename TBuffer::Handle_type>
        void deferRemove(TBuffer& buffer) {
            buffer.template add<commands::RemoveComponentCommand<TComponent>>(entityHandle_);
        }

        /**
         * @brief Enqueues the commands required to activate this entity into `buffer`.
         *
         * Adds `Active`, removes `Inactive`, and adds `DirtyComponentSpec<Active>`
         * via deferred commands. Also registers `Active` for dirty tracking immediately.
         * Commands are applied when the buffer is flushed by the `EntityMutationManager`.
         *
         * @tparam TBuffer Command buffer type. Its `Handle_type` must match this entity's.
         * @param  buffer  Target command buffer to enqueue into.
         */
        template<typename TBuffer>
        requires std::is_same_v<typename TEntityManager::Handle_type, typename TBuffer::Handle_type>
        void deferSetActive(TBuffer& buffer) {
            buffer.template add<commands::AddComponentCommand<Active<typename TEntityManager::Handle_type>>>(entityHandle_);
            buffer.template add<commands::RemoveComponentCommand<Active<typename TEntityManager::Handle_type>>>(entityHandle_);
            buffer.template add<commands::AddComponentCommand<DirtyComponentSpec<Active<typename TEntityManager::Handle_type>>>>(entityHandle_);

            trackDirty<Active<typename TEntityManager::Handle_type>>();
        }

        /**
         * @brief Enqueues the commands required to deactivate this entity into `buffer`.
         *
         * Adds `Inactive`, removes `Active`, and adds `DirtyComponentSpec<Inactive>`
         * via deferred commands. Also registers `Inactive` for dirty tracking immediately.
         * Commands are applied when the buffer is flushed by the `EntityMutationManager`.
         *
         * @tparam TBuffer Command buffer type. Its `Handle_type` must match this entity's.
         * @param  buffer  Target command buffer to enqueue into.
         */
        template<typename TBuffer>
        requires std::is_same_v<typename TEntityManager::Handle_type, typename TBuffer::Handle_type>
        void deferSetInactive(TBuffer& buffer) {
            buffer.template add<commands::AddComponentCommand<Inactive<typename TEntityManager::Handle_type>>>(entityHandle_);
            buffer.template add<commands::RemoveComponentCommand<Active<typename TEntityManager::Handle_type>>>(entityHandle_);
            buffer.template add<commands::AddComponentCommand<DirtyComponentSpec<Inactive<typename TEntityManager::Handle_type>>>>(entityHandle_);

            trackDirty<Inactive<typename TEntityManager::Handle_type>>();
        }

        /**
         * @brief Returns existing component or creates a new one.
         *
         * @tparam T The component type.
         * @tparam Args Constructor argument types.
         *
         * @param args Arguments forwarded to the constructor if creating.
         *
         * @return Reference to the existing or newly created component.
         */
        template<typename T, typename ...Args>
        T& getOrAdd(Args&& ...args) {
            if (entityManager_->template  has<T>(entityHandle_)) {
                return *entityManager_->template  get<T>(entityHandle_);
            }
            return add<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Tracks the specified component which will be added if not already existing.
         *
         * @tparam T The component type to track.
         * @tparam Args Constructor argument types.
         * @param args Arguments forwarded to the component constructor.
         * @return Reference to the tracked component.
         *
         * @see markDirty
         */
        template<typename T, typename ...Args>
        T& trackDirty(Args&& ...args) {
            entityManager_->template trackDirty<T>();
            markDirty<T>();
            return getOrAdd<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Updates the component's value and marks the component type dirty.
         *
         * @tparam TComponent The component type to update. Must be existing for this entity.
         * @tparam TValue The value type to set. Must be compatible with TComponent::Value_type.
         * @param component Pointer to the TComponent
         * @param value The value to update the component with.
         */
        template<typename TComponent, typename TValue>
        requires IsComponentDirtyTrackable<TComponent, TValue>
            && std::is_trivially_copyable_v<TValue>
            && (sizeof(TValue) <= 2* sizeof(void*))
        void setTrackedValue(TComponent* component, TValue value) {
            assert(component != nullptr && "Unexpected nullptr for component.");
            component->setValue(value);
            markDirty<TComponent>();
        }

        /**
         * @brief Updates the component's value and marks the component type dirty.
         *
         * @tparam TComponent The component type to update. Must be existing for this entity.
         * @tparam TValue The value type to set. Must be compatible with TComponent::Value_type.
         * @param component Pointer to the TComponent
         * @param value The value to update the component with.
         */
        template<typename TComponent, typename TValue>
        requires IsComponentDirtyTrackable<TComponent, TValue>
            && (!(std::is_trivially_copyable_v<TValue>
            && (sizeof(TValue) <= 2* sizeof(void*))))
        void setTrackedValue(TComponent* component, const TValue& value) {
            assert(component != nullptr && "Unexpected nullptr for component.");
            component->setValue(value);
            markDirty<TComponent>();
        }


        /**
         * @brief Removes a component from this entity.
         *
         * @tparam T The component type to remove.
         *
         * @return True if the component was removed, false if not present.
         */
        template<typename T>
        bool remove() {
            return entityManager_->template  remove<T>(entityHandle_);
        }

        /**
         * @brief Returns raw pointer to component by type ID.
         *
         * @param typeId The component type identifier.
         *
         * @return Raw void pointer to the component, or nullptr if not found.
         */
        void* raw(const ComponentTypeId_type typeId) {
            return entityManager_->raw(entityHandle_, typeId);
        }


        template<
            template<typename> typename TComponent
        >
        TComponent<Handle_type>* get() {
            return entityManager_->template  get<TComponent<Handle_type>>(entityHandle_);
        }

        template<
            template<typename> typename TComponent
        >
        const TComponent<Handle_type>* get() const {
            return entityManager_->template  get<TComponent<Handle_type>>(entityHandle_);
        }


        template<typename T>
        T* get() {
            return entityManager_->template  get<T>(entityHandle_);
        }

        template<typename T>
        const T* get() const {
            return entityManager_->template  get<T>(entityHandle_);
        }

        /**
         * @brief Checks if this entity has a specific component type.
         *
         * @tparam T The component type to check.
         *
         * @return True if the component is attached, false otherwise.
         */
        template<typename T>
        bool has() const noexcept{
            return entityManager_->template  has<T>(entityHandle_);
        }

        /**
         * @brief Checks if this entity has a component by type ID.
         *
         * @param typeId The component type identifier.
         *
         * @return True if the component is attached, false otherwise.
         */
        bool has(ComponentTypeId_type typeId) const noexcept {
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
                trackDirty<ActiveComponent_type>();
            }

            if (!isInActive && !active) {
                remove<ActiveComponent_type>();
                trackDirty<InactiveComponent_type>();
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



} // namespace helios
