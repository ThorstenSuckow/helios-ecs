/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <variant>

export module helios.ecs.manager.Manager;

import helios.ecs.entity.traits;

import helios.core.common.traits;

import helios.ecs.common.types;
import helios.ecs.common.container;

import helios.ecs.manager.types;
import helios.ecs.manager.concepts;

import helios.ecs.command.EntityMutationCommandSink;
import helios.ecs.command.EntityMutationCommandBuffer;

import helios.ecs.command.concepts;
import helios.ecs.command.traits;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.NullCommandBuffer;

export namespace helios::ecs::manager {

/**
 * @brief Type-erased wrapper for game world managers.
 */
class Manager {

    using CommandBuffer = ecs::command::CommandBuffer;
    using NullCommandBuffer = ecs::command::NullCommandBuffer;
    using EcsDataContainer = ecs::common::container::EcsDataContainer;

    template <typename THandle, typename ... TWriteComponents>
    using EntityMutationCommandSink = ecs::command::EntityMutationCommandSink<THandle, TWriteComponents...>;

    template <typename THandle>
    using EntityMutationCommandBuffer = ecs::command::EntityMutationCommandBuffer<THandle>;
    template<typename TMutationSink>
    using EcsDataContainerArgumentResolver = ecs::common::container::EcsDataContainerArgumentResolver<TMutationSink>;

    using EcsDataContainerFunctionInvoker = ecs::common::container::EcsDataContainerFunctionInvoker;

private:
    /**
     * @brief Internal virtual interface for type erasure.
     */
    class Concept {
    public:
        virtual ~Concept() = default;
        virtual bool executeCommands(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool init(EcsDataContainer& dataContainer) noexcept = 0;
        virtual bool flush(EcsDataContainer& dataContainer) noexcept = 0;

        virtual void reset() noexcept = 0;

        virtual command::CommandBuffer* commandBuffer() noexcept = 0;

        [[nodiscard]] virtual void* underlying() noexcept = 0;
        [[nodiscard]] virtual const void* underlying() const noexcept = 0;
    };

    /**
     * @brief Typed wrapper that adapts a concrete manager to the Concept interface.
     *
     * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
     */
    template <typename TConcreteManager>
    class Model final : public Concept {

        using ExecuteCommandFunction = decltype(&TConcreteManager::executeCommands);
        using ExecuteFunctionSignature = core::common::traits::FunctionSignatureTraits<ExecuteCommandFunction>;
        using CommandBufferInfo =
            ecs::command::traits::CommandBufferFromArguments<typename ExecuteFunctionSignature::ArgumentTypes>;
        static_assert(
            CommandBufferInfo::Count <= 1,
            "Manager executeCommands function must have at most one command buffer argument."
        );
        using ConcreteCommandBufferType = CommandBufferInfo::Type;

        using QueryInfo = ecs::entity::traits::QueryFromArguments<typename ExecuteFunctionSignature::ArgumentTypes>;
        using ConcreteQueryTypes = QueryInfo::list;
        using CommandSinkTypes = core::common::traits::ListToTuple<typename command::traits::EntityMutationCommandSinksFromQueries<ConcreteQueryTypes>::list>::tuple;
        CommandSinkTypes entityMutationSinkTuple_{};

        using EntityMutationCommandBufferTypes = command::traits::EntityMutationCommandBuffersFromHandles<typename QueryInfo::handles>::tuple ;
        EntityMutationCommandBufferTypes entityMutationCommandBufferTuple_{};

        template<std::size_t TIdx>
        auto& entityMutationCommandBuffer() {
            return std::get<TIdx>(entityMutationCommandBufferTuple_);
        }


        TConcreteManager manager_;
        CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};

        static bool constexpr hasCommandBuffer() noexcept {
            return !std::same_as<NullCommandBuffer, ConcreteCommandBufferType>;
        }

    public:
        explicit Model(TConcreteManager&& manager)
            : manager_(std::move(manager)), commandBuffer_(CommandBuffer{ConcreteCommandBufferType{}}) {}

        [[nodiscard]] command::CommandBuffer* commandBuffer() noexcept override {
            if (!hasCommandBuffer()) {
                return nullptr;
            }

            return &commandBuffer_;
        }

        bool flush(EcsDataContainer& ecsDataContainer) noexcept override {

            // each sink is associated with one command buffer to make sure mutations can be run in parallel later on
            constexpr std::size_t SinkCount = std::tuple_size_v<decltype(entityMutationSinkTuple_)>;
            ([&]<std::size_t... Idx>(std::index_sequence<Idx...>) {

                ([&]() {
                    auto& sink = std::get<Idx>(entityMutationSinkTuple_);
                    using SinkType = std::remove_cvref_t<decltype(sink)>;

                    if constexpr (!std::same_as<SinkType, std::monostate>) {
                        auto& mutationCommandBuffer = entityMutationCommandBuffer<Idx>();
                        sink.drain([&mutationCommandBuffer](auto&& cmd) {
                            using CmdType = std::remove_cvref_t<decltype(cmd)>;
                            mutationCommandBuffer.template add<CmdType>(std::move(cmd));
                        });
                        mutationCommandBuffer.flush(ecsDataContainer);
                    }
                }(), ...);

            }(std::make_index_sequence<SinkCount>{}));

            if constexpr (hasCommandBuffer()) {
                return commandBuffer_.flush(ecsDataContainer);
            }
            return true;
        }

        bool executeCommands(EcsDataContainer& ecsDataContainer) noexcept override {

            EcsDataContainerFunctionInvoker::invoke<&TConcreteManager::executeCommands>(
                manager_,
                ecsDataContainer,
                entityMutationSinkTuple_,
                ecsDataContainer,
                *static_cast<ConcreteCommandBufferType*>(commandBuffer_.underlying())
            );


            return true;
        }

        bool init(EcsDataContainer& ecsDataContainer) noexcept override {

            EcsDataContainerFunctionInvoker::invoke<&TConcreteManager::init>(
                manager_, ecsDataContainer,
                entityMutationSinkTuple_
            );

            return true;
        }
        void reset() noexcept override {
            manager_.reset();
        }

        void* underlying() noexcept override {
            return &manager_;
        }

        [[nodiscard]] const void* underlying() const noexcept override {
            return &manager_;
        }
    };

    std::unique_ptr<Concept> pimpl_;

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
    template <typename TConcreteManager>
        requires concepts::IsManagerLike<TConcreteManager>
    explicit Manager(TConcreteManager&& manager)
        : pimpl_(std::make_unique<Model<std::remove_cvref_t<TConcreteManager>>>(std::forward<TConcreteManager>(manager))) {}

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
    bool executeCommands(EcsDataContainer& dataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->executeCommands(dataContainer);
    }

    /**
     * @brief Delegates to the wrapped manager's `init()` method, if present.
     *
     * @param dataContainer
     *
     * @pre Manager must be initialized (pimpl_ != nullptr).
     */
    bool init(EcsDataContainer& dataContainer) noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->init(dataContainer);
    }

    /**
     * @brief Delegates to the wrapped manager's `reset()` method, if present.
     *
     * @pre Manager must be initialized (pimpl_ != nullptr).
     */
    void reset() noexcept {
        assert(pimpl_ && "Manager not initialized");
        (*pimpl_).reset();
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
   /* command::CommandBuffer* commandBuffer() noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->commandBuffer();
    }*/

     void flush(EcsDataContainer& ecsDataContainer) noexcept {
         pimpl_->flush(ecsDataContainer);
     }

    /**
     * @copydoc underlying()
     */
    [[nodiscard]] const void* underlying() const noexcept {
        assert(pimpl_ && "Manager not initialized");
        return pimpl_->underlying();
    }
};

} // namespace helios::ecs::manager
