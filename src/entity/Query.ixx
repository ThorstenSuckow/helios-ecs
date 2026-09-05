/**
 * @file Query.ixx
 * @brief Lightweight query for iterating entities with specific components.
 */
module;

#include <algorithm>
#include <functional>
#include <ranges>
#include <tuple>
#include <vector>

export module helios.ecs.entity.Query;

import :QueryTraits;
export import :QueryTypes;

import helios.ecs.component;

import helios.ecs.entity.storage.SparseSet;
import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;
import helios.ecs.entity.EntityAccessSet;
import helios.ecs.entity.EntityProxy;

import helios.ecs.common.concepts;
import helios.ecs.common.types;

import helios.core.common.types;
import helios.core.common.traits;

import helios.ecs.entity.EntityMutationBuffer;

using namespace helios::ecs::common::types;
using namespace helios::ecs::components;
using namespace helios::ecs::entity::storage;
using namespace helios::ecs::common::concepts::traits;

export namespace helios::ecs::entity {


template<typename TReadSet, typename TWriteSet, typename TFilter = Filter<AnyDirty<>>>
using Query = typename traits::QueryBuilder<TReadSet, TWriteSet, TFilter>::type;


template <typename TEntityManager, typename... TReadComponents, typename ... TWriteComponents, typename TFilter,  typename... TOptional>
requires core::common::traits::IsSubset<
    core::common::types::TypeList<TWriteComponents...>, core::common::types::TypeList<TReadComponents...>
    >::value
class PartialQuery<TEntityManager, core::common::types::TypeList<TReadComponents...>, core::common::types::TypeList<TWriteComponents...>, TFilter,  std::tuple<TOptional...>> {

    using EntityMutationBuffer = ecs::entity::EntityMutationBuffer<
        typename TEntityManager::HandleType, TWriteComponents...
    >;

    using DirtySetTraits = traits::DirtySetTrait<typename TFilter::dirtyList>;

private:
    TEntityManager* em_;


    /**
     * @brief Pointers to the SparseSets of the included components.
     */
    std::tuple<const SparseSet<TReadComponents>*...> includeSets_;

    std::tuple<SparseSet<TWriteComponents>*...> writeSets_;


    /**
     * @brief Optional components, might return nullptr. Are not considered by whereAnyDirty().
     */
    std::tuple<SparseSet<TOptional>*...> optionalSets_;

    /**
     * @brief Pointers to the SparseSets of the dirty component sets.
     */
    DirtySetTraits::tuple anyDirtySets_;

    /**
     * @brief List of exclusion predicates.
     * Stores functions that return true if an entity should be EXCLUDED.
     * Operates on EntityId (index) because the SparseSet uses it internally.
     */
    std::vector<std::function<bool(EntityId)>> excludeChecks_;


    /**
     * @brief Required SparseSets sorted by `componentCount` ascending.
     *
     * The set with fewest elements drives iteration (smallest lead set),
     * minimising the number of entities that must be validated against
     * the remaining sets.
     */
    std::vector<SparseSetBase*> sortedRequires_;

    /**
     * @brief Upper bound on `EntityId` values present in all required sets.
     *
     * Computed as the minimum `maxEntityId()` across all required sets
     * during construction. Used to short-circuit `isValid()` early.
     */
    EntityId maxEntityId_ = 0;

    EntityMutationBuffer* entityMutationBuffer_{};

    /**
     * @brief Populates `sortedRequires_` and computes `maxEntityId_`.
     *
     * Gathers all required-component SparseSets, determines the effective
     * entity-ID upper bound, and sorts the sets by `componentCount` so the
     * smallest set leads iteration.
     */
    void initializeRequiredSets() {

        (sortedRequires_.push_back(em_->template sparseSet<TReadComponents>()), ...);

        maxEntityId_ = std::ranges::min(std::views::transform(sortedRequires_, [](const auto* set) {
            return set ? set->maxEntityId() : Tombstone;
        }));

        em_->sort(sortedRequires_, SortCriteria::ComponentCount);
    };

public:

    using HandleType = typename TEntityManager::HandleType;

    using ReadSet = entity::ReadSet<TReadComponents...>;


    using WriteSet = entity::WriteSet<TWriteComponents...>;
    using DirtySet = DirtySetTraits::readSet;

    /**
     * @brief Constructs the view and retrieves the necessary component sets.
     *
     * @param em Pointer to the EntityManager to retrieve sets and construct Entities.
     */
    explicit PartialQuery(TEntityManager* em, EntityMutationBuffer* entityMutationBuffer = nullptr)
        : em_(em),
        includeSets_(std::make_tuple(em_->template sparseSet<TReadComponents>()...)),
        writeSets_(std::make_tuple(em_->template sparseSet<TWriteComponents>()...)),
        entityMutationBuffer_(entityMutationBuffer) ,
        anyDirtySets_(
            [em]<typename ... TDirty>(core::common::types::TypeList<TDirty...>){
                return std::make_tuple(em->template sparseSet<DirtyComponentSpec<TDirty>>()...);
            }(typename TFilter::dirtyList{})
        )

    {
        // Retrieve pointers to the specific component sets immediately.

        initializeRequiredSets();
    };

    /**
     * @brief Constructs the view with the specified component sets and filters.
     *
     * @param em Pointer to the EntityManager to retrieve sets and construct Entities.
     * @param includeSets Tuple of pointers to the SparseSets of the required components.
     * @param excludeChecks Vector of functions to determine if an entity should be excluded.
     * @param filterActiveOnly Flag to filter only entities with Active component.
     * @param activeSet Pointer to the SparseSet of Active components.
     */
    explicit PartialQuery(
        TEntityManager* em,
        std::tuple<const SparseSet<TReadComponents>*...> includeSets,
        std::tuple<SparseSet<TWriteComponents>*...> writeSets,
        std::vector<std::function<bool(EntityId)>> excludeChecks,
        EntityMutationBuffer* entityMutationBuffer = nullptr
    ) :
        em_(em),
        includeSets_(std::move(includeSets)),
        writeSets_(std::move(writeSets)),
        excludeChecks_(std::move(excludeChecks)),
        entityMutationBuffer_(entityMutationBuffer),

        optionalSets_(std::make_tuple(em_->template sparseSet<TOptional>()...)),
        anyDirtySets_(
            [em]<typename ... TDirty>(core::common::types::TypeList<TDirty...>){
                return std::make_tuple(em->template sparseSet<DirtyComponentSpec<TDirty>>()...);
            }(typename TFilter::dirtyList{})
        ) {

        initializeRequiredSets();
    }


    /**
     * @brief Excludes entities that have a specific component.
     *
     * @details Entities possessing the specified component type will be
     * skipped during iteration. Multiple exclusions can be chained.
     *
     * ```cpp
     * // Skip entities with Shield or Invincible
     * for (auto [e, health] : world->view<HealthComponent>()
     *     .exclude<ShieldComponent>()
     *     .exclude<InvincibleComponent>()) {
     *     // Only vulnerable entities
     * }
     * ```
     *
     * @tparam T The component type to exclude.
     *
     * @return Reference to this Query for method chaining.
     */
    template <typename T>
    PartialQuery& exclude() {
        auto* set = em_->template sparseSet<T>();

        if (set) {
            excludeChecks_.emplace_back([set](EntityId entityId) { return set->contains(entityId); });
        }
        return *this;
    }

    /**
     * @brief Returns true if the view has no entities to iterate over.
     *
     * @return boolean
     */
    [[nodiscard]] bool empty() {
        auto* leadSet = std::get<0>(includeSets_);
        if (leadSet == nullptr) {
            return true;
        }
        return begin() == end();
    }

    /**
     * @brief Forward iterator for Query traversal.
     *
     * @details Uses the first component type as the "lead" iterator and
     * validates each entity against all include/exclude criteria
     * before yielding.
     */
    struct Iterator {

        /** @brief `Entity` wrapper type produced on dereference. */
        using Entity_type = Entity<TEntityManager>;

        /** @brief Iterator over the lead set's dense entity-ID span. */
        using LeadIterator = std::span<const EntityId>::iterator;

        /** @brief Current position in the lead set's entity-ID span. */
        LeadIterator current_;

        /** @brief Sentinel past the last entity in the lead set. */
        LeadIterator end_;

        /** @brief Non-owning pointer to the owning `PartialQuery` for filter access. */
        const PartialQuery* view_;

        /**
         * @brief Default constructor creating an invalid iterator.
         */
        Iterator() = default;

        /**
         * @brief Constructs an iterator with the given range and view.
         *
         * @param current Iterator to the current position.
         * @param end Iterator to the end position.
         * @param view Pointer to the owning Query for filter access.
         */
        Iterator(LeadIterator current, LeadIterator end, const PartialQuery* view)
            : current_(current), end_(end), view_(view) {}

        /**
         * @brief Validates if the current entity matches all filter criteria.
         *
         * @details Performs the following checks in order:
         * 1. Entity validity in the registry
         * 2. Include check - entity has all required components
         * 3. Exclude check - entity has none of the excluded components
         *
         * @return True if the entity passes all checks, false otherwise.
         */
        [[nodiscard]] bool isValid() const {
            if (current_ == end_) {
                return true;
            }

            // 1. Get Entity ID (from the Lead Iterator)
            EntityId entityId = *current_;

            // dirty check
            if constexpr (DirtySet::size > 0) {
                const bool hasAnyDirtyIncludes = std::apply(
                    [entityId](auto*... sets) {
                        return ((sets && entityId <= sets->maxEntityId() && sets->contains(entityId)) || ...);
                    },
                    view_->anyDirtySets_
                );

                if (!hasAnyDirtyIncludes) {
                    return false;
                }
            }

            // 2. INCLUDE CHECK (Do we have all required components? - leadset mustnt be considered)
            for (std::size_t i = 1; i < view_->sortedRequires_.size(); ++i) {
                auto* set = view_->sortedRequires_[i];
                if (entityId > set->maxEntityId() || !set->contains(entityId)) {
                    return false;
                }
            }

            // 3. EXCLUDE CHECK (Must NOT be present)
            for (const auto& excludeCheck : view_->excludeChecks_) {
                if (excludeCheck(entityId)) {
                    return false; // If check returns true (has component), the entity is invalid.
                }
            }

            return true;
        }

        /**
         * @brief Advances to the next valid entity.
         *
         * @details Increments the underlying iterator and skips invalid
         * entities until a valid one is found or end is reached.
         */
        void advance() {
            do {
                ++current_;
            } while (current_ != end_ && !isValid());
        }

        /**
         * @brief Pre-increment operator.
         *
         * @return Reference to this iterator after advancing.
         */
        Iterator& operator++() noexcept {
            advance();
            return *this;
        }

        /**
         * @brief Inequality comparison.
         *
         * @param other The iterator to compare against.
         *
         * @return True if iterators point to different positions.
         */
        bool operator!=(const Iterator& other) const noexcept {
            return current_ != other.current_;
        }

        /**
         * @brief Active components are never part of the returned tuple.
         *
         * @tparam TSet The type of the component set.
         * @param entityId The ID of the entity.
         * @param set The component set.
         * @return A tuple containing the component if it's not active, otherwise an empty tuple.
         */
        template<typename TComponent>
        auto includeComponent(EntityId entityId, const SparseSet<TComponent>* set) const {
            using ComponentType = TComponent;

            if constexpr (IsActiveComponent_v<ComponentType>) {

                return std::tuple{}; // Active components are not included in the returned tuple.

            } else if constexpr (core::common::traits::IsInList<ComponentType, TWriteComponents...>::value) {

                auto* writeSet = std::get<SparseSet<TComponent>*>(view_->writeSets_);
                return std::make_tuple(writeSet->get(entityId));

            } else {
                return std::make_tuple(set->get(entityId));
            }
        }

        /**
         * @brief Dereference operator.
         *
         * @return A tuple containing:
         *         1) `Entity_handle`,
         *         2) pointers to all required components,
         *         3) pointers to all optional components.
         *
         * Optional component pointers may be `nullptr` when the current
         * entity does not own the respective component.
         *
         * @note Returns by value to support C++17 structured binding.
         */
        [[nodiscard]] auto operator*() const {
            EntityId entityId = *current_;
            auto handle = view_->em_->handle(entityId);

            return std::tuple_cat(
                std::make_tuple(EntityProxy<typename TEntityManager::HandleType, TWriteComponents...>(
                    handle, view_->entityMutationBuffer_)),

                // tuple_cat is required to make sure ActiveComponent is not included
                // since this is treated as meta information we are not interested in
                std::apply(
                    [this, entityId](auto*... sets) {
                        return std::tuple_cat(
                            includeComponent(entityId, sets)...
                        );
                    },
                    view_->includeSets_
                ),

                std::apply(
                    [entityId](auto*... sets) {
                        return std::make_tuple(
                            ([entityId, &sets]() {
                                if (!sets || !sets->contains(entityId)) {
                                    return nullptr;
                                }

                                return sets->get(entityId);
                            }())...

                        );
                    },
                    view_->optionalSets_
                )

            );
        }

        [[nodiscard]] bool operator==(const Iterator& other) const noexcept {
            return current_ == other.current_;
        }
    };

    /**
     * @brief Returns an iterator to the first valid entity.
     *
     * @details Uses the first component type's SparseSet as the lead.
     * If the first entity is invalid, advances to the next valid one.
     *
     * @return Iterator to the first valid entity, or end() if none found.
     */
    [[nodiscard]] Iterator begin() {

        if (sortedRequires_.empty()) {
            return end();
        }

        // check if any nullptr occurs
        for (const auto* set : sortedRequires_) {
            if (set == nullptr) {
                return end();
            }
        }

        // dirty check. If dirty checks are required, but empty, there is no result set
        if constexpr (DirtySet::size > 0) {
            const bool areDirtyIncludesEmpty =
                std::apply([](auto*... sets) {
                    return ((sets && sets->componentCount() == 0) && ...);
                }, anyDirtySets_);

            if (areDirtyIncludesEmpty) {
                return end();
            }
        }

        const auto entities = sortedRequires_.front()->entityIds();

        Iterator it{entities.begin(), entities.end(), this};

        if (!it.isValid()) {
            it.advance();
        }

        return it;
    }

    /**
     * @brief Returns an iterator to the end (past the last entity).
     *
     * @return End iterator for comparison.
     */
    [[nodiscard]] Iterator end() {
        if (sortedRequires_.empty()) {
            return Iterator{};
        }

        // check if any nullptr occurs
        for (const auto* set : sortedRequires_) {
            if (set == nullptr) {
                return Iterator{};
            }
        }

        const auto entities = sortedRequires_.front()->entityIds();
        return Iterator{entities.end(), entities.end(), this};
    }
};

} // namespace helios::ecs