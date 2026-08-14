/**
 * @file ContextTypeId.ixx
 * @brief Runtime-assigned type identifier for context types.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.common.types:ContextTypeId;

import helios.core.TypeIndexer;
import helios.core.types;

export namespace helios::ecs::common::types {

    /**
     * @brief Unique type identifier for context types, scoped per domain.
     */
    template<typename TPhase>
    class ContextTypeId {

        /**
         * @brief The underlying ID value.
         */
        size_t id_{0};

    public:

        /**
         * @brief Constructs a ContextTypeId with a specific ID value.
         *
         * @param id The unique identifier assigned to the component type.
         * This ID must be a valid non-negative value.
         */
        explicit ContextTypeId(const size_t id) : id_(id) {}

        /**
         * @brief Constructs an uninitialized ContextTypeId.
         *
         * @param no_init_t Tag to indicate no initialization.
         */
        explicit ContextTypeId(helios::core::types::no_init_t) {}

        /**
         * @brief Returns the underlying ID value.
         *
         * @return The numeric type ID, suitable for use as an array index.
         */
        [[nodiscard]] size_t value() const noexcept {
            return id_;
        }

        /**
         * @brief Gets the ContextTypeId for a specific component type.
         *
         * @tparam T The context type.
         *
         * @return The unique ContextTypeId for type T.
         */
        template <typename T>
        [[nodiscard]] static ContextTypeId id() {
            static const size_t tid = helios::core::TypeIndexer<TPhase>::template typeIndex<T>();
            return ContextTypeId(tid);
        }

        friend constexpr bool operator==(ContextTypeId, ContextTypeId) noexcept = default;
    };


}


/**
 * @brief std::hash specialization for ContextTypeId.
 *
 * @details Enables use of ContextTypeId as a key in unordered containers.
 */
template<typename TPhase>
struct std::hash<helios::ecs::common::types::ContextTypeId<TPhase>> {
   std::size_t operator()(const helios::ecs::common::types::ContextTypeId<TPhase>& id) const noexcept {
        return id.value();
    }

};