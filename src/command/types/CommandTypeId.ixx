/**
 * @file CommandTypeId.ixx
 * @brief Unique type identifier for command types.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.command.types:CommandTypeId;

import helios.core.TypeIndexer;
import helios.core.types;

export namespace helios::ecs::command::types {

    /**
     * @brief Unique type identifier for command types.
     */
    class CommandTypeId {

        /**
         * @brief Tag type for the TypeIndexer domain.
         */
        struct helios_ecs_tag_CommandTypes{};

        using ComponentType = helios_ecs_tag_CommandTypes;

        /**
         * @brief The underlying ID value.
         */
        size_t id_{0};


    public:


        /**
         * @brief Constructs a CommandTypeId with a specific value.
         *
         * @param id The type ID value.
         */
        explicit CommandTypeId(const size_t id) : id_(id) {}


        /**
         * @brief Constructs an uninitialized CommandTypeId.
         *
         * @param no_init_t Tag to indicate no initialization.
         */
        explicit CommandTypeId(helios::core::types::no_init_t) {}


        /**
         * @brief Returns the underlying ID value.
         *
         * @return The numeric type ID.
         */
        [[nodiscard]] size_t value() const noexcept {
            return id_;
        }


        /**
         * @brief Returns the CommandTypeId for a specific command type.
         *
         * @details Uses TypeIndexer to generate a unique ID per type.
         * The ID is generated once and cached.
         *
         * @tparam T The command type.
         *
         * @return The unique CommandTypeId for type T.
         */
        template <typename T>
        [[nodiscard]] static CommandTypeId id() {
            static const size_t tid = helios::core::TypeIndexer<ComponentType>::typeIndex<T>();
            return CommandTypeId(tid);
        }

        friend constexpr bool operator==(CommandTypeId, CommandTypeId) noexcept = default;
    };


}


/**
 * @brief Hash specialization for CommandTypeId.
 */
template<>
struct std::hash<helios::ecs::command::types::CommandTypeId> {
   std::size_t operator()(const helios::ecs::command::types::CommandTypeId& id) const noexcept {
        return id.value();
    }

};