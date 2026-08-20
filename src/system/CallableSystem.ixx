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
import helios.ecs.command.types;
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
    template <typename THandle, typename TFunc, typename ... TCommands>
    class CallableSystem<THandle, TFunc, TCommands...> {

        TFunc lambda_;

    public:

        using Handle_type = THandle;

        using EcsRoleTag = ecs::system::tags::CallableSystemRole;

        using CommandTypes = ecs::command::types::CommandTypeList<TCommands...>;


        /**
         * @brief Creates the system from an update callback.
         * @param lambda Callback executed in `update`.
         */
        explicit CallableSystem(TFunc lambda) : lambda_(std::move(lambda)) {}

        /**
         * @brief Executes the stored callback for the current update step.
         * @param updateContext Runtime context of the engine update.
         */
        template<typename TUpdateContext>
        bool update(TUpdateContext& updateContext) noexcept {
            return std::invoke(lambda_, updateContext);
        }

        template<typename TUpdateContext, typename TCommandBuffer>
        bool update(TUpdateContext& updateContext, TCommandBuffer& commandBuffer) noexcept {
            return std::invoke(lambda_, updateContext, commandBuffer);
        }
    };



    template<typename THandle, typename ... TCommands, typename TFunc>
    [[nodiscard]] auto Lambda(TFunc&& func) {
        using FuncType = std::remove_cvref_t<TFunc>;

        return CallableSystem<THandle, FuncType, TCommands...>(std::forward<TFunc>(func));
    };


}