/**
 * @file EcsDataContainer.ixx
 * @brief EcsDataContainer for storing arbitrary data in a TypeMap.
 */
module;

#include <utility>
#include <tuple>

export module helios.ecs.common.container:EcsDataContainer;

import helios.core.common.container;

import helios.ecs.command.concepts;

namespace helios::ecs::common::container::__detail {
    struct EcsDataContainerTag{};
}

export namespace helios::ecs::common::container {


    using EcsDataContainer = core::common::container::TypeMap<helios::ecs::common::container::__detail::EcsDataContainerTag>;

    struct EcsDataContainerArgumentResolver {



        template<typename TNeedle, typename ... THaystack>
        struct LookupList;

        template<typename TNeedle>
        struct LookupList<TNeedle> {
            constexpr static bool contains = false;
            constexpr static std::size_t index = 0;
        };

        template<typename TNeedle, typename First, typename ... Rest>
        struct LookupList<TNeedle, First, Rest...> {
            constexpr static bool contains = std::same_as<TNeedle, First>
                    || LookupList<TNeedle, Rest...>::contains;
            constexpr static std::size_t index = std::same_as<TNeedle, First>
                    ? 0 : 1 + LookupList<TNeedle, Rest...>::index;
        };

        /**
         * @brief Compares the specified current type against a list of pre-fabricated types and returns an item
         * of this list is the type is found, otherwise looks the type up in EcsDataContainer and returns a reference
         * to the stored instance.
         *
         * @tparam TArg
         * @tparam TConcreteTypes
         * @param ecsDataContainer
         * @param concreteTypes
         * @return
         */
        template<typename TArg, typename ... TConcreteTypes>
        static auto& resolve(EcsDataContainer& ecsDataContainer, TConcreteTypes& ... concreteTypes) {

            using Type = std::remove_cvref_t<TArg>;
            using QualifiedType = std::remove_reference_t<std::remove_reference_t<TArg>>;

            static_assert(std::is_lvalue_reference_v<TArg>, "Function arguments must be lvalue references.");

            using ConcreteTypesList = LookupList<Type, std::remove_cvref_t<TConcreteTypes>...>;

            if constexpr( ConcreteTypesList::contains) {
                constexpr auto index = ConcreteTypesList::index;
                auto tuple = std::tie(concreteTypes...);
                return std::get<index>(tuple);
            } else if constexpr(std::is_const_v<QualifiedType>) {
                return static_cast<const Type&>(ecsDataContainer.get<Type>());
            } else {
                return static_cast<Type&>(ecsDataContainer.get<Type>());
            }
        }


    };

}
