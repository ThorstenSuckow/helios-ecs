/**
 * @file HandleTypeId.ixx
 * @brief Runtime-assigned type identifier for context types.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.common.types:HandleTypeId;

import helios.core.TypeIndexer;
import helios.core.types;

export namespace helios::ecs::common::types {

    /**
     * @brief Unique type identifier for handle types.
     * 
     * This provides a handle type accross the handle universe. Domain-specific handle-types
     * should be used where identification in this specific domain is required.
     */
    class HandleTypeId {

        /**
         * @brief The underlying ID value.
         */
        size_t id_{0};

        struct HandleTypeIdDomain{};

    public:

        /**
         * @brief Constructs a HandleTypeId with a specific ID value.
         *
         * @param id The unique identifier assigned to the handle type.
         * This ID must be a valid non-negative value.
         */
        explicit HandleTypeId(const size_t id) : id_(id) {}

        /**
         * @brief Constructs an uninitialized HandleTypeId.
         *
         * @param no_init_t Tag to indicate no initialization.
         */
        explicit HandleTypeId(helios::core::types::no_init_t) {}

        /**
         * @brief Returns the underlying ID value.
         *
         * @return The numeric type ID, suitable for use as an array index.
         */
        [[nodiscard]] size_t value() const noexcept {
            return id_;
        }

        /**
         * @brief Gets the HandleTypeId for a specific handle type.
         *
         * @tparam T The handle type.
         *
         * @return The unique HandleTypeId for type T.
         */
        template <typename T>
        [[nodiscard]] static HandleTypeId id() {
            static const size_t tid = helios::core::TypeIndexer<HandleTypeIdDomain>::template typeIndex<T>();
            return HandleTypeId(tid);
        }

        friend constexpr bool operator==(HandleTypeId, HandleTypeId) noexcept = default;
    };


}


/**
 * @brief std::hash specialization for HandleTypeId.
 *
 * @details Enables use of HandleTypeId as a key in unordered containers.
 */
template<>
struct std::hash<helios::ecs::common::types::HandleTypeId> {
   std::size_t operator()(const helios::ecs::common::types::HandleTypeId& id) const noexcept {
        return id.value();
    }

};