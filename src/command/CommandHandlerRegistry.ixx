/**
 * @file CommandHandlerRegistry.ixx
 * @brief Registry for mapping command types to move-consuming handlers.
 */
module;

#include <cassert>
#include <concepts>
#include <type_traits>
#include <utility>
#include <vector>

export module helios.ecs.command.CommandHandlerRegistry;

import helios.ecs.command.types;
import helios.ecs.command.commands;
import helios.ecs.manager.types;

export namespace helios::ecs::command {

/**
 * @brief Maps command types to manager handlers.
 */
class CommandHandlerRegistry {

    struct CommandHandlerRef {
        void* manager{};
        bool (*submit)(void* manager, void* cmd){};
        [[nodiscard]] explicit operator bool() const noexcept {
            return manager != nullptr && submit != nullptr;
        }
    };

    using ManagerTypeId = manager::types::ManagerTypeId;

    std::vector<CommandHandlerRef> commandToHandlerRef_;

public:
    CommandHandlerRegistry() = default;

    CommandHandlerRegistry(const CommandHandlerRegistry&) = delete;
    CommandHandlerRegistry& operator=(const CommandHandlerRegistry&) = delete;
    CommandHandlerRegistry(CommandHandlerRegistry&&) = default;
    CommandHandlerRegistry& operator=(CommandHandlerRegistry&&) = default;

    /**
     * @brief Registers a handler for a concrete command type.
     * @tparam TCommandType Command type to route.
     * @tparam TManager Manager type that handles the command via submit(TCommandType&&).
     * @param owner Manager instance owning the handler.
     */
    template <typename TCommandType, typename TManager>
    void registerHandler(TManager& manager) {
        static_assert(requires(TManager& x, TCommandType&& c) {
            { x.submit(std::move(c)) } -> std::same_as<bool>;
        });

        const auto idx = types::CommandTypeId::id<TCommandType>().value();

        if (commandToHandlerRef_.size() <= idx) {
            commandToHandlerRef_.resize(idx + 1);
        }

        if (commandToHandlerRef_[idx]) {
#if HELIOS_DEBUG
            auto id = ManagerTypeId::id<TManager>();
            assert(
                commandToHandlerRef_[idx].manager == std::addressof(manager) &&
                "Handler already registered for this command type for a different owner"
            );
#endif
            return;
        }

        commandToHandlerRef_[idx] = CommandHandlerRef{
        std::addressof(manager),
        +[](void* managerPtr, void* cmdPtr) {
              auto& concreteManager = *static_cast<TManager*>(managerPtr);
              auto& concreteCmd = *static_cast<TCommandType*>(cmdPtr);
              return concreteManager.submit(std::move(concreteCmd));
          }};
    }



    /**
     * @brief Convenience helper to register multiple command types for one manager.
     * @tparam TCommandType Command types to register.
     * @tparam TManager Manager type handling all listed command types.
     * @param manager Manager instance owning the handlers.
     */
    template <typename... TCommandType, typename TManager>
    void handleCommands(TManager& manager) {
        (registerHandler<TCommandType>(manager), ...);
    }

    /**
     * @brief Returns whether a handler exists for the command.
     * @tparam TCommandType Command type to query.
     */
    template <typename TCommandType>
    [[nodiscard]] bool has() const noexcept {
        const auto idx = types::CommandTypeId::id<TCommandType>().value();
        if (idx < commandToHandlerRef_.size() && commandToHandlerRef_[idx]) {
            return true;
        }

        return false;
    }

    /**
     * @brief Resolves the manager that handles a command type.
     * @tparam TCommandType Command type to resolve.
     * @return Pointer to manager, or nullptr when no mapping exists.
     */
    template <typename TCommandType>
    [[nodiscard]] const CommandHandlerRef* tryHandler() const noexcept {
        const auto idx = types::CommandTypeId::id<TCommandType>().value();

        if (idx < commandToHandlerRef_.size()) {
            const auto& entry = commandToHandlerRef_[idx];
            if (entry) {
                return std::addressof(entry);
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
    template <typename TCommandType>
    bool submit(TCommandType&& cmd) const noexcept {
        using Cmd = std::remove_cvref_t<TCommandType>;

        if (auto* handler = tryHandler<Cmd>()) {
            return handler->submit(handler->manager, std::addressof(cmd));
        }

        return false;
    }

    /**
     * @brief Placeholder for batched command submission.
     * @tparam TCommandType Command value/reference type.
     * @param cmd Command or batch input to submit.
     */
    template <typename TCommandType>
    bool submitBatch(TCommandType&& cmd) const noexcept {
        assert(false && "not implemented");
    }
};

} // namespace helios::ecs::command