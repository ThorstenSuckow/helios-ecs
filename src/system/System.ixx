/**
 * @file System.ixx
 * @brief Type-erased system wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <optional>
#include <variant>
#include <mach/mach_voucher_types.h>

export module helios.ecs.system.System;

import helios.core.common.traits;
import helios.core.common.concepts;
import helios.core.common.types;

import helios.ecs.entity.traits;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.EntityMutationCommandSink;
import helios.ecs.command.EntityMutationCommandBuffer;

import helios.ecs.command.NullCommandBuffer;
import helios.ecs.command.concepts;
import helios.ecs.command.traits;

import helios.ecs.common;
import helios.ecs.system.concepts;
import helios.ecs.system.types;

export namespace helios::ecs::system {

/**
 * @brief Type-erased wrapper for game logic processors.
 */
class System {

    using CommandBuffer = ecs::command::CommandBuffer;
    using NullCommandBuffer = ecs::command::NullCommandBuffer;
    using EcsDataContainer = ecs::common::container::EcsDataContainer;

    template<typename TMutationSinkTuple>
    using EcsDataContainerArgumentResolver = ecs::common::container::EcsDataContainerArgumentResolver<TMutationSinkTuple>;

    template <typename THandle, typename ... TWriteComponents>
    using EntityMutationCommandSink = ecs::command::EntityMutationCommandSink<THandle, TWriteComponents...>;

    template <typename THandle>
    using EntityMutationCommandBuffer = ecs::command::EntityMutationCommandBuffer<THandle>;

private:
    /**
     * @brief Internal virtual interface for type erasure.
     */
    class Concept {
    public:
        virtual ~Concept() = default;
        virtual bool update(EcsDataContainer& ecsDataContainer) noexcept = 0;

        [[nodiscard]] virtual CommandBuffer* commandBuffer() noexcept = 0;

        [[nodiscard]] virtual void* underlying() noexcept = 0;
        [[nodiscard]] virtual const void* underlying() const noexcept = 0;

        virtual bool flush(EcsDataContainer& ecsDataContainer) noexcept = 0;
    };

    template <typename TConcreteSystem>
    class Model final : public Concept {

        TConcreteSystem system_;

        static consteval auto updateFunction() {
            if constexpr (ecs::system::concepts::IsCallableSystem<TConcreteSystem>) {
                return &TConcreteSystem::operator();
            } else {
                return &TConcreteSystem::update;
            }
        }

        using UpdateFunction = decltype(updateFunction());
        using UpdateFunctionTraits = core::common::traits::FunctionSignatureTraits<UpdateFunction>;

        using CommandBufferInfo = ecs::command::traits::CommandBufferFromArguments<typename UpdateFunctionTraits::ArgumentTypes>;
        static_assert(
            CommandBufferInfo::Count <= 1, "System update function must have at most one command buffer argument."
        );
        using ConcreteCommandBufferType = CommandBufferInfo::Type;

        using QueryInfo = ecs::entity::traits::QueryFromArguments<typename UpdateFunctionTraits::ArgumentTypes>;
        using ConcreteQueryTypes = QueryInfo::list;
        using CommandSinkTypes = core::common::traits::ListToTuple<typename command::traits::EntityMutationCommandSinksFromQueries<ConcreteQueryTypes>::list>::tuple;
        CommandSinkTypes entityMutationSinkTuple_{};

        using EntityMutationCommandBufferTypes = command::traits::EntityMutationCommandBuffersFromHandles<typename QueryInfo::handles>::tuple ;
        EntityMutationCommandBufferTypes entityMutationCommandBufferTuple_{};

        template<std::size_t TIdx>
        auto& entityMutationCommandBuffer() {
            return std::get<TIdx>(entityMutationCommandBufferTuple_);
        }

        template<std::size_t TIdx>
        using UpdateFuncArgType = typename UpdateFunctionTraits::template ArgumentType<TIdx>;




        template <std::size_t... Idx>
        auto invokeUpdate(
            EcsDataContainer& ecsDataContainer,
            ConcreteCommandBufferType& concreteCommandBuffer,
            std::index_sequence<Idx...>
         /*unused*/) {

            auto resolver =  EcsDataContainerArgumentResolver(ecsDataContainer, entityMutationSinkTuple_);


            if constexpr (ecs::system::concepts::IsCallableSystem<TConcreteSystem>) {
                return std::invoke(
                    system_,
                    resolver.template resolve<Idx, UpdateFuncArgType<Idx>>(concreteCommandBuffer)...
                );
            } else {
                return system_.update(
                    resolver.template resolve<Idx, UpdateFuncArgType<Idx>>(concreteCommandBuffer)...
                );
            }
        }

        using ProducedFrameResultType = UpdateFunctionTraits::ReturnType;

        using StoredFrameResultType =
            std::conditional_t<std::is_void_v<ProducedFrameResultType>, std::monostate, ProducedFrameResultType>;

        std::optional<StoredFrameResultType> frameResult_;

        CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};

        static constexpr bool hasCommandBuffer() noexcept {
            return !std::is_same_v<ConcreteCommandBufferType, command::NullCommandBuffer>;
        }

        void updateAndStore(EcsDataContainer& ecsDataContainer) {
            if constexpr (std::is_void_v<ProducedFrameResultType>) {
                invokeUpdate(
                    ecsDataContainer,
                    *commandBuffer_.tryGet<ConcreteCommandBufferType>(),
                    std::make_index_sequence<UpdateFunctionTraits::NumArgs>{}
                );
            } else {
                frameResult_.emplace(invokeUpdate(
                    ecsDataContainer,
                    *commandBuffer_.tryGet<ConcreteCommandBufferType>(),
                    std::make_index_sequence<UpdateFunctionTraits::NumArgs>{}
                ));
            }
        }

    public:
        explicit Model(TConcreteSystem&& sys) : system_(std::move(sys)) {}

        bool update(EcsDataContainer& ecsDataContainer) noexcept override {
            updateAndStore(ecsDataContainer);
            return true;
        }

        [[nodiscard]] CommandBuffer* commandBuffer() noexcept override {
            if constexpr (hasCommandBuffer()) {
                return nullptr;
            }

            return &commandBuffer_;
        }

        bool flush(EcsDataContainer& ecsDataContainer) noexcept override {

            if constexpr (!std::is_void_v<ProducedFrameResultType>) {
                ecsDataContainer.emplace<ProducedFrameResultType>(std::move(*frameResult_));
                frameResult_.reset();
            }

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
                commandBuffer_.flush(ecsDataContainer);
            }

            return true;

        }

        void* underlying() noexcept override {
            return &system_;
        }

        [[nodiscard]] const void* underlying() const noexcept override {
            return &system_;
        }
    };

    std::unique_ptr<Concept> pimpl_;

    explicit System(std::unique_ptr<Concept>&& pimpl) : pimpl_(std::move(pimpl)) {}

public:
    template <typename TConcreteSystem>
    explicit System(TConcreteSystem&& system)
        : pimpl_(std::make_unique<Model<std::remove_cvref_t<TConcreteSystem>>>(std::forward<TConcreteSystem>(system))) {}

    System() = delete;
    System(const System&) = delete;
    System& operator=(const System&) = delete;

    System& operator=(System&&) = default;
    System(System&&) noexcept = default;

    /**
     * @brief Delegates to the wrapped system's `update()` method.
     *
     * @param updateContext The current frame's update context.
     *
     * @pre System must be initialized (pimpl_ != nullptr).
     */
    bool update(EcsDataContainer& ecsDataContainer) noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->update(ecsDataContainer);
    }

    bool flush(EcsDataContainer& ecsDataContainer) {
        assert(pimpl_ && "System not initialized");
        return pimpl_->flush(ecsDataContainer);
    }

 /*   [[nodiscard]] CommandBuffer* commandBuffer() noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->commandBuffer();
    }*/

    /**
     * @brief Returns a type-erased pointer to the wrapped system instance.
     *
     * @return Pointer to the underlying concrete system.
     *
     * @pre System must be initialized (pimpl_ != nullptr).
     */
    [[nodiscard]] void* underlying() noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->underlying();
    }

    /**
     * @copydoc underlying()
     */
    [[nodiscard]] const void* underlying() const noexcept {
        assert(pimpl_ && "System not initialized");
        return pimpl_->underlying();
    }
};

} // namespace helios::ecs::system