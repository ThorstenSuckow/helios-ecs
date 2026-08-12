/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>

export module helios.ecs.manager.Manager;

import helios.ecs.types;

import helios.ecs.manager.concepts;


export namespace helios::ecs::manager {

    /**
     * @brief Concept detecting an optional `init(CommandHandlerRegistry&)` method on a manager.
     *
     * @tparam TConcreteManager The manager type to inspect.
     */
    template<typename TConcreteManager>
    concept HasInit = requires {
        &TConcreteManager::init;
    };

    /**
     * @brief Concept detecting an optional `reset()` method on a manager.
     *
     * @tparam TConcreteManager The manager type to inspect.
     */
    template<typename TConcreteManager>
    concept HasReset = requires(TConcreteManager& t) {
            {t.reset() } -> std::same_as<void>;
    };


    /**
     * @brief Type-erased wrapper for game world managers.
     */
    template<typename TExecutionContext, typename TInitContext>
    class Manager {



    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual void executeCommands(TExecutionContext& executionContext) noexcept = 0;
            virtual void init(TInitContext& initContext) noexcept = 0;
            virtual void reset() noexcept = 0;
            virtual void executeCommandsParallel(TExecutionContext& executionContext) noexcept = 0;

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

            public:

            explicit Model(TConcreteManager sys) :  manager_(std::move(sys)) {}

            void executeCommands(TExecutionContext& executionContext) noexcept override {
                manager_.executeCommands(executionContext);
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
            void executeCommandsParallel(TExecutionContext& executionContext) noexcept override {
                if constexpr (concepts::HasExecuteParallel<TConcreteManager>) {
                    manager_.executeCommandsParallel(executionContext);
                    return;
                }
                assert(false && "Manager does not support executeCommandsParallel");
                manager_.executeCommands(executionContext);
            }

            void init(TInitContext& initContext) noexcept override {
                if constexpr (HasInit<TConcreteManager>) {
                    manager_.init(initContext);
                }
            }
            void reset() noexcept override {
                if constexpr (HasReset<TConcreteManager>) {
                    manager_.reset();
                }
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
        Manager() = default;

        /**
         * @brief Wraps a concrete manager in a type-erased Manager.
         *
         * @tparam TConcreteManager The concrete manager type, must satisfy `IsManagerLike<TConcreteManager>`.
         *
         * @param manager The concrete manager instance to wrap (moved into internal storage).
         */
        template<typename TConcreteManager>
        requires concepts::IsManagerLike<TConcreteManager>
        explicit Manager(TConcreteManager manager) : pimpl_(std::make_unique<Model<TConcreteManager>>(std::move(manager))) {}

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
        void executeCommands(TExecutionContext& executionContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->executeCommands(executionContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `executeCommandsParallel()` method.
         *
         * @param executionContext The current frame's update context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        void executeCommandsParallel(TExecutionContext& executionContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->executeParallel(executionContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `init()` method, if present.
         *
         * @param initContext The initialization context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        void init(TInitContext& initContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->init(initContext);
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

