/**
 * @file CommandHandlerRegistry.ixx
 * @brief Registry for mapping command types to move-consuming handlers.
 */
module;

#include <concepts>
#include <vector>
#include <cassert>
#include <type_traits>
#include <utility>
#include <memory>
#include <optional>

export module helios.ecs.command.CommandHandlerRegistry;

import helios.ecs.command.types;
import helios.ecs.manager.ManagerRegistry;
import helios.ecs.manager.Manager;
import helios.ecs.manager.types;

export namespace helios::ecs::command {



    /**
     * @brief Maps command and command-group types to manager handlers.
     */
    class CommandHandlerRegistry {

        using ManagerTypeId = manager::types::ManagerTypeId;
        using Manager = manager::Manager;
        using ManagerRegistry = manager::ManagerRegistry;

        std::vector<std::optional<ManagerTypeId>> commandToManager_;

        std::vector<std::optional<ManagerTypeId>> commandGroupToManager_;

        ManagerRegistry& registry_;

    public:

        /**
         * @brief Creates a registry bound to an existing manager registry.
         * @param managerRegistry Registry used to resolve manager instances by type id.
         */
        explicit CommandHandlerRegistry(ManagerRegistry& managerRegistry) : registry_(managerRegistry) {}


        /**
         * @brief Registers a handler for a concrete command type.
         * @tparam TCommandType Command type to route.
         * @tparam TManager Manager type that handles the command via submit(TCommandType&&).
         * @param owner Manager instance owning the handler.
         */
        template<typename TCommandType, typename TManager>
        void registerHandler(TManager& owner) {
            static_assert(requires(TManager& x, TCommandType&& c) {
                { x.submit(std::move(c)) } -> std::same_as<bool>;
            });

            const auto idx = types::CommandTypeId::id<TCommandType>().value();

            if (commandToManager_.size() <= idx) {
                commandToManager_.resize(idx + 1);
            }

            if (commandToManager_[idx]) {
                #if HELIOS_DEBUG
                auto id = ManagerTypeId::id<TManager>();
                assert(*commandToManager_[idx] == id && "Handler already registered for this command type for a different owner");
                #endif
                return;
            }

            commandToManager_[idx] = ManagerTypeId::id<TManager>();
        }

        /**
         * @brief Registers a handler for a full command group.
         * @tparam TCommandGroupType Command group tag type.
         * @tparam TManager Manager type handling commands in that group.
         * @param owner Manager instance owning the group handler.
         */
        template<typename TCommandGroupType, typename TManager>
        void registerHandlerForCommandGroup(TManager& owner) {

            const auto idx = types::CommandGroupTypeId::id<TCommandGroupType>().value();


            if (commandGroupToManager_.size() <= idx) {
                commandGroupToManager_.resize(idx + 1);
            }

            if (commandGroupToManager_[idx]) {
                #if HELIOS_DEBUG
                auto id = ManagerTypeId::id<TManager>();
                assert(*commandGroupToManager_[idx] == id && "Handler already registered for this command type for a different owner");
                #endif
                return;
            }

            commandGroupToManager_[idx] = ManagerTypeId::id<TManager>();
        }


        /**
         * @brief Convenience helper to register multiple command types for one manager.
         * @tparam TCommandType Command types to register.
         * @tparam TManager Manager type handling all listed command types.
         * @param owner Manager instance owning the handlers.
         */
        template<typename... TCommandType, typename TManager>
        void handleCommands(TManager& owner) {
            (registerHandler<TCommandType>(owner), ...);
        }


        /**
         * @brief Returns whether a handler exists for the command or its command group.
         * @tparam TCommandType Command type to query.
         */
        template<typename TCommandType>
        [[nodiscard]] bool has() const noexcept {
            const auto idx = types::CommandTypeId::id<TCommandType>().value();
            if (idx < commandToManager_.size()) {
                const auto& entry = commandToManager_[idx];
                if (entry) {
                    return true;
                }
            }
            if constexpr (TCommandType::CommandGroupType) {
                const auto groupIdx = types::CommandGroupTypeId::id<typename TCommandType::CommandGroupType>().value();
                if (groupIdx >= commandGroupToManager_.size()) {
                    return false;
                }
                const auto& groupEntry = commandGroupToManager_[groupIdx];
                if (groupEntry) {
                    return true;
                }
            }
            return false;
        }


        /**
         * @brief Resolves the manager that handles a command type.
         * @tparam TCommandType Command type to resolve.
         * @return Pointer to manager, or nullptr when no mapping exists.
         */
        template<typename TCommandType>
        [[nodiscard]] Manager* tryHandler() const noexcept {
            const auto idx = types::CommandTypeId::id<TCommandType>().value();

            if (idx < commandToManager_.size()) {
                const auto& entry = commandToManager_[idx];
                if (entry) {
                    return registry_.item(*commandToManager_[idx]);
                }
            }

            if constexpr (requires {typename TCommandType::CommandGroupType;}) {
                const auto groupIdx = types::CommandGroupTypeId::id<typename TCommandType::CommandGroupType>().value();
                if (groupIdx < commandGroupToManager_.size()) {
                    const auto& entry = commandGroupToManager_[groupIdx];
                    if (entry) {
                        return registry_.item(*commandGroupToManager_[idx]);
                    }
                }
            }

            return nullptr;
        }


        /**
         * @brief Submits a command to its resolved handler.
         * @tparam TCommandType Command value/reference type.
         * @param cmd Command instance to forward to the handler.
         * @return true when a handler exists and accepts the command.
         */
        template<typename TCommandType>
        bool submit(TCommandType&& cmd) const noexcept {
            using Cmd = std::remove_cvref_t<TCommandType>;

            if (auto handler = tryHandler<Cmd>()) {
                return handler.submit(std::move(cmd));
            }

            return false;
        }


        /**
         * @brief Placeholder for batched command submission.
         * @tparam TCommandType Command value/reference type.
         * @param cmd Command or batch input to submit.
         */
        template<typename TCommandType>
        bool submitBatch(TCommandType&& cmd) const noexcept {
            assert(false && "not implemented");
        }

    };

} // namespace