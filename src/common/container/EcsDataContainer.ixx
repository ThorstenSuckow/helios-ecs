/**
 * @file EcsDataContainer.ixx
 * @brief EcsDataContainer for storing arbitrary data in a TypeMap.
 */
module;

#include <utility>

export module helios.ecs.common.container:EcsDataContainer;

import helios.core.common.container;

import helios.ecs.command.concepts;

namespace helios::ecs::common::container::__detail {
    struct EcsDataContainerTag{};
}

export namespace helios::ecs::common::container {


    using EcsDataContainer = core::common::container::TypeMap<helios::ecs::common::container::__detail::EcsDataContainerTag>;

    struct EcsDataContainerArgumentResolver {
        /**
         * @brief Used to resolve function arguments.
         *
         * @details Makes sure that the specified commandBuffer is returned iff the
         * resolver recognizes a CommandBufferLike argument.
         *
         * @tparam TArg
         * @tparam TConcreteCommandBuffer
         * @param ecsDataContainer
         * @param commandBuffer
         * @return
         */
        template<typename TArg, typename TConcreteCommandBuffer>
        static auto& resolve(container::EcsDataContainer& ecsDataContainer, TConcreteCommandBuffer& commandBuffer) {

            using Type = std::remove_cvref_t<TArg>;
            using QualifiedType = std::remove_reference_t<std::remove_reference_t<TArg>>;

            static_assert(std::is_lvalue_reference_v<TArg>, "Function arguments must be lvalue references.");

            if constexpr(command::concepts::IsCommandBufferLike<Type>) {
                static_assert(std::is_same_v<TConcreteCommandBuffer, std::remove_cvref_t<Type>>,
                    "Function arguments must be CommandBufferLike types.");
                return commandBuffer;

            } else if constexpr(std::is_const_v<QualifiedType>) {
                return static_cast<const Type&>(ecsDataContainer.get<Type>());
            } else {
                return static_cast<Type&>(ecsDataContainer.get<Type>());
            }
        }

        template<typename TArg>
        static auto& resolve(container::EcsDataContainer& ecsDataContainer) {

            using Type = std::remove_cvref_t<TArg>;
            using QualifiedType = std::remove_reference_t<std::remove_reference_t<TArg>>;

            static_assert(std::is_lvalue_reference_v<TArg>, "Function arguments must be lvalue references.");

            if constexpr(std::is_const_v<QualifiedType>) {
                return static_cast<const Type&>(ecsDataContainer.get<Type>());
            } else {
                return static_cast<Type&>(ecsDataContainer.get<Type>());
            }
        }

    };

}
