/**
 * @file EntityMutationBuffer.ixx
 * @brief Command buffer for entity mutation related commands.
 */
module;

#include <tuple>
#include <vector>

export module helios.ecs.entity.mutation.EntityMutationBuffer;

import helios.ecs.entity.EntityManager;
import helios.ecs.command.commands;
import helios.ecs.component.components;

export namespace helios::ecs::entity::mutation {

    template<typename THandle, typename ... TWriteComponents>
    class EntityMutationBuffer {

        using EntityManager = EntityManager<THandle>;

        std::tuple<
            std::vector<commands::AddComponentCommand<THandle, TWriteComponents>>...
        > addComponents_{};

        std::tuple<
            std::vector<commands::RemoveComponentCommand<THandle, TWriteComponents>>...
        > removeComponents_{};

        void drainCommands(EntityManager& entityManager) {
            auto& addCommands = addComponents_;
            auto& removeCommands = removeComponents_;

            auto drain = []<typename TTuple>(TTuple& ttuple, EntityManager& em) {

                std::apply([&em](auto& ... vectors) {
                    ([&]() {
                        using VectorType = std::remove_cvref_t<decltype(vectors)>;
                        using CommandType = typename VectorType::value_type;
                        using ComponentType = typename CommandType::ComponentType;

                        if constexpr (std::same_as<CommandType, commands::AddComponentCommand<THandle, ComponentType>>) {
                            for (auto& cmd : vectors) {
                                em.template emplace<ComponentType>(cmd.handle, std::move(cmd.component));
                            }
                        } else if constexpr (std::same_as<CommandType, commands::RemoveComponentCommand<THandle, ComponentType>>) {
                            for (auto& cmd : vectors) {
                                em.template remove<ComponentType>(cmd.handle);
                            }
                        } else {
                            static_assert(false, "Unsupported command type for EntityMutationBuffer");
                        }

                        vectors.clear();
                    }(), ...);

                }, ttuple);
            };

            drain(addCommands, entityManager);
            drain(removeCommands, entityManager);
        }

    public:

        using HandleType = THandle;

        template<template <typename, typename> typename TCommand, typename TInner>
        void add(TCommand<THandle, TInner>&& cmd) {

            using CommandType = std::remove_cvref_t<TCommand<THandle, TInner>>;

            if constexpr (std::same_as<commands::AddComponentCommand<HandleType, TInner>, CommandType>) {
                auto& vec = std::get<std::vector<CommandType>>(addComponents_);
                vec.push_back(std::move(cmd));

            } else if constexpr (std::same_as<commands::RemoveComponentCommand<HandleType, TInner>, CommandType>) {
                auto& vec = std::get<std::vector<CommandType>>(removeComponents_);
                vec.push_back(std::move(cmd));
            } else {
                static_assert(false, "Unsupported command type for EntityMutationBuffer");
            }
        }


        void flush(EntityManager& entityManager) {
            drainCommands(entityManager);
        }

    };


}