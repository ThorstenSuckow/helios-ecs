/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <exception>

export module helios.ecs.manager.Manager;

import helios.ecs.common.types;

import helios.ecs.manager.types;
import helios.ecs.manager.concepts;

import helios.ecs.command.concepts;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.NullCommandBuffer;

export namespace helios::ecs::manager {


    /**
     * @brief Type-erased wrapper for game world managers.
     */
    class Manager {

        using ContextRef = ecs::common::types::ContextRef;
        using ContextTypeId = ecs::common::types::ContextTypeId;
        using CommandBuffer = ecs::command::CommandBuffer;
        using NullCommandBuffer = ecs::command::NullCommandBuffer;

    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual bool executeCommands(ContextRef executionContext) noexcept = 0;
            virtual bool init(ContextRef initContext) noexcept = 0;
            virtual void reset() noexcept = 0;
            virtual bool executeCommandsParallel(ContextRef executionContext) noexcept = 0;

            virtual command::CommandBuffer* commandBuffer() noexcept = 0;

            [[nodiscard]] virtual ContextTypeId expectedInitContextTypeId() const noexcept = 0;
            [[nodiscard]] virtual ContextTypeId expectedExecutionContextTypeId() const noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };

        /**
         * @brief Typed wrapper that adapts a concrete manager to the Concept interface.
         *
         * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
         */
        template<typename TConcreteManager, typename TInitContext, typename TExecutionContext, typename TConcreteCommandBuffer>
        class Model final : public Concept {
            TConcreteManager manager_;

            using ExecutionContextType = TExecutionContext;
            using InitContextType = TInitContext;

            CommandBuffer commandBuffer_;

            public:

            explicit Model(TConcreteManager&& sys, CommandBuffer&& cmdBuffer)
            :  manager_(std::move(sys)), commandBuffer_(std::move(cmdBuffer)) {}

            [[nodiscard]] command::CommandBuffer* commandBuffer() noexcept override {
                if constexpr(std::same_as<NullCommandBuffer, TConcreteCommandBuffer>) {
                    return nullptr;
                }

                return &commandBuffer_;
            }

            [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept override {
                return ContextTypeId::template id<InitContextType>();
            }

            [[nodiscard]] ContextTypeId expectedExecutionContextTypeId() const noexcept override {
                return ContextTypeId::template id<ExecutionContextType>();
            }

            bool executeCommands(const ContextRef executionContextRef) noexcept override {

                if constexpr (!std::same_as<NullCommandBuffer, TConcreteCommandBuffer>) {
                    static_assert(
                        requires(TExecutionContext& ctx, TConcreteCommandBuffer& buffer)
                    {
                        {manager_.executeCommands(ctx, buffer)} -> std::same_as<bool>;

                    },
                    "TConcreteManager must have a member function `bool executeCommands(TExecutionContext&, TCommandBuffer&)`");

                    if (auto* ctx = executionContextRef.tryGet<ExecutionContextType>()) {
                        return manager_.executeCommands(
                            *ctx, *static_cast<TConcreteCommandBuffer*>(commandBuffer_.underlying()));
                    }
                    return false;
                } else {
                    static_assert(
                          requires(TExecutionContext& ctx)
                      {
                          {manager_.executeCommands(ctx)} -> std::same_as<bool>;

                      },
                      "TConcreteManager must have a member function `bool executeCommands(TExecutionContext&)`");

                    if (auto* ctx = executionContextRef.tryGet<ExecutionContextType>()) {
                        return manager_.executeCommands(*ctx);
                    }
                    return false;
                }

                return false;
            }

            bool executeCommandsParallel(const ContextRef executionContextRef) noexcept override {

                if constexpr (!concepts::HasExecuteCommandsParallel<TConcreteManager, TExecutionContext>) {
                    assert(false && "TConcreteManager must have a member function `bool executeCommandsParallel(TExecutionContext&)`");
                    std::terminate();
                    return false;
                } else {

                    if constexpr (!std::same_as<NullCommandBuffer, TConcreteCommandBuffer>) {
                        static_assert(
                            requires(TExecutionContext& ctx, TConcreteCommandBuffer& buffer)
                        {
                            {manager_.executeCommandsParallel(ctx, buffer)} -> std::same_as<bool>;

                        },
                        "TConcreteManager must have a member function `bool executeCommandsParallel(TExecutionContext&, TCommandBuffer&)`");

                        if (auto* ctx = executionContextRef.tryGet<ExecutionContextType>()) {
                            return manager_.executeCommandsParallel(
                                *ctx, *static_cast<TConcreteCommandBuffer*>(commandBuffer_.underlying()));
                        }
                        return false;
                    } else {
                        static_assert(
                              requires(TExecutionContext& ctx)
                          {
                              {manager_.executeCommandsParallel(ctx)} -> std::same_as<bool>;

                          },
                          "TConcreteManager must have a member function `bool executeCommands(TExecutionContext&)`");

                        if (auto* ctx = executionContextRef.tryGet<ExecutionContextType>()) {
                            return manager_.executeCommandsParallel(*ctx);
                        }
                        return false;
                    }
                    return false;
                }
            }

            bool init(ContextRef initContext) noexcept override {
                if (auto* ctx = initContext.tryGet<InitContextType>()) {
                    return manager_.init(*ctx);
                }

                return false;
            }
            void reset() noexcept override {
                manager_.reset();
            }

            void* underlying() noexcept override {
                return &manager_;
            }

            const void* underlying() const noexcept override {
                return &manager_;
            }
        };

        std::unique_ptr<Concept> pimpl_;

        explicit Manager(std::unique_ptr<Concept>&& pimpl)
            : pimpl_(std::move(pimpl))
        {}

    public:

        /**
         * @brief Default constructor creating an empty Manager.
         */
        Manager() = delete;

        /**
         * @brief Wraps a concrete manager in a type-erased Manager.
         *
         * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
         *
         * @param manager The concrete manager instance to wrap (moved into internal storage).
         */
        template<typename TConcreteManager, typename TInitContext, typename TExecutionContext, typename TCommandBufferFactory>
        requires concepts::IsManagerLike<TConcreteManager>
        static Manager make(TConcreteManager&& manager) {

            using ManagerType = std::remove_cvref_t<TConcreteManager>;

            if constexpr(requires { typename ManagerType::CommandTypes;}) {
                auto cmdBuffer = TCommandBufferFactory::make(typename ManagerType::CommandTypes{});
                using CommandBufferType = std::remove_cvref_t<decltype(cmdBuffer)>;
                auto erasedCmdBuffer = CommandBuffer::make<CommandBufferType, typename TCommandBufferFactory::FlushContextType>(
                    std::move(cmdBuffer));

                return Manager(
                std::make_unique<Model<
                    std::remove_cvref_t<TConcreteManager>,
                    TInitContext,
                    TExecutionContext,
                    CommandBufferType
                    >>(std::move(manager), std::move(erasedCmdBuffer))
            );
            } else {
                auto erasedCmdBuffer = CommandBuffer::make<NullCommandBuffer, typename TCommandBufferFactory::FlushContextType>(
                    std::move(NullCommandBuffer{}));
                return Manager(
                    std::make_unique<Model<
                        std::remove_cvref_t<TConcreteManager>,
                        TInitContext,
                        TExecutionContext,
                        NullCommandBuffer
                        >>(std::move(manager), std::move(erasedCmdBuffer))
                );
            }

        }

        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;

        Manager& operator=(Manager&&) = default;
        Manager(Manager&&) noexcept = default;


        /**
         * @brief Delegates to the wrapped manager's `executeCommands()` method.
         *
         * @param executionContext The current frame's update context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        bool executeCommands(const ContextRef executionContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->executeCommands(executionContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `executeCommandsParallel()` method.
         *
         * @param executionContext The current frame's update context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        bool executeCommandsParallel(const ContextRef executionContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->executeCommandsParallel(executionContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `init()` method, if present.
         *
         * @param initContext The initialization context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        bool init(ContextRef initContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->init(initContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `reset()` method, if present.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        void reset() noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->reset();
        }

        /**
         * @brief Returns a type-erased pointer to the wrapped manager instance.
         *
         * @return Pointer to the underlying concrete manager.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        [[nodiscard]] void* underlying() noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->underlying();
        }

        /**
         * @brief Provide CommandBuffer access in case managers write commands into owned command buffers.
         *
         * @details This is useful if systems executed the Manager and must make sure that generated commands are
         * being flushed..
         *
         * @return A span of pointers to the owned command buffers, or an empty span if the manager does not
         * own any command buffers.
         */
        command::CommandBuffer* commandBuffer() noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->commandBuffer();
        }

        /**
         * @copydoc underlying()
         */
        [[nodiscard]] const void* underlying() const noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->underlying();
        }

        /**
         * @brief Returns the expected TypeId of the InitContextType.
         * @return The expected InitContextType's TypeId.
         */
        [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->expectedInitContextTypeId();
        }

        /**
         * @brief Returns the expected TypeId of the ExecutionContextType.
         * @return The expected ExecutionContextType's TypeId.
         */
        [[nodiscard]] ContextTypeId expectedExecutionContextTypeId() const noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->expectedExecutionContextTypeId();
        }

    };


}

