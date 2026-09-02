/**
 * @file EcsDataContainer.ixx
 * @brief EcsDataContainer for storing arbitrary data in a TypeMap.
 */
module;

#include <tuple>
#include <utility>
#include <variant>

export module helios.ecs.common.container:EcsDataContainer;

import helios.core.common.container;
import helios.core.common.traits;
import helios.ecs.command.concepts;
import helios.ecs.entity.Query;
import helios.ecs.entity.EntityManager;
import helios.ecs.entity.EntityAccessSet;
import helios.ecs.entity.concepts;

namespace helios::ecs::common::container::_detail {
struct EcsDataContainerTag {};

    template<typename T>
    struct IsTuple {
        static constexpr bool value = false;
    };

    template<typename ... T>
    struct IsTuple<std::tuple<T...>> {
        static constexpr bool value = true;
    };

} // namespace helios::ecs::common::container::__detail

export namespace helios::ecs::common::container {

using EcsDataContainer =
    core::common::container::TypeMap<helios::ecs::common::container::_detail::EcsDataContainerTag>;

template<typename TMutationSinkTuple>
struct EcsDataContainerArgumentResolver {

    template<typename TNeedle, typename... THaystack>
    using IsInList = helios::core::common::traits::IsInList<TNeedle, THaystack...>;

    EcsDataContainer& ecsDataContainer_;
    TMutationSinkTuple& mutationSinkTuple_;


    explicit EcsDataContainerArgumentResolver(
        EcsDataContainer& ecsDataContainer_, TMutationSinkTuple& mutationSinkTuple
    ) : ecsDataContainer_(ecsDataContainer_), mutationSinkTuple_(mutationSinkTuple) {}

    template <std::size_t TArgIdx, typename TArg, typename... TConcreteTypes>
    decltype(auto) resolve(TConcreteTypes&... concreteTypes) {

        using Type = std::remove_cvref_t<TArg>;
        using QualifiedType = std::remove_reference_t<TArg>;

        static_assert(!std::is_rvalue_reference_v<TArg>, "Function arguments must be lvalue references.");

        using ConcreteTypesList = IsInList<Type, std::remove_cvref_t<TConcreteTypes>...>;

        if constexpr (ConcreteTypesList::value) {
            constexpr auto index = ConcreteTypesList::index;
            auto tuple = std::tie(concreteTypes...);
            return std::get<index>(tuple);
        } else if constexpr(ecs::entity::concepts::IsQuery<Type>) {

            using EntityManager =  ecs::entity::EntityManager<
                std::tuple_element_t<0,
                typename ecs::entity::EntityAccessSet<typename Type::ReadSet, typename Type::WriteSet>::AccessHandles
            >>;

            using MutationSinkType = std::tuple_element_t<TArgIdx, TMutationSinkTuple>;

            if constexpr(Type::WriteSet::size > 0) {

                static_assert(!std::same_as<MutationSinkType, std::monostate>, "Query types require a mutation sink to be provided.");

                auto& mutationSink = std::get<TArgIdx>(mutationSinkTuple_);
                return Type{&ecsDataContainer_.get<EntityManager>(), &mutationSink};

            } else {
                return Type{&ecsDataContainer_.get<EntityManager>(), nullptr};
            }

        }  else if constexpr (std::is_const_v<QualifiedType>) {
            return static_cast<const Type&>(ecsDataContainer_.get<Type>());
        } else {
            return static_cast<Type&>(ecsDataContainer_.get<Type>());
        }

    }


};

struct EcsDataContainerFunctionInvoker {

private:
    template <auto TFunction, typename TMember, typename TMutationSinkTuple, std::size_t... Idx, typename... TConcreteTypes>
    static decltype(auto) invokeImpl(
        TMember& member,
        EcsDataContainer& ecsDataContainer,
        TMutationSinkTuple& mutationSinkTuple,
        std::index_sequence<Idx...> /*unused*/,
        TConcreteTypes&... concreteTypes
    ) {
        using FunctionSignature = core::common::traits::FunctionSignatureTraits<decltype(TFunction)>;
        EcsDataContainerArgumentResolver resolver{ecsDataContainer, mutationSinkTuple};
        return std::invoke(
            TFunction,
            member,
            resolver.template resolve<
                Idx, typename FunctionSignature::template ArgumentType<Idx>, TConcreteTypes...
            >(concreteTypes...)...
        );
    }

    template <auto TFunction, typename TMember, std::size_t... Idx, typename... TConcreteTypes>
    static decltype(auto) invokeImpl(
        TMember& member,
        EcsDataContainer& ecsDataContainer,
        std::index_sequence<Idx...> /*unused*/,
        TConcreteTypes&... concreteTypes
    ) {
        using FunctionSignature = core::common::traits::FunctionSignatureTraits<decltype(TFunction)>;
        std::tuple<> emptyTuple{};
        EcsDataContainerArgumentResolver resolver{ecsDataContainer, emptyTuple};
        return std::invoke(
            TFunction,
            member,
            resolver.template resolve<
                Idx, typename FunctionSignature::template ArgumentType<Idx>, TConcreteTypes...
            >(concreteTypes...)...
        );
    }

public:
    template <auto TFunction, typename TMember, typename TMutationSinkTuple, typename... TConcreteTypes>
    requires helios::ecs::common::container::_detail::IsTuple<std::remove_cvref_t<TMutationSinkTuple>>::value
    static decltype(auto) invoke(
        TMember& member, EcsDataContainer& ecsDataContainer, TMutationSinkTuple& mutationSinkTuple, TConcreteTypes&... concreteTypes
    ) {

        using FunctionSignature = core::common::traits::FunctionSignatureTraits<decltype(TFunction)>;

        auto indexSequence = std::make_index_sequence<FunctionSignature::NumArgs>{};

        return invokeImpl<TFunction>(member, ecsDataContainer, mutationSinkTuple, indexSequence, concreteTypes...);
    }

    template <auto TFunction, typename TMember, typename... TConcreteTypes>
    static decltype(auto) invoke(
        TMember& member, EcsDataContainer& ecsDataContainer, TConcreteTypes&... concreteTypes
    ) {
        std::tuple<> emptyTuple{};
        return invoke<TFunction>(member, ecsDataContainer, emptyTuple, concreteTypes...);
    }
};

} // namespace helios::ecs::common::container
