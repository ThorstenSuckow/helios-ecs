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

export module helios.ecs.command.CommandHandlerRegistry;

import helios.ecs.command.types;

export namespace helios::ecs::command {

    /**
     * @brief Type-erased storage entry for a registered command handler.
     */
    struct CommandHandlerEntry {
        /**
         * @brief Pointer to the owning object (the handler instance).
         */
        void* owner = nullptr;

        /**
         * @brief Type-erased static trampoline function that casts owner and command to concrete types.
         */
        bool (*submitFn)(void*, void*) noexcept = nullptr;
    };

    /**
     * @brief Typed reference wrapper for invoking a registered handler.
     *
     * @tparam TCommandType The specific command type this reference handles.
     */
    template<typename TCommandType>
    struct CommandHandlerRef {

        /**
         * @brief Pointer to the handler instance.
         */
        void* owner = nullptr;

        /**
         * @brief Type-erased trampoline for command dispatch.
         */
        bool (*submitFn)(void*, void*) noexcept = nullptr;

        /**
         * @brief Checks if this reference points to a valid handler.
         *
         * @return True if both owner and submitFn are non-null.
         */
        [[nodiscard]] explicit operator bool() const noexcept {
            return owner && submitFn;
        }

        /**
         * @brief Submits (and consumes) a command via the referenced handler.
         *
         * @param cmd The command instance to forward.
         * @return True if the command was accepted/handled.
         */
        bool submit(TCommandType&& cmd) const noexcept {
            return submitFn(owner, std::addressof(cmd));
        }

    };

    /**
     * @brief Registry that maps TCommandType types to handler instances via function pointers.
     *
     * This avoids virtual inheritance (TypedCommandHandler) and allows any class with a matching
     * `submit` signature to act as a handler.
     *
     * Lookup is O(1) based on the CommandTypeId.
     */
    class CommandHandlerRegistry {

        /**
         * @brief Dense vector of handler entries, indexed by CommandTypeId value.
         */
        std::vector<CommandHandlerEntry> entries_;

        /**
         * @brief Dense vector of handler entries, indexed by CommandGroupTypeId value.
         */
        std::vector<CommandHandlerEntry> groupEntries_;

    public:

        /**
         * @brief Registers an object as the handler for a specific command type.
         *
         * @tparam TCommandType The command type to handle.
         * @tparam OwningT The concrete type of the handler object.
         *
         * @param owner Reference to the handler instance. Must outlive the registry (usually Owned by GameWorld/ResourceRegistry).
         *
         * @pre No handler is currently registered for this command type.
         *
         * @note this method is not threadsafe and should not be called concurrently.
         */
        template<typename TCommandType, typename OwningT>
        void registerHandler(OwningT& owner) {
            static_assert(requires(OwningT& x, TCommandType&& c) {
                { x.submit(std::move(c)) } -> std::same_as<bool>;
            });

            const auto idx = types::CommandTypeId::id<TCommandType>().value();

            if (entries_.size() <= idx) {
                entries_.resize(idx + 1);
            }

            if (entries_[idx].owner != nullptr) {
                #if HELIOS_DEBUG
                auto* ownerPtr = static_cast<void*>(std::addressof(owner));
                assert(entries_[idx].owner == ownerPtr && "Handler already registered for this command type for a different owner");
                #endif
                return;
            }

            entries_[idx] = CommandHandlerEntry{
                &owner,
                +[](void* owner, void* cmd) noexcept -> bool {
                    return static_cast<OwningT*>(owner)->submit(
                        std::move(*static_cast<TCommandType*>(cmd))
                    );
                }
            };
        }

        template<typename TCommandGroupType, typename OwningT>
        void registerHandlerForCommandGroup(OwningT& owner) {

            const auto idx = types::CommandGroupTypeId::id<TCommandGroupType>().value();

            if (groupEntries_.size() <= idx) {
                groupEntries_.resize(idx + 1);
            }

            if (groupEntries_[idx].owner != nullptr) {
                #if HELIOS_DEBUG
                auto* ownerPtr = static_cast<void*>(std::addressof(owner));
                assert(groupEntries_[idx].owner == ownerPtr && "Handler already registered for this command type for a different owner");
                #endif
                return;
            }

            groupEntries_[idx] = CommandHandlerEntry{
                &owner,
                +[](void* owner, void* cmd) noexcept -> bool {
                    return static_cast<OwningT*>(owner)->submit(
                        std::move(*static_cast<TCommandGroupType*>(cmd))
                    );
                }
            };
        }

        /**
         * @brief Registers one owner as handler for multiple command types.
         *
         * @tparam TCommandType Command types to register.
         * @tparam OwningT Concrete owner type implementing matching `submit(...)` overloads.
         * @param owner Handler owner instance.
         */
        template<typename... TCommandType, typename OwningT>
        void handleCommands(OwningT& owner) {
            (registerHandler<TCommandType>(owner), ...);
        }

        /**
         * @brief Checks if a handler is registered for the specified command type.
         *
         * @tparam TCommandType The command type to check.
         *
         * @return True if a valid handler exists.
         */
        template<typename TCommandType>
        [[nodiscard]] bool has() const noexcept {
            const auto idx = types::CommandTypeId::id<TCommandType>().value();
            if (idx < entries_.size()) {
                const auto& entry = entries_[idx];
                if (entry.owner && entry.submitFn) {
                    return true;
                }
            }
            if constexpr (TCommandType::Group_type) {
                const auto groupIdx = types::CommandGroupTypeId::id<typename TCommandType::Group_type>().value();
                if (idx >= groupEntries_.size()) {
                    return false;
                }
                const auto& groupEntry = groupEntries_[groupIdx];
                return groupEntry.owner && groupEntry.submitFn;
            }
            return false;
        }

        /**
         * @brief Retrieves a typed reference to the registered handler.
         *
         * @tparam TCommandType The command type.
         *
         * @return A CommandHandlerRef wrapper. Can be checked for validity via operator bool().
         */
        template<typename TCommandType>
        [[nodiscard]] CommandHandlerRef<TCommandType> tryHandler() const noexcept {
            const auto idx = types::CommandTypeId::id<TCommandType>().value();

            if (idx < entries_.size()) {
                const auto& entry = entries_[idx];
                if (entry.owner && entry.submitFn) {
                    return CommandHandlerRef<TCommandType>{ entry.owner, entry.submitFn };
                }
            }

            if constexpr (TCommandType::Group_type) {
                const auto groupIdx = types::CommandGroupTypeId::id<typename TCommandType::Group_type>().value();
                if (groupIdx >= groupEntries_.size()) {
                    return {};
                }
                const auto& groupEntry = groupEntries_[groupIdx];

                if (!groupEntry.owner || !groupEntry.submitFn) {
                    return {};
                }

                return CommandHandlerRef<TCommandType>{ groupEntry.owner, groupEntry.submitFn };
            }

            return {};
        }

        /**
         * @brief Directly submits and consumes a command via its registered handler.
         *
         * @tparam TCommandType The command type.
         *
         * @param cmd The command instance to forward.
         *
         * @return True if a handler was found and it returned true; false otherwise.
         */
        template<typename TCommandType>
        bool submit(TCommandType&& cmd) const noexcept {
            using Cmd = std::remove_cvref_t<TCommandType>;

            if (auto handler = tryHandler<Cmd>()) {
                return handler.submit(std::move(cmd));
            }

            return false;
        }


        template<typename TCommandType>
        bool submitBatch(TCommandType&& cmd) const noexcept {
            assert(false && "not implemented");
        }

    };

} // namespace