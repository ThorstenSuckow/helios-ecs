/**
 * @file InvocationContext.ixx
 * @brief Context object for entity mutation execution, query and commandbuffer discovery.
 */
module;

#include <cstddef>
#include <tuple>
#include <variant>
#include <cassert>

export module helios.ecs.common.InvocationContext;

import helios.core.common;

import helios.ecs.entity.query.NullQuery;
import helios.ecs.entity.EntityManager;


import helios.ecs.command.traits;
import helios.ecs.entity.query.traits;
import helios.ecs.entity.mutation.traits;
import helios.ecs.common.container;
import helios.ecs.command;
import helios.ecs.component;

export namespace helios::ecs::common {

    /**
     * @brief Context for ECS mutation command execution.
     *
     * @details InvocationContext collects various type information about queries, return types and
     * command buffers of a specific method. The information can be used to determine whether two systems
     * providing the same API can be run in parallel by testing their query read / write sets or the consumed / produced
     * result.
     */
    template <typename TFunction>
    struct InvocationContext {
        using EcsDataContainer = ecs::common::container::EcsDataContainer;

        using InvocationFunctionTraits = core::common::traits::FunctionSignatureTraits<TFunction>;

        using CommandBufferInfo = ecs::command::traits::CommandBufferFromArguments<typename InvocationFunctionTraits::ArgumentTypes>;
        static_assert(
            CommandBufferInfo::Count <= 1, "System update function must have at most one command buffer argument."
        );
        using ConcreteCommandBufferType = CommandBufferInfo::Type;



        using QueryInfo = ecs::entity::query::traits::QueryFromArguments<typename InvocationFunctionTraits::ArgumentTypes>;
        using ConcreteQueryTypes = QueryInfo::list;
        using EntityMutationBufferTypes = core::common::traits::ListToTuple<
            typename entity::mutation::traits::EntityMutationBufferFromQueries<ConcreteQueryTypes>::list
        >::tuple;

        template<std::size_t TIdx>
        using InvocationFunctionArgType = typename InvocationFunctionTraits::template ArgumentType<TIdx>;
    };

} // namespace helios::ecs::common