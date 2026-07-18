/**
 * @file View.ixx
 * @brief Lightweight view for iterating entities with specific components.
 */
module;

#include <tuple>
#include <vector>
#include <functional>

export module helios.ecs.View;

import helios.ecs.components;
import helios.ecs.SparseSet;
import helios.ecs.types.TypeDefs;
import helios.ecs.EntityManager;
import helios.ecs.Entity;
import helios.ecs.concepts.Traits;
import helios.ecs.types.EntityHandle;

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
     * >().whereAllEnabled().whereAnyChanged().withOptional<MaybeComponent>()) {
     *     // Process entity
     * }
     * ```
     *
     * @tparam TEntityManager The concrete `EntityManager` specialisation to
     *                        iterate over. Determines the handle type and
     *                        component storage used.
     * @tparam TRequiredComponents Tuple of required component types.
     * @tparam TDirtyComponents Tuple of dirty component types.
     * @tparam TOptionalComponents Tuple of optional component types.
     *
     * @see EntityManager
     * @see SparseSet
     * @see TypedHandleWorld
     */
    template<typename TEntityManager, typename TRequiredComponents, typename TDirtyComponents, typename TOptionalComponents>
    class PartialView;

    /**
     * @brief Convenience alias for a view with required components only.
     *
     * Optional components can be attached fluently via `withOptional<...>()`.
     *
     * @tparam TEntityManager Concrete entity manager type.
     * @tparam TRequired Required component types that must be present.
     */
    template<typename TEntityManager, typename... TRequired>
    using View = PartialView<TEntityManager, std::tuple<TRequired...>, std::tuple<>, std::tuple<>>;

    template<typename TEntityManager, typename... TRequired, typename... TDirty, typename... TOptional>
    class PartialView<TEntityManager, std::tuple<TRequired...>, std::tuple<TDirty...>, std::tuple<TOptional...>> {

    private:
        TEntityManager* em_;

        /**
         * @brief Pointers to the SparseSets of the included components.
         */
        std::tuple<SparseSet<TRequired>*... > includeSets_;

        /**
         * @brief Optional components, might return nullptr. Are not considered by whereAllEnabled() or whereAnyChanged().
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
         * @brief Flag to filter only enabled components.
         */
        bool filterEnabledOnly_ = false;


        /**
         * @brief SparseSet for Active component, required with filterActiveOnly_.
         */
        SparseSet<Active<typename TEntityManager::Handle_type>>* activeSet_ = nullptr;

        /**
         * @brief Flag to filter only entities with Active component.
         */
        bool filterActiveOnly_ = false;

    public:
        /**
         * @brief Constructs the view and retrieves the necessary component sets.
         *
         * @param em Pointer to the EntityManager to retrieve sets and construct Entities.
         */
        explicit PartialView(TEntityManager* em)
            requires (sizeof...(TOptional) == 0 && sizeof...(TDirty) == 0)
        : em_(em) {
            // Retrieve pointers to the specific component sets immediately.
            includeSets_ = std::make_tuple(em_->template getSparseSet<TRequired>()...);
        };


        /**
         * @brief Constructs the view with the specified component sets and filters.
         *
         * @param em Pointer to the EntityManager to retrieve sets and construct Entities.
         * @param includeSets Tuple of pointers to the SparseSets of the required components.
         * @param excludeChecks Vector of functions to determine if an entity should be excluded.
         * @param filterEnabledOnly Flag to filter only enabled components.
         * @param filterActiveOnly Flag to filter only entities with Active component.
         * @param activeSet Pointer to the SparseSet of Active components.
         */
        explicit PartialView(
            TEntityManager* em,
            std::tuple<SparseSet<TRequired>*...> includeSets,
            std::vector<std::function<bool(EntityId)>> excludeChecks,
            const bool filterEnabledOnly,
            const bool filterActiveOnly,
            SparseSet<Active<typename TEntityManager::Handle_type>>* activeSet
        )  : em_(em),
            includeSets_(std::move(includeSets)),
            excludeChecks_(std::move(excludeChecks)),
            filterEnabledOnly_(filterEnabledOnly),
            filterActiveOnly_(filterActiveOnly),
            activeSet_(activeSet),

            optionalSets_(std::make_tuple(em_->template getSparseSet<TOptional>()...)),
            anyDirtySets_(std::make_tuple(em_->template getSparseSet<DirtyComponentSpec<TDirty>>()...))
        {}

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
                std::tuple<TRequired...>,
                std::tuple<TDirty...>,
                std::tuple<TNewOptional...>>(
                em_,
                includeSets_,
                excludeChecks_,
                filterEnabledOnly_,
                filterActiveOnly_,
                activeSet_,
                anyDirtySets_
            );
        }

        /**
         * @brief Filters to only include entities with changed components.
         *
         * @details Components must implement `hasChanges()` returning bool.
         * Components without this method are assumed to have changes.
         *
         * @return Reference to this View for method chaining.
         */
        template<typename... TNewDirty>
        auto whereAnyDirty() requires (sizeof...(TOptional) == 0 && sizeof...(TDirty) == 0)  {

            return PartialView<
                TEntityManager,
                std::tuple<TRequired...>,
                std::tuple<TNewDirty...>,
                std::tuple<>
            >(
                em_,
                includeSets_,
                excludeChecks_,
                filterEnabledOnly_,
                filterActiveOnly_,
                activeSet_
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
            auto* set = em_->template getSparseSet<T>();

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
        PartialView& withActive() {
            activeSet_ = em_->template getSparseSet<Active<typename TEntityManager::Handle_type>>();
            filterActiveOnly_ = true;
            return *this;
        }

        /**
         * @brief Filters to only include entities with enabled components.
         *
         * @details Components must implement `isEnabled()` returning bool.
         * Components without this method are assumed to be enabled.
         *
         * @return Reference to this View for method chaining.
         */
        PartialView& whereAllEnabled() {
            filterEnabledOnly_ = true;
            return *this;
        }


        /**
         * @brief Forward iterator for View traversal.
         *
         * @details Uses the first component type as the "lead" iterator and
         * validates each entity against all include/exclude/enabled criteria
         * before yielding.
         */
        struct Iterator {

            using Entity_type = Entity<TEntityManager>;

            /**
             * @brief The first component type determines iteration order.
             */
            using LeadComponent = std::tuple_element_t<0, std::tuple<TRequired...>>;

            /**
             * @brief Iterator type from the lead component's SparseSet.
             */
            using LeadIterator  = typename SparseSet<LeadComponent>::Iterator;

            LeadIterator current_;
            LeadIterator end_;
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
             * 4. Enabled check - all components pass isEnabled() (if filtered)
             *
             * @return True if the entity passes all checks, false otherwise.
             */
            [[nodiscard]] bool isValid() const {
                if (current_ == end_) {
                    return true;
                }

                // 1. Get Entity ID (from the Lead Iterator)
                EntityId entityId = current_.entityId();

                if (!view_->em_->isValid(entityId)) {
                    return false;
                }

                // 2. INCLUDE CHECK (Do we have all other required components?)
                // We iterate over the tuple of sets and check 'contains' for each.
                const bool hasAllIncludes = std::apply([entityId](auto*... sets) {
                    return ((sets && sets->contains(entityId)) && ...);
                }, view_->includeSets_);

                if (view_->filterActiveOnly_ && (!view_->activeSet_ || !view_->activeSet_->contains(entityId))) {
                    return false;
                }

                if (!hasAllIncludes) {
                    return false;
                }

                // dirty check
                if constexpr (sizeof...(TDirty) > 0) {
                    const bool hasAnyDirtyIncludes = std::apply([entityId](auto*... sets) {
                       return ((sets && sets->contains(entityId)) || ...);
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

                // 4. ENABLED CHECK (State)
                if (view_->filterEnabledOnly_) {

                    // SFINAE Helper Lambda: Checks if .isEnabled() exists.
                    auto isComponentEnabled = [](const auto& comp) -> bool {
                        if constexpr (concepts::traits::HasToggleable<std::remove_cvref_t<decltype(comp)>>) {
                            return comp.isEnabled();
                        } else {
                            return true; // Assume enabled if method is missing.
                        }
                    };

                    // Check the Lead component (*current_ returns the component reference)
                    if (!isComponentEnabled(*current_)) {
                        return false;
                    }

                    // Check all other included components
                    const bool allEnabled = std::apply([&](auto*... sets) {
                        // sets->get(id) returns a pointer, *ptr gives the reference.
                        return (isComponentEnabled(*sets->get(entityId)) && ...);
                    }, view_->includeSets_);

                    if (!allEnabled) {
                        return false;
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
             * @brief Dereference operator.
             *
             * @return A tuple containing:
             *         1) `Entity_type`,
             *         2) pointers to all required components,
             *         3) pointers to all optional components.
             *
             * Optional component pointers may be `nullptr` when the current
             * entity does not own the respective component (or it is filtered by
             * `whereAllEnabled()` when `isEnabled()` is available).
             *
             * @note Returns by value to support C++17 structured binding.
             */
            [[nodiscard]] auto operator*() const {
                EntityId entityId = current_.entityId();
                auto handle = view_->em_->handle(entityId);


                return std::tuple_cat(
                    std::make_tuple(Entity_type(handle, view_->em_)),
                    std::apply([entityId](auto*... sets) {
                        return std::make_tuple(sets->get(entityId)...);
                    }, view_->includeSets_),

                    std::apply([
                        entityId,
                        filterEnabledOnly = view_->filterEnabledOnly_
                    ](auto*... sets) {
                        return std::make_tuple(
                        ([filterEnabledOnly, entityId, &sets]() {
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
            auto* leadSet = std::get<0>(includeSets_);

            if (!leadSet) {
                return Iterator{};
            }

            Iterator it{leadSet->begin(), leadSet->end(), this};

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
            auto* leadSet = std::get<0>(includeSets_);

            if (!leadSet) {
                return Iterator{};
            }

            return Iterator{leadSet->end(), leadSet->end(), this};
        }

    };
}