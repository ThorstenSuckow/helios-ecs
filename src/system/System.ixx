/**
 * @file System.ixx
 * @brief Type-erased system wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>
#include <optional>

export module helios.ecs.system.System;

import helios.ecs.command.CommandBuffer;
import helios.ecs.command.concepts;

import helios.ecs.common.types;
import helios.ecs.system.concepts;

export namespace helios::ecs::system {


    /**
     * @brief Type-erased wrapper for game logic processors.
     */
    class System {

        using ContextRef = ecs::common::types::ContextRef;
        using ContextTypeId = ecs::common::types::ContextTypeId;
        using NullInitContext = ecs::common::types::NullInitContext;
        using NullFlushContext = ecs::common::types::NullFlushContext;
        using CommandBuffer = ecs::command::CommandBuffer;

    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:

            virtual ~Concept() = default;
            virtual bool update(ContextRef& contextRef) noexcept = 0;
            virtual bool flush(ContextRef& contextRef) noexcept = 0;
            virtual bool init(ContextRef& contextRef) noexcept = 0;

            [[nodiscard]] virtual ContextTypeId expectedInitContextTypeId() const noexcept = 0;
            [[nodiscard]] virtual ContextTypeId expectedFlushContextTypeId() const noexcept = 0;
            [[nodiscard]] virtual ContextTypeId expectedUpdateContextTypeId() const noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };

        /**
         * @brief Typed wrapper that adapts a concrete system to the Concept interface.
         *
         * @tparam T The concrete system type, must satisfy `IsSystemLike<T>`.
         */
        template<typename TConcreteSystem>
        class Model final : public Concept {

            TConcreteSystem system_;

            using UpdateContextType = TConcreteSystem::UpdateContextType;

        public:

            explicit Model(TConcreteSystem sys, void* buffer = nullptr)
                    : system_(std::move(sys)) {}

            bool update(ContextRef& contextRef) noexcept override {
                if (auto* ctx = contextRef.tryGet<UpdateContextType>()) {
                    return system_.update(*ctx);
                }

                return false;
            }

            bool init(ContextRef& contextRef) noexcept override {
                return true;
            }

            bool flush(ContextRef& ctx) noexcept override {
                return true;
            }

            [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept override {
                return ContextTypeId::id<NullInitContext>();
            }

            [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept override {
                return ContextTypeId::id<NullFlushContext>();
            }

            [[nodiscard]] ContextTypeId expectedUpdateContextTypeId() const noexcept override {
                return ContextTypeId::id<UpdateContextType>();
            }

            void* underlying() noexcept override {
                return &system_;
            }

            const void* underlying() const noexcept override {
                return &system_;
            }
        };

        template<typename TConcreteSystem, typename TCommandBuffer>
        class ModelWithOwnedCommandBuffer final : public Concept {

            using InitContextType = TCommandBuffer::InitContextType;
            using FlushContextType = TCommandBuffer::FlushContextType;
            using UpdateContextType = TConcreteSystem::UpdateContextType;

            TConcreteSystem system_;

            /**
             * @brief Wrapped CommandBuffer to make sure init/flush can be called with ContextRef.
             */
            CommandBuffer commandBuffer_;
        public:

            explicit ModelWithOwnedCommandBuffer(TConcreteSystem sys, TCommandBuffer cb) :
            system_(std::move(sys)),
            commandBuffer_(CommandBuffer(std::move(cb))) {}

            bool update(ContextRef& contextRef) noexcept override {
                if (auto* ctx = contextRef.tryGet<UpdateContextType>()) {
                    return system_.update(*ctx, *static_cast<TCommandBuffer*>(commandBuffer_.underlying()));
                }
                return false;
            }

            bool init(ContextRef& contextRef) noexcept override {
                return commandBuffer_.init(contextRef);
            }

            bool flush(ContextRef& contextRef) noexcept override {
                return commandBuffer_.flush(contextRef);
            }

            [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept override {
                return ContextTypeId::template id<InitContextType>();
            }

            [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept override {
                return ContextTypeId::template id<FlushContextType>();
            }

            [[nodiscard]] ContextTypeId expectedUpdateContextTypeId() const noexcept override {
                return ContextTypeId::template id<UpdateContextType>();
            }

            void* underlying() noexcept override {
                return &system_;
            }

            const void* underlying() const noexcept override {
                return &system_;
            }
        };

        std::unique_ptr<Concept> pimpl_;

    public:

        /**
         * @brief Default constructor creating an empty System.
         */
        System() = default;

        /**
         * @brief Wraps a concrete system in a type-erased System.
         *
         * @tparam TConcreteSystem The concrete system type, must satisfy `IsRuntimeSystemLike<TConcreteSystem>`.
         *
         * @param system The concrete system instance to wrap (moved into internal storage).
         */
        template<typename TConcreteSystem>
        requires concepts::IsRuntimeSystemLike<std::remove_cvref_t<TConcreteSystem>>
        explicit System(TConcreteSystem&& system)
             : pimpl_(std::make_unique<Model<std::remove_cvref_t<TConcreteSystem>>>(std::forward<TConcreteSystem>(system))){}

        /**
         * @brief Wraps a concrete system in a type-erased System that also owns a CommandBuffer.
         *
         * @tparam TConcreteSystem The concrete system type, must satisfy `IsRuntimeSystemLike<TConcreteSystem>`.
         * @tparam TCommandBuffer The concrete CommandBuffer-type, must satisfy `IsCommandBufferLike<TCommandBuffer>`.
         *
         * @param system The concrete system instance to wrap (moved into internal storage).
         * @param commandBuffer The concrete command buffer instance to wrap (moved into internal storage).
         */
        template<typename TConcreteSystem, typename TCommandBuffer>
        requires concepts::IsRuntimeSystemLike<std::remove_cvref_t<TConcreteSystem>>
            && ecs::command::concepts::IsCommandBufferLike<std::remove_cvref_t<TCommandBuffer>>
            && (!std::is_lvalue_reference_v<TCommandBuffer>)
        explicit System(TConcreteSystem&& system, TCommandBuffer&& commandBuffer)
            : pimpl_(
                std::make_unique<
                    ModelWithOwnedCommandBuffer<
                        std::remove_cvref_t<TConcreteSystem>,
                        std::remove_cvref_t<TCommandBuffer>
                    >
                >(
                    std::forward<TConcreteSystem>(system),
                    std::forward<TCommandBuffer>(commandBuffer)
                ))
        {}

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
        bool update(ContextRef& contextRef) noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->update(contextRef);
        }

        /**
         * @brief Flushes any outstanding changes, e.g. commands of the associated buffer.
         *
         * @param updateContext The current frame's update context.
         */
        bool flush(ContextRef& contextRef) noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->flush(contextRef);
        }

        /**
         * @brief Inits this system.
         *
         * @param ContextRef The context ref to use for initialization.
         */
        bool init(ContextRef& contextRef) noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->init(contextRef);
        }

        [[nodiscard]] ContextTypeId expectedInitContextTypeId() const noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->expectedInitContextTypeId();
        }

        [[nodiscard]] ContextTypeId expectedFlushContextTypeId() const noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->expectedFlushContextTypeId();
        }

        [[nodiscard]] ContextTypeId expectedUpdateContextTypeId() const noexcept {
            assert(pimpl_ && "System not initialized");
            return pimpl_->expectedUpdateContextTypeId();
        }

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


}