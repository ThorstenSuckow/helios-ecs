/**
 * @file CommandBufferTypeId.ixx
 * @brief Unique type identifier for command buffers.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.command.types:CommandBufferTypeId;

import helios.core.TypeIndexer;
import helios.core.types;

export namespace helios::ecs::command::types {

    /**
     * @brief Unique type identifier for command buffer types.
     *
     */
    template<typename TFlushContext, typename TInitContext>
    class CommandBufferTypeId {

        /**
         * @brief Tag type for the TypeIndexer domain.
         */
        struct helios_ecs_tag_CommandBufferTypes{};

        using CommandBufferType = helios_ecs_tag_CommandBufferTypes;

        /**
         * @brief The underlying ID value.
         */
        size_t id_{0};


    public:


        /**
         * @brief Constructs a CommandBufferTypeId with a specific value.
         *
         * @param id The type ID value.
         */
        explicit CommandBufferTypeId(const size_t id) : id_(id) {}


        /**
         * @brief Constructs an uninitialized CommandBufferTypeId.
         *
         * @param no_init_t Tag to indicate no initialization.
         */
        explicit CommandBufferTypeId(helios::core::types::no_init_t) {}


        /**
         * @brief Returns the underlying ID value.
         *
         * @return The numeric type ID.
         */
        [[nodiscard]] size_t value() const noexcept {
            return id_;
        }


        /**
         * @brief Returns the CommandBufferTypeId for a specific command type.
         *
         * @tparam T The command type.
         *
         * @return The unique CommandBufferTypeId for type T.
         */
        template <typename T>
        [[nodiscard]] static CommandBufferTypeId id() {
            static const size_t tid = helios::core::TypeIndexer<CommandBufferType>::template typeIndex<T>();
            return CommandBufferTypeId(tid);
        }

        friend constexpr bool operator==(CommandBufferTypeId, CommandBufferTypeId) noexcept = default;
    };


}


/**
 * @brief Hash specialization for CommandBufferTypeId.
 */
template<typename TFlushContext, typename TInitContext>
struct std::hash<helios::ecs::command::types::CommandBufferTypeId<TFlushContext, TInitContext>> {
   std::size_t operator()(const helios::ecs::command::types::CommandBufferTypeId<TFlushContext, TInitContext>& id) const noexcept {
        return id.value();
    }

};