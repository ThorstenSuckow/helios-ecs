module;

#include <cassert>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <utility>
#include <vector>

export module helios.ecs.entity.EntityWorld;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;
import helios.ecs.entity.EntityAccessSet;

import helios.core.common.traits;
import helios.ecs.common.types;

import helios.ecs.common;
import helios.ecs.entity.storage;
import :TypedHandleWorld;

export namespace helios::ecs::entity {

/**
 * @brief Type-erased facade for accessing TypedHandleWorld instances.
 */
class EntityWorld {

    class EntityManagerRef {
        void* entityManager_{};
        void (*clearAllDirtySets_)(void*) noexcept {};

    public:
        template <typename TEntityManager>
        explicit EntityManagerRef(TEntityManager& entityManager) noexcept
            : entityManager_(std::addressof(entityManager)), clearAllDirtySets_(+[](void* entityManager) noexcept {
                  static_cast<TEntityManager*>(entityManager)->clearAllDirtySets();
              }) {}

        void clearAllDirtySets() noexcept {
            clearAllDirtySets_(entityManager_);
        }
    };

    class Concept {
    public:
        virtual ~Concept() = default;
    };

    template <typename TTypedHandleWorld>
    class Model final : public Concept {
        TTypedHandleWorld typedHandleWorld_;

    public:
        explicit Model(TTypedHandleWorld typedHandleWorld) : typedHandleWorld_(std::move(typedHandleWorld)) {}

        auto& entityManagers() {
            return typedHandleWorld_.entityManagers();
        }
    };

    std::unique_ptr<Concept> pimpl_;
    std::vector<void*> entityManagers_;
    std::vector<EntityManagerRef> entityManagerRefs;

    template <typename TEntityManager>
    void registerEntityManager(TEntityManager& entityManager) {
        using HandleType = TEntityManager::HandleType;
        auto typeId = common::types::HandleTypeId::template id<HandleType>();
        auto idx = typeId.value();
        if (entityManagers_.size() <= idx) {
            entityManagers_.resize(idx + 1, nullptr);
        }
        entityManagers_[idx] = std::addressof(entityManager);
        entityManagerRefs.emplace_back(entityManager);
    }

public:
    EntityWorld() = delete;
    EntityWorld(const EntityWorld&) = delete;
    EntityWorld& operator=(const EntityWorld&) = delete;

    EntityWorld(EntityWorld&&) noexcept = default;
    EntityWorld& operator=(EntityWorld&&) noexcept = default;

    template <typename... THandles>
    static EntityWorld make() noexcept {
        return EntityWorld{TypedHandleWorld<THandles...>{}};
    }

    template <typename TTypedHandleWorld>
        requires(!std::is_lvalue_reference_v<TTypedHandleWorld>)
    explicit EntityWorld(TTypedHandleWorld&& typedHandleWorld) {

        using TypedHandleWorld = std::remove_cvref_t<TTypedHandleWorld>;
        auto model = std::make_unique<Model<TypedHandleWorld>>(std::forward<TTypedHandleWorld>(typedHandleWorld));

        std::apply(
            [&]<typename... TEntityManager>(TEntityManager&... entityManager) {
                (registerEntityManager<TEntityManager>(entityManager), ...);
            },
            model->entityManagers()
        );

        pimpl_ = std::move(model);
    };

    template <typename THandle>
    entity::EntityManager<THandle>& entityManager() {
        return const_cast<entity::EntityManager<THandle>&>(std::as_const(*this).entityManager<THandle>());
    }

    template <typename THandle>
    [[nodiscard]] const entity::EntityManager<THandle>& entityManager() const {

        auto idx = common::types::HandleTypeId::template id<THandle>().value();

        if (idx >= entityManagers_.size()) [[unlikely]] {
            std::cerr << "No EntityManager registered for the given handle type:" << typeid(THandle).name()
                      << '\n';
            assert(false && "No EntityManager registered for the given handle type.");
            std::terminate();
        }

        return *static_cast<entity::EntityManager<THandle>*>(entityManagers_[idx]);
    }

    /**
     * @brief Creates an entity in the `THandle` domain.
     *
     * @tparam THandle Target handle type.
     * @return Entity facade for the new entity.
     */
    template <typename THandle>
    [[nodiscard]] auto add() {
        auto& em = entityManager<THandle>();

        auto handle = em.create();

        return entity::Entity{handle, &em};
    }

    /**
     * @brief Destroys an entity by handle.
     *
     * @tparam THandle Target handle type.
     * @param handle Handle to destroy.
     * @return `true` if destruction succeeded.
     */
    template <typename THandle>
    bool destroy(THandle handle) {
        auto& em = entityManager<THandle>();

        return em.destroy(handle);
    }

    /**
     * @brief Finds an entity by handle.
     *
     * @tparam THandle Target handle type.
     * @param handle Handle to resolve.
     * @return Optional entity facade.
     */
    template <typename THandle>
    [[nodiscard]] std::optional<entity::Entity<entity::EntityManager<THandle>>> find(THandle handle) {
        auto& em = entityManager<THandle>();

        return em.entity(handle);
    }

    /**
     * @brief Creates a new entity and copies all components from `source`.
     *
     * @tparam THandle Target handle type.
     * @param source Source entity handle.
     * @return Entity facade for the cloned entity.
     */
    template <typename THandle>
    [[nodiscard]] auto copy(THandle source) noexcept {
        auto& em = entityManager<THandle>();

        auto entity = add<THandle>();

        em.copy(source, entity.handle());

        return entity;
    }

    /**
     * @brief Creates a typed view for one handle domain.
     *
     * @tparam THandle Handle domain.
     * @tparam TComponents Component filter pack.
     * @return View object for iterating matching entities.
     */
    template <typename THandle, typename... TComponents>
    [[nodiscard]] auto view() {
        auto& em = entityManager<THandle>();
        using EM = std::remove_reference_t<decltype(em)>;
        return View<EM, entity::Read<TComponents...>, entity::Write<TComponents...>>(&em);
    }

    /**
     * @brief Clears dirty sets for one handle domain.
     *
     * @tparam THandle Handle domain.
     * @tparam TComponents Optional component types to clear selectively.
     */
    template <typename THandle = void, typename... TComponents>
    void clearDirtySets() noexcept {

        if constexpr (std::is_same_v<THandle, void>) {
            for (auto& emRef : entityManagerRefs) {
                emRef.clearAllDirtySets();
            }
        } else {
            auto& em = entityManager<THandle>();

            if constexpr (sizeof...(TComponents) == 0) {
                em.clearAllDirtySets();
            } else {
                (em.template clearDirtySet<TComponents>(), ...);
            }
        }
    }

    /**
     * @brief Check whether the specified handle is valid.
     *
     * @tparam THandle Handle domain.
     * @param handle Handle to check.
     * @return True if the handle is valid, false otherwise.
     */
    template <typename THandle>
    [[nodiscard]] bool isValid(const THandle handle) const noexcept {
        auto& em = entityManager<THandle>();
        return em.isValid(handle);
    }

    /**
     * @brief Returns the underlying SparseSet managing the handle domain.
     * @tparam THandle
     * @return SparseSet<THandle> or nullptr if not available.
     */
    template <typename THandle, typename TComponent>
    [[nodiscard]] entity::storage::SparseSet<TComponent>* sparseSet() {
        auto& em = entityManager<THandle>();
        return em.template sparseSet<TComponent>();
    }
};

}; // namespace helios::ecs