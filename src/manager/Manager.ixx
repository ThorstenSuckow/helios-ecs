/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>

export module helios.ecs.manager.Manager;

import helios.ecs.common.types;

import helios.ecs.manager.types;
import helios.ecs.manager.concepts;


export namespace helios::ecs::manager {


    /**
     * @brief Type-erased wrapper for game world managers.
     */
    class Manager {

        using ExecutionContextRef = ecs::common::types::ContextRef<ecs::common::types::Execution>;
        using InitContextRef = ecs::common::types::ContextRef<ecs::common::types::Init>;


    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual bool executeCommands(ExecutionContextRef executionContext) noexcept = 0;
            virtual bool init(InitContextRef initContext) noexcept = 0;
            virtual void reset() noexcept = 0;
            virtual bool executeCommandsParallel(ExecutionContextRef executionContext) noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };

        /**
         * @brief Typed wrapper that adapts a concrete manager to the Concept interface.
         *
         * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
         */
        template<typename TConcreteManager>
        class Model final : public Concept {
            TConcreteManager manager_;

            using ExecutionContextType = typename TConcreteManager::ExecutionContextType;
            using InitContextType = typename TConcreteManager::InitContextType;

            public:

            explicit Model(TConcreteManager sys) :  manager_(std::move(sys)) {}

            bool executeCommands(const ExecutionContextRef executionContextRef) noexcept override {
                
                if (auto* ctx = executionContextRef.tryGet<ExecutionContextType>()) {
                    return manager_.executeCommands(*ctx);
                }

                return false;
            }

            /**
             * @brief Delegates to the wrapped manager's `executeCommandsParallel()` method.
             *
             * Will fall back to executeCommands() if executeCommandsParallel() is not implemented by the underlying
             * manager.
             *
             * @param executionContext The current frame's update context.
             *
             * @pre Manager must be initialized (pimpl_ != nullptr).
             */
            bool executeCommandsParallel(const ExecutionContextRef executionContext) noexcept override {
                
                if (auto* ctx = executionContext.tryGet<ExecutionContextType>()) {
                    if constexpr (concepts::HasExecuteCommandsParallel<TConcreteManager>) {
                        return manager_.executeCommandsParallel(*ctx);
                    } else {
                        assert(false && "Manager does not support executeCommandsParallel");
                        return manager_.executeCommands(*ctx);
                    }
                }

                return false;
            }

            bool init(InitContextRef initContext) noexcept override {
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
        template<typename TConcreteManager>
        requires concepts::IsManagerLike<TConcreteManager>
        explicit Manager(TConcreteManager manager)
            : pimpl_(std::make_unique<Model<TConcreteManager>>(std::move(manager))) {}

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
        bool executeCommands(const ExecutionContextRef executionContext) noexcept {
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
        bool executeCommandsParallel(const ExecutionContextRef executionContext) noexcept {
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
        bool init(InitContextRef initContext) noexcept {
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
         * @copydoc underlying()
         */
        [[nodiscard]] const void* underlying() const noexcept {
            assert(pimpl_ && "Manager not initialized");
            return pimpl_->underlying();
        }

    };


}

