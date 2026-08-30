/**
 * @file TypedHandleWorld.ixx
 * @brief Multi-domain ECS world dispatching by handle type.
 */
module;

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

export module helios.ecs.EcsWorld:TypedHandleWorld;

import helios.ecs.View;
import helios.ecs.Entity;
import helios.ecs.EntityManager;

/**
 * @brief Maps a handle type to its `EntityManager` index in a pack.
 *
 * @tparam THandle Handle type to resolve.
 * @tparam TEntityManagers Entity-manager pack.
 */
template <typename THandle, typename... TEntityManagers>
struct HandleToManager;

template <typename THandle, typename THead, typename... TTail>
struct HandleToManager<THandle, THead, TTail...> {
    static constexpr size_t value = [] {
        if constexpr (std::is_same_v<typename THead::Handle_type, THandle>) {
            return size_t{0};
        } else {
            return size_t{1} + HandleToManager<THandle, TTail...>::value;
        }
    }();
};

template <typename THandle, typename THead>
struct HandleToManager<THandle, THead> {
    static_assert(
        std::is_same_v<typename THead::Handle_type, THandle>, "No EntityManager found for the given handle type."
    );
    static constexpr size_t value = 0;
};

template <typename THandle>
struct HandleToManager<THandle> {
    static_assert(sizeof(THandle) == 0, "No EntityManager found for the given handle type.");
};

export namespace helios::ecs {

/**
 * @brief World containing one `EntityManager` per handle domain.
 *
 * Operations are dispatched at compile time via `THandle`.
 *
 * @tparam THandles Handle-domain types managed by this world.
 */
template <typename... THandles>
class TypedHandleWorld {

public:
    /**
     * @brief Tuple type of all underlying entity managers.
     */
    using EntityManager_types = std::tuple<EntityManager<THandles>...>;

    /**
     * @brief Constructs an empty typed-handle world.
     */
    TypedHandleWorld() = default;

    TypedHandleWorld(const TypedHandleWorld&) = delete;
    TypedHandleWorld& operator=(const TypedHandleWorld&) = delete;
    TypedHandleWorld(TypedHandleWorld&&) noexcept = default;
    TypedHandleWorld& operator=(TypedHandleWorld&&) noexcept = default;

    /**
     * @brief Returns the manager for `THandle`.
     *
     * @tparam THandle Target handle type.
     * @return Mutable manager reference.
     */
    template <typename THandle>
    auto& entityManager() {
        constexpr size_t idx = HandleToManager<THandle, EntityManager<THandles>...>::value;
        return std::get<idx>(entityManagers_);
    }

    /**
     * @brief Returns the const manager for `THandle`.
     *
     * @tparam THandle Target handle type.
     * @return Const manager reference.
     */
    template <typename THandle>
    const auto& entityManager() const {
        constexpr size_t idx = HandleToManager<THandle, EntityManager<THandles>...>::value;
        return std::get<idx>(entityManagers_);
    }

    /**
     * Returns the entity managers registered with this TypedHandelWorld.
     *
     * @return tuple of registered entity managers
     */
    auto& entityManagers() noexcept {
        return entityManagers_;
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

        return Entity{handle, &em};
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
    [[nodiscard]] auto find(THandle handle) {
        auto& em = entityManager<THandle>();

        using EM = std::remove_reference_t<decltype(em)>;
        using Entity_type = Entity<EM>;

        if (!em.isValid(handle)) {
            return std::optional<Entity_type>{std::nullopt};
        }

        return std::optional<Entity_type>{std::in_place, handle, &em};
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
        return View<EM, TComponents...>(&em);
    }

    /**
     * @brief Clears dirty sets for one handle domain.
     *
     * @tparam THandle Handle domain.
     * @tparam TComponents Optional component types to clear selectively.
     */
    template <typename THandle, typename... TComponents>
    void clearDirtySets() {
        auto& em = entityManager<THandle>();

        if constexpr (sizeof...(TComponents) == 0) {
            em.clearAllDirtySets();
        } else {
            (em.template clearDirtySet<TComponents>(), ...);
        }
    }

    /**
     * @brief Reserves capacity for one handle domain.
     *
     * @tparam THandle Handle domain.
     * @param capacity Requested capacity.
     */
    template <typename THandle>
    void reserve(std::size_t capacity) {
        auto& em = entityManager<THandle>();
        em.reserve(capacity);
    }

private:
    /**
     * @brief Storage of all domain-specific entity managers.
     */
    EntityManager_types entityManagers_{};
};

} // namespace helios::ecs
