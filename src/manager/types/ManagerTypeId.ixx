/**
 * @file ManagerTypeId.ixx
 * @brief Unique type identifier for engine resources.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.manager.types:ManagerTypeId;

import helios.core.TypeIndexer;
import helios.core.types;

export namespace helios::ecs::manager::types {

    /**
     * @brief Unique type identifier for engine resources.
     */
    template<typename TExecutionContext, typename TInitContext>
    class ManagerTypeId {

        /**
         * @brief Tag type for the TypeIndexer domain.
         */
        struct helios_engine_common_tag_ResourceTypes{};

        using ResourceType = helios_engine_common_tag_ResourceTypes;

        /**
         * @brief The underlying ID value.
         */
        size_t id_{0};


    public:


        /**
         * @brief Constructs a ManagerTypeId with a specific value.
         *
         * @param id The type ID value.
         */
        explicit ManagerTypeId(const size_t id) : id_(id) {}


        /**
         * @brief Constructs an uninitialized ManagerTypeId.
         *
         * @param no_init_t Tag to indicate no initialization.
         */
        explicit ManagerTypeId(helios::core::types::no_init_t) {}


        /**
         * @brief Returns the underlying ID value.
         *
         * @return The numeric type ID, suitable for use as an array index.
         */
        [[nodiscard]] size_t value() const noexcept {
            return id_;
        }


        /**
         * @brief Returns the ManagerTypeId for a specific type.
         *
         * @tparam T The resource type.
         *
         * @return The unique ManagerTypeId for type T.
         */
        template <typename T>
        [[nodiscard]] static ManagerTypeId id() {
            static const size_t tid = helios::core::TypeIndexer<ResourceType>::template typeIndex<T>();
            return ManagerTypeId(tid);
        }

        friend constexpr bool operator==(ManagerTypeId, ManagerTypeId) noexcept = default;
    };


}


/**
 * @brief Hash specialization for ManagerTypeId.
 */
template<typename TExecutionContext, typename TInitContext>
struct std::hash<helios::ecs::manager::types::ManagerTypeId<TExecutionContext, TInitContext>> {
   std::size_t operator()(const helios::ecs::manager::types::ManagerTypeId<TExecutionContext, TInitContext>& id) const noexcept {
        return id.value();
    }

};