/**
 * @file InvocationContext.ixx
 * @brief Context object for entity mutation execution, query and commandbuffer discovery.
 */
module;

#include <cstddef>
#include <tuple>
#include <variant>

export module helios.ecs.common.InvocationContext;

import helios.core.common;

import helios.ecs.entity.NullQuery;
import helios.ecs.entity.EntityMutationManager;
import helios.ecs.entity.EntityManager;


import helios.ecs.command.traits;
import helios.ecs.entity.traits;
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

        command::CommandBuffer commandBuffer_{ConcreteCommandBufferType{}};

        using QueryInfo = ecs::entity::traits::QueryFromArguments<typename InvocationFunctionTraits::ArgumentTypes>;
        using ConcreteQueryTypes = QueryInfo::list;
        using EntityMutationBufferTypes = core::common::traits::ListToTuple<
            typename entity::traits::EntityMutationBufferFromQueries<ConcreteQueryTypes>::list
        >::tuple;

        EntityMutationBufferTypes entityMutationBuffers_{};

        template<std::size_t TIdx>
        using InvocationFunctionArgType = typename InvocationFunctionTraits::template ArgumentType<TIdx>;

    public:


        [[nodiscard]] EntityMutationBufferTypes& entityMutationBuffers() noexcept {
            return entityMutationBuffers_;
        };


        [[nodiscard]] bool syncRequiredStructuralState(EcsDataContainer& ecsDataContainer) {

            auto walkQueries = [&]<typename ... TQueries>(core::common::types::TypeList<TQueries...>) {

                ([&]() {
                    using TQuery = TQueries;
                    if constexpr (!std::same_as<TQuery, ecs::entity::NullQuery>) {
                        using HandleType = typename TQuery::HandleType;
                        using ReadSet = core::common::traits::ConcatList<
                            typename TQuery::ReadSet::ComponentList,
                            typename core::common::traits::WrapElements<
                                ecs::components::DirtyComponentSpec, typename TQuery::DirtySet::ComponentList
                            >::list
                        >;

                        auto& manager = ecsDataContainer.get<ecs::entity::EntityMutationManager<HandleType>>();

                        auto walkComponents = [&]<typename ... TComponents>(core::common::types::TypeList<TComponents...>) {
                            manager.template commitMutations<TComponents...>(
                                ecsDataContainer.get<ecs::entity::EntityManager<HandleType>>()
                            );
                        };
                        walkComponents(typename ReadSet::list{});
                    }
                }(), ...);

            };

            walkQueries(typename QueryInfo::list{});

            return true;
        }


        bool publishEntityMutations(EcsDataContainer& ecsDataContainer) noexcept {

            // each sink is associated with one command buffer to make sure mutations can be run in parallel later on
            constexpr std::size_t BufferCount = std::tuple_size_v<decltype(entityMutationBuffers_)>;
            ([&]<std::size_t... Idx>(std::index_sequence<Idx...>) {

                ([&]() {
                    auto& buffer = std::get<Idx>(entityMutationBuffers_);
                    using BufferType = std::remove_cvref_t<decltype(buffer)>;

                    if constexpr (!std::same_as<BufferType, std::monostate>) {
                        auto& entityMutationManager = ecsDataContainer.get<
                            ecs::entity::EntityMutationManager<typename BufferType::HandleType>
                        >();
                        buffer.flush(entityMutationManager);
                    }
                }(), ...);

            }(std::make_index_sequence<BufferCount>{}));

            return true;
        }

    };

} // namespace helios::ecs::common