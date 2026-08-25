/**
 * @file EntityManager.ixx
 * @brief Central manager for entity lifecycle and component storage.
 */
module;

#include <memory>
#include <vector>
#include <utility>
#include <cassert>
#include <algorithm>
#include "helios-ecs-config.h"

export module helios.ecs.EntityManager;

import helios.ecs.storage.SparseSet;
import helios.ecs.EntityRegistry;

import helios.ecs.common.types;
import helios.ecs.component;

import helios.ecs.common.concepts;


using namespace helios::ecs::common::types;
using namespace helios::ecs::components;
using namespace helios::ecs::storage;
using namespace helios::ecs::common::concepts;
export namespace helios::ecs {

    /**
     * @brief Sorting criteria for `SparseSetBase*` lists.
     */
    enum class SortCriteria {
        MaxEntityId,
        ComponentCount
    };

    /**
     * @brief Stores entities and their component sparse sets for one handle domain.
     *
     * @tparam THandle Entity handle type managed by this manager.
     */
    template<typename THandle>
    class EntityManager {


    public:

        /**
         * @brief Registry type used by this manager.
         */
        using EntityRegistry_type = EntityRegistry<THandle>;

        /**
         * @brief Entity handle type of this manager.
         */
        using Handle_type = THandle;

        using HandleType = THandle;

        /**
         * @brief Component type-id provider bound to `HandleType`.
         */
        using ComponentTypeId_type = ComponentTypeId<HandleType>;

        /**
         * @brief Non-copyable: copying an EntityManager is explicitly disabled.
         */
        EntityManager(const EntityManager&) = delete;
        EntityManager& operator=(const EntityManager&) = delete;

        /**
         * @brief Move constructor.
         */
        EntityManager(EntityManager&&) noexcept = default;

        /**
         * @brief Move-assignment operator.
         */
        EntityManager& operator=(EntityManager&&) noexcept = default;


        /**
         * @brief Constructs an `EntityManager` and optionally reserves capacity.
         *
         * @param capacity Initial capacity for registry and sparse sets.
         */
        explicit EntityManager(const size_t capacity = 0)
        : capacity_(capacity) {
            reserve(capacity);
        }

        /**
         * @brief Reserves capacity for registry and already allocated sparse sets.
         *
         * @param capacity Requested capacity.
         */
        void reserve(const std::size_t capacity) {
            if (capacity > capacity_) {
                registry_.reserve(capacity);
                for (auto& component : components_) {
                    if (component) {
                        component->reserve(capacity);
                    }
                }
                capacity_ = capacity;
            }
        }

        /**
         * @brief Creates a new entity handle.
         *
         * @return Newly created handle.
         */
        [[nodiscard]] HandleType create() {
            return registry_.create();
        }

        /**
         * @brief Checks if an entity handle is valid.
         *
         * @param handle The handle to validate.
         *
         * @return `true` if the handle refers to a living entity.
         */
        [[nodiscard]] bool isValid(const HandleType handle) const noexcept {
            return registry_.isValid(handle);
        }

        /**
         * @brief Checks if an entity ID is valid.
         *
         * @param entityId The entity ID to validate.
         *
         * @return `true` if the entity ID refers to a living entity.
         */
        [[nodiscard]] bool isValid(const EntityId entityId) const noexcept {
            return registry_.isValid(handle(entityId));
        }

        /**
         * @brief Destroys an entity and removes all attached components.
         *
         * @param handle Handle to destroy.
         * @return `true` if the entity was destroyed.
         */
        [[nodiscard]] bool destroy(const HandleType handle) {

            if (!registry_.isValid(handle)) {
                return false;
            }

            for (size_t i = 0; i < components_.size(); i++) {

                if (!components_[i]) {
                    continue;
                }

                components_[i]->remove(handle.entityId());
            }

            registry_.destroy(handle);

            return true;
        }

        /**
         * @brief Retrieves a component for the given entity.
         *
         * @tparam T The component type to retrieve.
         *
         * @param handle The entity whose component to retrieve.
         *
         * @return Pointer to the component, or `nullptr` if the entity is invalid
         *         or does not have the requested component.
         */
        template<typename T>
        [[nodiscard]] T* get(const HandleType handle) const {
            if (!has<T>(handle)) {
                return nullptr;
            }

            const auto entityId = handle.entityId();
            const auto typeId = ComponentTypeId_type::template id<T>().value();

            auto* sparseSet = static_cast<SparseSet<T>*>(components_[typeId].get());

            return sparseSet->get(entityId);
        }

        /**
         * @brief Returns the SparseSet for a component type.
         *
         * @tparam T The component type.
         *
         * @return Const Pointer to the SparseSet, or `nullptr` if the type has no storage.
         */
        template<typename T>
        [[nodiscard]] const SparseSet<T>* sparseSet() const noexcept {

            const auto typeId = ComponentTypeId_type::template id<T>().value();

            if (typeId >= components_.size() || !components_[typeId]) {
                return nullptr;
            }

            return static_cast<SparseSet<T>*>(components_[typeId].get());
        }

        /**
         * @brief Returns the SparseSet for a component type (const).
         *
         * @tparam T The component type.
         *
         * @return Pointer to the SparseSet, or `nullptr` if the type has no storage.
         */
        template<typename T>
        [[nodiscard]] SparseSet<T>* sparseSet() {
            return const_cast<SparseSet<T>*>(std::as_const(*this).template sparseSet<T>());
        }

        /**
         * @brief Retrives a ptr to a SparseSetBase based on the ComponentTyoeIdType.
         *
         * @param typeId The type id of the component to look up
         *
         * @return The associated sparse set, or nullptr if not found.
         */
        [[nodiscard]] SparseSetBase* sparseSet(ComponentTypeId_type typeId)  noexcept {

            const auto idx = typeId.value();

            if (idx >= components_.size() || !components_[idx]) {
                return nullptr;
            }

            return &*components_[idx];
        }

        /**
         * @brief Checks whether an entity has a specific component.
         *
         * @tparam T The component type to check for.
         *
         * @param handle The entity to query.
         *
         * @return `true` if the entity has the component, `false` if the handle
         *         is invalid or the component is not attached.
         */
        template<typename T>
        [[nodiscard]] bool has(const HandleType handle) const {
            if (!registry_.isValid(handle)) {
                return false;
            }

            const auto typeId = ComponentTypeId_type::template id<T>().value();

            if (typeId < components_.size() && components_[typeId]) {
                return components_[typeId]->contains(handle.entityId());
            }

            return false;
        }

        /**
         * @brief Checks whether an entity has a component by type ID.
         *
         * @param handle The entity to query.
         * @param typeId The component type identifier.
         *
         * @return `true` if the entity has the component, `false` otherwise.
         */
        [[nodiscard]] bool has(const HandleType handle, const ComponentTypeId_type typeId) const {
            if (!registry_.isValid(handle)) {
                return false;
            }

            const auto tvalue = typeId.value();

            if (tvalue < components_.size() && components_[tvalue]) {
                return components_[tvalue]->contains(handle.entityId());
            }

            return false;
        }

        /**
         * @brief Ensures that a `SparseSet` for `TComponent` exists, creating it if necessary.
         *
         * Resizes the component storage vector and allocates a new `SparseSet`
         * when the type has not been seen before. Safe to call multiple times.
         *
         * @tparam TComponent Component type whose storage should be guaranteed.
         * @return Non-owning pointer to the (potentially newly created) `SparseSet`.
         */
        template<typename TComponent>
        [[nodiscard]] SparseSet<TComponent>* ensureSparseSet() {

            const auto typeId = ComponentTypeId_type::template id<TComponent>().value();

            if (typeId >= components_.size()) {
                components_.resize(typeId + 1);
            }

            if (!components_[typeId]) {
                components_[typeId] = std::make_unique<SparseSet<TComponent>>(capacity_);
            }

            return static_cast<SparseSet<TComponent>*>(components_[typeId].get());
        }

        /**
         * @brief Constructs and attaches a component to an entity.
         *
         * If the component type has not been registered yet, a new `SparseSet`
         * is created. The component is constructed in-place with the provided
         * arguments. Returns `nullptr` if the component already exists or if the
         * handle was invalid.
         *
         * @tparam T The component type to emplace.
         * @tparam Args Constructor argument types.
         *
         * @param handle The entity to attach the component to.
         * @param args Arguments forwarded to the component constructor.
         *
         * @return Pointer to the newly created component, or `nullptr` if the
         *         handle is invalid.
         */
        template<typename T, typename... Args>
        T* emplace(const HandleType handle, Args&& ...args) {

            if (!registry_.isValid(handle)) {
                return nullptr;
            }

            const auto entityId = handle.entityId();

            const auto typeId = ComponentTypeId_type::template id<T>().value();

            auto* sparseSet = ensureSparseSet<T>();

            if (sparseSet->contains(entityId)) {
                return nullptr;
            }

            return sparseSet->emplace(entityId, std::forward<Args>(args)...);
        }

        /**
         * @brief Returns existing component or creates a new one.
         *
         * @tparam T The component type.
         * @tparam Args Constructor argument types.
         *
         * @param handle The entity.
         * @param args Arguments forwarded to the constructor if creating.
         *
         * @return Pointer to the existing or newly created component,
         *         or `nullptr` if the handle is invalid.
         */
        template<typename T, typename... Args>
        T* emplaceOrGet(const HandleType handle, Args&& ...args) {

            if (!registry_.isValid(handle)) {
                return nullptr;
            }

            auto* raw = emplace<T>(handle, std::forward<Args>(args)...);

            if (!raw) {
                return get<T>(handle);
            }

            return raw;
        }

        /**
         * @brief Removes a specific component from an entity.
         *
         * Unlike `destroy()`, this only removes a single component type while
         * keeping the entity and other components intact.
         *
         * @tparam T The component type to remove.
         *
         * @param handle The entity whose component to remove.
         *
         * @return `true` if the component was removed, `false` if the handle was
         *         invalid or the component was not attached.
         *
         * @see destroy
         * @see SparseSet::remove
         */
        template<typename T>
        [[nodiscard]] bool remove(const HandleType& handle) {

            if (!has<T>(handle)) {
                return false;
            }

            const auto typeId = ComponentTypeId_type::template id<T>();

            return components_[typeId.value()]->remove(handle.entityId());
        }

        /**
         * @brief Returns true if the specified ComponentType is managed by this EntityManager.
         *
         * @tparam TComponent The component to check.
         *
         * @returns true if this component is managed by this Entitymanager.
         */
        template<typename TComponent>
        [[nodiscard]] bool managesDirty() {

            auto typeId = ComponentTypeId_type::template id<DirtyComponentSpec<TComponent>>().value();

            return typeId < components_.size() && components_[typeId];
        }

        /**
         * @brief Registers and allocates the dirty set for `TComponent`.
         *
         * @tparam TComponent Component type to track.
         */
        template<typename TComponent>
        void trackDirty() {
            auto typeId = ComponentTypeId_type::template id<DirtyComponentSpec<TComponent>>().value();

            if (typeId >= components_.size()) {
                components_.resize(typeId + 1);
            }

            // not calling ensureSparseSet() since we need to make sure registeredDirtySets
            // is pushed once with typeId
            if (!components_[typeId]) {
                components_[typeId] = std::make_unique<SparseSet<DirtyComponentSpec<TComponent>>>(capacity_);
                registeredDirtySets_.push_back(typeId);
            }
            #if HELIOS_DEBUG
            else {
                bool found = false;
                for (auto id : registeredDirtySets_) {
                    if (id == typeId) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    assert(false && "typeId found, but was missing in registeredDirtySets_");
                    //registeredDirtySets_.push_back(typeId);
                }

            }
            #endif


        }

        /**
         * @brief Clears all dirty sets that have been registered via `trackDirty()`.
         *
         * @todo garbage management when components are entirely removed and not managed by this manager anymore
         */
        void clearAllDirtySets() {

            for (const auto id : registeredDirtySets_) {
                if (id < components_.size() && components_[id]) {
                    components_[id]->clear();
                }
            }
        }

        /**
         * @brief Checks if the `SparseSet` for `DirtyComponentSpec<T>` exists and clears it.
         *
         * Accepts a variadic list of component types; all matching dirty sets are cleared.
         *
         * @tparam T Component types whose dirty sets should be cleared.
         */
        template<typename... T>
        void clearDirtySet() {
            ([this] {
                const auto typeId = ComponentTypeId_type::template id<DirtyComponentSpec<T>>().value();
                if (typeId < components_.size() && components_[typeId]) {
                    components_[typeId]->clear();
                }
            }(), ...);
        }

        /**
         * @brief Invokes `func` for every component type ID attached to an entity.
         *
         * Iterates over all allocated component slots and calls `func(ComponentTypeId)`
         * for each type whose `SparseSet` contains the given entity.
         * Does nothing if the handle is invalid.
         *
         * @tparam TFunc Callable with signature `void(ComponentTypeId_type)`.
         * @param handle The entity to query.
         * @param func   Callback invoked for each attached component type.
         */
        template<typename TFunc>
        void forEachComponentTypeId(const HandleType handle, TFunc&& func) const {
            if (!registry_.isValid(handle)) {
                return;
            }

            for (size_t i = 0; i < components_.size(); i++) {
                if (components_[i] && components_[i]->contains(handle.entityId())) {
                    std::forward<TFunc>(func)(ComponentTypeId_type{i});
                }
            }
         }

        /**
         * @brief Copies all components from source entity to target entity.
         *
         * @details Iterates through all components on the source and copies them
         * to the target by invoking `SparseSet::copy()` for each component type.
         * Skips component types that already exist on the target entity.
         *
         * @param source The entity to copy from.
         * @param target The entity to copy to.
         */
        bool copy(const HandleType source, const HandleType target) {

            if (!registry_.isValid(source) || !registry_.isValid(target)) {
                assert(false && "Source/target handle not valid.");
                return false;
            }

            forEachComponentTypeId(
                    source,
                [&](const ComponentTypeId_type typeId) {
                    if (!has(target, typeId)) {

                        std::ignore = components_[typeId.value()]->copy(source.entityId(), target.entityId());
                    }
                }
            );

            return true;
        }

        /**
         * @brief Creates a new entity and copies all components from a source entity in another EntityManager.
         *
         * @param sourceEntityManager The source entity manager to copy from.
         * @param sourceHandle The source handle to copy from. Must be valid.
         *
         * @return The newly created handle, or an invalid, temporary handle.
         */
        HandleType copyFrom(EntityManager& sourceEntityManager, const HandleType sourceHandle) {

            if (!sourceHandle.isValid()) {
                assert(false && "Source handle not valid.");
                return HandleType{};
            }

            auto targetHandle = create();
            bool called = false;
            bool allCopied = true;
            sourceEntityManager.forEachComponentTypeId(sourceHandle, [&](const ComponentTypeId_type typeId) {
                const auto idx = typeId.value();
                const auto* source = sourceEntityManager.sparseSet(typeId);
                if (idx >= components_.size()) {
                    components_.resize(idx + 1);
                }
                if (!components_[idx]) {
                    components_[idx] = source->makeEmpty();
                }
                allCopied = allCopied && source->copyTo(sourceHandle.entityId(), *components_[idx], targetHandle.entityId());
                called = true;
            });

            const bool success = called && allCopied;

            assert(success && "Failed to copy source to target");
            if (!success) {
                destroy(targetHandle);
                return HandleType{};
            }
            return targetHandle;
        }


        /**
         * @brief Returns raw void pointer to a component.
         *
         * @param handle The entity.
         * @param typeId The component type identifier.
         *
         * @return Raw pointer to the component, or `nullptr` if not found.
         */
        [[nodiscard]] void* raw(const HandleType handle, const ComponentTypeId_type typeId ) const {
            if (!has(handle, typeId)) {
                return nullptr;
            }

            return components_[typeId.value()]->raw(handle.entityId());
        }

        /**
         * @brief Reconstructs an EntityHandle from an EntityId.
         *
         * @param entityId The entity ID.
         *
         * @return EntityHandle with current version from the registry.
         */
        [[nodiscard]] HandleType handle(const EntityId entityId) const {
            return HandleType{entityId, registry_.version(entityId)};
        }

        /**
         * @brief Sorts the submitted list of SparseSets in a ascending order.
         *
         * Nullptr entries are treated as having the lowest value.
         *
         * @warning The caller must ensure exclusive access to the SparseSets since componentCount() is not thread-safe.
         *
         * @param sparseSets The list of SparseSets to sort.
         * @param sortCriteria The criteria to sort by.
         */
        void sort(std::vector<SparseSetBase*>& sparseSets, const SortCriteria sortCriteria) noexcept {

            switch (sortCriteria) {
                case SortCriteria::MaxEntityId:
                    std::sort(sparseSets.begin(), sparseSets.end(),
                        [this](const auto a, const auto b) noexcept{
                        const auto aMax = a ? a->maxEntityId() : Tombstone;
                        const auto bMax = b ? b->maxEntityId() : Tombstone;

                        const bool aTombstone = aMax == Tombstone;
                        const bool bTombstone = bMax == Tombstone;

                        if (aTombstone && bTombstone) {
                            return false;
                        }

                        if (aTombstone || bTombstone) {
                            return aTombstone && !bTombstone;
                        }

                        return aMax < bMax;
                    });
                return;
                case SortCriteria::ComponentCount:
                    std::sort(sparseSets.begin(), sparseSets.end(),
                        [this](const auto a, const auto b) noexcept{
                        const auto aMax = a ? a->componentCount() : 0;
                        const auto bMax = b ? b->componentCount() : 0;

                        return aMax < bMax;
                    });
                return;
            }

            std::unreachable();
        }

        /**
         * @brief Finalizes any outstanding mutations of the managed data containers.
         *
         * @warning This method is not thread safe.
         */
        void finalizeMutations() {
            for (const auto& set : components_) {
                if (set) {
                    set->finalizeMutations();
                }
            }
        }

        /**
         * @brief Finalizes pending mutation metadata for one component type.
         *
         * @param typeId Component type id.
         */
        void finalizeMutations(const ComponentTypeId_type typeId) {
            assert (components_[typeId.value()] && "Component-group not existing.");
            components_[typeId.value()]->finalizeMutations();
        }

    private:

        /**
         * @brief Dirty-set type IDs registered via `trackDirty()`.
         */
        std::vector<size_t> registeredDirtySets_;

        /**
         * @brief Component storage indexed by type ID.
         *
         * @todo sort after size()
         */
        std::vector<std::unique_ptr<SparseSetBase>> components_;

        /**
         * @brief Entity registry owned by this EntityManager.
         */
        EntityRegistry_type registry_;

        /**
         * @brief Initial reserved capacity for sparse sets.
         */
        size_t capacity_ = 0;
    };


}