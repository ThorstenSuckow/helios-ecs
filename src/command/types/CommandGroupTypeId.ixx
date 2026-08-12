/**
 * @file CommandGroupTypeId.ixx
 * @brief Unique type identifier for command group types.
 */
module;

#include <functional>
#include <cstddef>

export module helios.ecs.command.types:CommandGroupTypeId;

import helios.core.TypeIndexer;
import helios.core.types;

export namespace helios::ecs::command::types {

    /**
     * @brief Unique type identifier for command group types.
     */
    class CommandGroupTypeId {

        /**
         * @brief Tag type for the TypeIndexer domain.
         */
        struct helios_ecs_tag_CommandGroupTypes{};

        using ComponentType = helios_ecs_tag_CommandGroupTypes;

        /**
         * @brief The underlying ID value.
         */
        size_t id_{0};


    public:


        /**
         * @brief Constructs a CommandGroupTypeId with a specific value.
         *
         * @param id The type ID value.
         */
        explicit CommandGroupTypeId(const size_t id) : id_(id) {}


        /**
         * @brief Constructs an uninitialized CommandGroupTypeId.
         *
         * @param no_init_t Tag to indicate no initialization.
         */
        explicit CommandGroupTypeId(helios::core::types::no_init_t) {}


        /**
         * @brief Returns the underlying ID value.
         *
         * @return The numeric type ID.
         */
        [[nodiscard]] size_t value() const noexcept {
            return id_;
        }


        /**
         * @brief Returns the CommandGroupTypeId for a specific command type.
         *
         * @details Uses TypeIndexer to generate a unique ID per type.
         * The ID is generated once and cached.
         *
         * @tparam T The command type.
         *
         * @return The unique CommandGroupTypeId for type T.
         */
        template <typename T>
        [[nodiscard]] static CommandGroupTypeId id() {
            static const size_t tid = helios::core::TypeIndexer<ComponentType>::typeIndex<T>();
            return CommandGroupTypeId(tid);
        }

        friend constexpr bool operator==(CommandGroupTypeId, CommandGroupTypeId) noexcept = default;
    };


}


/**
 * @brief Hash specialization for CommandGroupTypeId.
 */
template<>
struct std::hash<helios::ecs::command::types::CommandGroupTypeId> {
   std::size_t operator()(const helios::ecs::command::types::CommandGroupTypeId& id) const noexcept {
        return id.value();
    }

};