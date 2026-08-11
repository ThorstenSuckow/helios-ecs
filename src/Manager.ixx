/**
 * @file Manager.ixx
 * @brief Type-erased manager wrapper using the Concept/Model pattern.
 */
module;

#include <cassert>
#include <memory>

export module helios.ecs.Manager;

import helios.ecs.types;
import helios.ecs.concepts.IsManagerLike;
import helios.ecs.concepts.HasFlushParallel;

import helios.ecs.CommandHandlerRegistry;

export namespace helios::ecs {

    /**
     * @brief Concept detecting an optional `init(CommandHandlerRegistry&)` method on a manager.
     *
     * @tparam T The manager type to inspect.
     */
    template<typename T>
    concept HasInit = requires(T& t, CommandHandlerRegistry& commandHandlerRegistry) {
        {t.init(commandHandlerRegistry) } -> std::same_as<void>;
    };

    /**
     * @brief Concept detecting an optional `reset()` method on a manager.
     *
     * @tparam T The manager type to inspect.
     */
    template<typename T>
    concept HasReset = requires(T& t) {
            {t.reset() } -> std::same_as<void>;
    };


    /**
     * @brief Type-erased wrapper for game world managers.
     */
    class Manager {

        using UpdateContext = types::UpdateContext;

    private:
        /**
         * @brief Internal virtual interface for type erasure.
         */
        class Concept {
        public:
            virtual ~Concept() = default;
            virtual void flush(types::UpdateContext& updateContext) noexcept = 0;
            virtual void init(CommandHandlerRegistry& commandHandlerRegistry) noexcept = 0;
            virtual void reset() noexcept = 0;
            virtual void flushParallel(types::UpdateContext& updateContext) noexcept = 0;

            [[nodiscard]] virtual void* underlying() noexcept = 0;
            [[nodiscard]] virtual const void* underlying() const noexcept = 0;
        };

        /**
         * @brief Typed wrapper that adapts a concrete manager to the Concept interface.
         *
         * @tparam T The concrete manager type, must satisfy `IsManagerLike<T>`.
         */
        template<typename T>
        class Model final : public Concept {
            T manager_;

            public:

            explicit Model(T sys) :  manager_(std::move(sys)) {}

            void flush(types::UpdateContext& updateContext) noexcept override {
                manager_.flush(updateContext);
            }

            /**
             * @brief Delegates to the wrapped manager's `flushParallel()` method.
             *
             * Will fall back to flush() if flushParallel() is not implemented by the underlying
             * manager.
             *
             * @param updateContext The current frame's update context.
             *
             * @pre Manager must be initialized (pimpl_ != nullptr).
             */
            void flushParallel(types::UpdateContext& updateContext) noexcept override {
                if constexpr (concepts::HasFlushParallel<T>) {
                    manager_.flushParallel(updateContext);
                    return;
                }
                assert(false && "Manager does not support flushParallel");
                manager_.flush(updateContext);
            }

            void init(CommandHandlerRegistry& commandHandlerRegistry) noexcept override {
                if constexpr (HasInit<T>) {
                    manager_.init(commandHandlerRegistry);
                }
            }
            void reset() noexcept override {
                if constexpr (HasReset<T>) {
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
         * @tparam T The concrete manager type, must satisfy `IsManagerLike<T>`.
         *
         * @param manager The concrete manager instance to wrap (moved into internal storage).
         */
        template<typename T>
        requires concepts::IsManagerLike<T>
        explicit Manager(T manager) : pimpl_(std::make_unique<Model<T>>(std::move(manager))) {}

        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;

        Manager& operator=(Manager&&) = default;
        Manager(Manager&&) noexcept = default;


        /**
         * @brief Delegates to the wrapped manager's `flush()` method.
         *
         * @param updateContext The current frame's update context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        void flush(types::UpdateContext& updateContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->flush(updateContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `flushParallel()` method.
         *
         * @param updateContext The current frame's update context.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        void flushParallel(types::UpdateContext& updateContext) noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->flushParallel(updateContext);
        }

        /**
         * @brief Delegates to the wrapped manager's `init()` method, if present.
         *
         * @param commandHandlerRegistry Registry used for one-time handler registration.
         *
         * @pre Manager must be initialized (pimpl_ != nullptr).
         */
        void init(CommandHandlerRegistry& commandHandlerRegistry) noexcept {
            assert(pimpl_ && "Manager not initialized");
            pimpl_->init(commandHandlerRegistry);
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

