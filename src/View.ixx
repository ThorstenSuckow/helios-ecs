/**
 * @file View.ixx
 * @brief Lightweight view for iterating entities with specific components.
 */
module;

#include <tuple>
#include <vector>
#include <functional>
#include <scoped_allocator>
#include <algorithm>
#include <ranges>

export module helios.ecs.View;

import helios.ecs.components;
import helios.ecs.SparseSet;
import helios.ecs.EntityManager;
import helios.ecs.Entity;
import helios.ecs.concepts.Traits;
import helios.ecs.types.EntityHandle;

import helios.ecs.types;

using namespace helios::ecs::types;
using namespace helios::ecs::components;
using namespace helios::ecs::concepts::traits;
export namespace helios::ecs {

    /**
     * @brief A view class to iterate over entities having specific components.
     *
     * The View acts as a lightweight iterator over the SparseSets of the requested
     * components. It uses the first component type (Lead) as the primary iterator
     * and cross-references existence in other sets.
     *
     * ## Usage
     *
     * ```cpp
     * for (auto [entity, transform, velocity, active] : world.view<
     *     GameHandle,
     *     TransformComponent,
     *     VelocityComponent,
     *     Active
     * >().whereAnyChanged().withOptional<MaybeComponent>()) {
     *     // Process entity
     * }
     * ```
     *
     * @tparam TEntityManager The concrete `EntityManager` specialisation to
     *                        iterate over. Determines the handle type and
     *                        component storage used.
     * @tparam TIncludedComponents Tuple of required component types.
     * @tparam TDirtyComponents Tuple of dirty component types.
     * @tparam TOptionalComponents Tuple of optional component types.
     *
     * @see EntityManager
     * @see SparseSet
     * @see TypedHandleWorld
     */
    template<typename TEntityManager, typename TIncludedComponents, typename TDirtyComponents, typename TOptionalComponents>
    class PartialView;

    /**
     * @brief Convenience alias for a view with required components only.
     *
     * Optional components can be attached fluently via `withOptional<...>()`.
     *
     * @tparam TEntityManager Concrete entity manager type.
     * @tparam TRequired Required component types that must be present.
     */
    template<typename TEntityManager, typename... TIncluded>
    using View = PartialView<TEntityManager, std::tuple<TIncluded...>, std::tuple<>, std::tuple<>>;

    template<typename TEntityManager, typename... TIncluded, typename... TDirty, typename... TOptional>
    class PartialView<TEntityManager, std::tuple<TIncluded...>, std::tuple<TDirty...>, std::tuple<TOptional...>> {

    private:
        TEntityManager* em_;

        /**
         * @brief Pointers to the SparseSets of the included components.
         */
        std::tuple<SparseSet<TIncluded>*... > includeSets_;

        /**
         * @brief Optional components, might return nullptr. Are not considered by whereAnyDirty().
         */
        std::tuple<SparseSet<TOptional>*... > optionalSets_;

        /**
         * @brief Pointers to the SparseSets of the dirty component sets.
         */
        std::tuple<SparseSet<DirtyComponentSpec<TDirty>>*... > anyDirtySets_;


        /**
         * @brief List of exclusion predicates.
         * Stores functions that return true if an entity should be EXCLUDED.
         * Operates on EntityId (index) because the SparseSet uses it internally.
         */
        std::vector<std::function<bool(EntityId)>> excludeChecks_;

        /**
         * @brief Flag to filter only entities with Active component.
         */
        bool filterActiveOnly_ = false;

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

        /**
         * @brief Populates `sortedRequires_` and computes `maxEntityId_`.
         *
         * Gathers all required-component SparseSets, determines the effective
         * entity-ID upper bound, and sorts the sets by `componentCount` so the
         * smallest set leads iteration.
         */
        void initializeRequiredSets() {
            (sortedRequires_.push_back(em_->template sparseSet<TIncluded>()), ...);

            maxEntityId_ = std::ranges::min(sortedRequires_ | std::views::transform([](const auto* set){
                return set ? set->maxEntityId() : Tombstone;
            }));

            em_->sort(sortedRequires_, SortCriteria::ComponentCount);
        };

    public:
        /**
         * @brief Constructs the view and retrieves the necessary component sets.
         *
         * @param em Pointer to the EntityManager to retrieve sets and construct Entities.
         */
        explicit PartialView(TEntityManager* em)
            requires (sizeof...(TOptional) == 0 && sizeof...(TDirty) == 0)
        : em_(em),
        includeSets_(std::make_tuple(em_->template sparseSet<TIncluded>()...))
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
        explicit PartialView(
            TEntityManager* em,
            std::tuple<SparseSet<TIncluded>*...> includeSets,
            std::vector<std::function<bool(EntityId)>> excludeChecks,
            const bool filterActiveOnly
        )  : em_(em),
            includeSets_(std::move(includeSets)),
            excludeChecks_(std::move(excludeChecks)),
            filterActiveOnly_(filterActiveOnly),

            optionalSets_(std::make_tuple(em_->template sparseSet<TOptional>()...)),
            anyDirtySets_(std::make_tuple(em_->template sparseSet<DirtyComponentSpec<TDirty>>()...)) {

            initializeRequiredSets();
        }

        /**
         * @brief Adds optional component types to the current view.
         *
         * @details Optional components do not participate in entity filtering.
         * For each optional type, iteration yields either a component pointer or
         * `nullptr` if the entity does not own that component.
         *
         * This method is intended to be called once with all optional types:
         *
         * ```cpp
         * for (auto [e, transform, velocity, maybeHealth, maybeShield] : world
         *     .view<EntityManager, TransformComponent, VelocityComponent>()
         *     .withOptional<HealthComponent, ShieldComponent>()) {
         *     // maybeHealth / maybeShield may be nullptr
         * }
         * ```
         *
         * @tparam TNewOptional Optional component types to expose in iteration.
         *
         * @return A new `PartialView` with unchanged required components and
         *         the provided optional component types.
         */
        template<typename... TNewOptional>
        auto withOptional()
            requires (sizeof...(TOptional) == 0)
        {
            return PartialView<
                TEntityManager,
                std::tuple<TIncluded...>,
                std::tuple<TDirty...>,
                std::tuple<TNewOptional...>>(
                em_,
                includeSets_,
                excludeChecks_,
                filterActiveOnly_,
                anyDirtySets_
            );
        }

        /**
         * @brief Filters to only include entities with changed components.
         *
         * @return Reference to this View for method chaining.
         */
        template<typename... TNewDirty>
        auto whereAnyDirty() requires (sizeof...(TOptional) == 0 && sizeof...(TDirty) == 0)  {
            return PartialView<
                TEntityManager,
                std::tuple<TIncluded...>,
                std::tuple<TNewDirty...>,
                std::tuple<>
            >(
                em_,
                includeSets_,
                excludeChecks_,
                filterActiveOnly_
            );

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
         * @return Reference to this View for method chaining.
         */
        template<typename T>
        PartialView& exclude() {
            auto* set = em_->template sparseSet<T>();

            if (set) {
                excludeChecks_.emplace_back([set](EntityId entityId) {
                    return set->contains(entityId);
                });
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
            if (!leadSet) {
                return true;
            }
            return begin() == end();
        }

        /**
         * @brief Filters to only include entities with an active component.
         *
         * @return Reference to this View for method chaining.
         */
        auto withActive()
            requires (sizeof...(TOptional) == 0 && sizeof...(TDirty) == 0)
        {
            using ActiveComponent = Active<typename TEntityManager::Handle_type>;

            return PartialView<
               TEntityManager,
               std::tuple<ActiveComponent, TIncluded...>,
               std::tuple<>,
               std::tuple<>
           >(
               em_,
               std::tuple_cat(
                    std::make_tuple(em_->template sparseSet<ActiveComponent>()), includeSets_
                ),
               excludeChecks_,
               true
           );
        }

        /**
         * @brief Forward iterator for View traversal.
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

            /** @brief Non-owning pointer to the owning `PartialView` for filter access. */
            const PartialView* view_;

            /**
             * @brief Default constructor creating an invalid iterator.
             */
            Iterator() = default;

            /**
             * @brief Constructs an iterator with the given range and view.
             *
             * @param current Iterator to the current position.
             * @param end Iterator to the end position.
             * @param view Pointer to the owning View for filter access.
             */
            Iterator(LeadIterator current, LeadIterator end, const PartialView* view)
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

                if (entityId > view_->maxEntityId_ || !view_->em_->isValid(entityId)) {
                    return false;
                }

                // 2. INCLUDE CHECK (Do we have all required components?)
                for (auto* set : view_->sortedRequires_) {
                    if (entityId > set->maxEntityId() || !set->contains(entityId)) {
                        return false;
                    }
                }


                // dirty check
                if constexpr (sizeof...(TDirty) > 0) {
                    const bool hasAnyDirtyIncludes = std::apply([entityId](auto*... sets) {
                       return ((sets && entityId <= sets->maxEntityId() && sets->contains(entityId)) || ...);
                   }, view_->anyDirtySets_);

                    if (!hasAnyDirtyIncludes) {
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
            template<typename TSet>
            static auto includeComponent(EntityId entityId, TSet* set) {
                using Component_type = typename TSet::Component_type;

                if constexpr(IsActiveComponent_v<Component_type>) {
                    return std::tuple{};
                } else {
                    return std::make_tuple(set->get(entityId));
                }

            }

            /**
             * @brief Dereference operator.
             *
             * @return A tuple containing:
             *         1) `Entity_type`,
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
                auto filterActiveOnly = view_->filterActiveOnly_;

                return std::tuple_cat(
                    std::make_tuple(Entity_type(handle, view_->em_)),

                    std::apply([entityId](auto*... sets) {
                        return std::tuple_cat(includeComponent(entityId, sets)...);
                    }, view_->includeSets_),

                    std::apply([
                        entityId
                    ](auto*... sets) {
                        return std::make_tuple(
                        ([entityId, &sets]() {
                            if (!sets || !sets->contains(entityId)) {
                                return nullptr;
                            }

                            return sets->get(entityId);
                        }())...

                        );
                    }, view_->optionalSets_)

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
                return Iterator{};
            }

            // check if any nullptr occurs
            for (const auto* set : sortedRequires_) {
                if (!set) {
                    return Iterator{};
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
                if (!set) {
                    return Iterator{};
                }
            }

            const auto entities = sortedRequires_.front()->entityIds();
            return Iterator{entities.end(), entities.end(), this};
        }

    };
}