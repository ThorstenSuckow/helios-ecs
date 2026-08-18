/**
 * @file CallableSystem.ixx
 * @brief Small ECS system wrapper around an update lambda.
 */
module;

#include <memory>
#include <functional>
#include <type_traits>
#include <utility>

export module helios.ecs.system.CallableSystem;

import helios.ecs.system.tags;
import helios.ecs.common.concepts;
import helios.ecs.command.concepts;
import helios.ecs.component;

export namespace helios::ecs::system {

    template <typename THandle, typename... TArgs>
    class CallableSystem;

    /**
     * @brief ECS system that executes a stored lambda on each update.
     *
     * @tparam THandle Handle type used by the surrounding system graph.
     * @tparam TFunc lambda function type
     */
    template <typename THandle, typename TUpdateContext, typename TFunc>
    class CallableSystem<THandle, TUpdateContext, TFunc> {

        TFunc lambda_;

    public:

        using Handle_type = THandle;

        using EcsRoleTag = ecs::system::tags::CallableSystemRole;

        using UpdateContextType = TUpdateContext;

        /**
         * @brief Creates the system from an update callback.
         * @param lambda Callback executed in `update`.
         */
        explicit CallableSystem(TFunc lambda) : lambda_(std::move(lambda)) {}


        /**
         * @brief Executes the stored callback for the current update step.
         * @param updateContext Runtime context of the engine update.
         */
        bool update(TUpdateContext& updateContext) noexcept {
            return std::invoke(lambda_, updateContext);
        }

    };

    /**
     * @brief ECS system that executes a stored lambda on each update.
     *
     * @tparam THandle Handle type used by the surrounding system graph.
     * @tparam TFunc lambda function type
     */
    template <typename THandle, typename TUpdateContext, typename TCommandBuffer, typename TFunc>
    requires ecs::command::concepts::IsCommandBufferLike<TCommandBuffer>
    class CallableSystem<THandle, TUpdateContext, TCommandBuffer, TFunc> {

        TFunc lambda_;

    public:

        using CommandBuffer_type = TCommandBuffer;

        using EcsRoleTag = ecs::system::tags::CallableSystemRole;

        using UpdateContextType = TUpdateContext;

        /**
         * @brief Creates the system from an update callback.
         * @param lambda Callback executed in `update`.
         */
        explicit CallableSystem(TFunc lambda) : lambda_(std::move(lambda)) {}


        /**
         * @brief Executes the stored callback for the current update step.
         * @param updateContext Runtime context of the engine update.
         * @param commandBuffer
         */
        bool update(TUpdateContext& updateContext, TCommandBuffer& commandBuffer) noexcept {
            return std::invoke(lambda_, updateContext, commandBuffer);
        }

    };

    /**
     * @brief Helper function for creating a CallableSystem with deduced function type.
     *
     * @tparam THandle Handle to distinguish the system's domain (e.g., entity handle type).
     * @tparam TFunc Callable type deduced from the function object.
     *
     * @param func Callable executed by the created lambda system.
     *
     * @return A CallableSystem specialized for the given handle/tag type and callable.
     */
    template<typename THandle, typename TUpdateContext, typename TFunc>
    [[nodiscard]] auto Lambda(TFunc&& func) {
        using FuncType = std::remove_cvref_t<TFunc>;

        return CallableSystem<THandle, TUpdateContext, FuncType>(std::forward<TFunc>(func));

    };

    /**
     * @brief Helper function for creating a CallableSystem with CommandBuffer and deduced function type.
     *
     * @tparam THandle Handle to distinguish the system's domain (e.g., entity handle type).
     * @tparam TCommandBuffer Command buffer type used for deferred command submission.
     * @tparam TFunc Callable type deduced from the function object.
     *
     * @param func Callable executed by the created lambda system.
     *
     * @return A CallableSystem specialized for the given handle/tag type and callable.
     */
    template<typename THandle, typename TUpdateContext, typename TCommandBuffer, typename TFunc>
    requires ecs::command::concepts::IsCommandBufferLike<TCommandBuffer>
    [[nodiscard]] auto Lambda(TFunc&& func)  {
        using FuncType = std::remove_cvref_t<TFunc>;

        return CallableSystem<THandle, TUpdateContext, TCommandBuffer, FuncType>(std::forward<TFunc>(func));

    };
}