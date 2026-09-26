/**
 * @file EntityMutationBuffer.ixx
 * @brief Command buffer for entity mutation related commands.
 */
module;

#include <tuple>
#include <vector>

export module helios.ecs.entity.mutation.EntityMutationBuffer;

import helios.ecs.entity.mutation.EntityMutationManager;
import helios.ecs.command.commands;
import helios.ecs.component.components;

export namespace helios::ecs::entity::mutation {

    template<typename THandle, typename ... TWriteComponents>
    class EntityMutationBuffer {

        using EntityMutationManager = EntityMutationManager<THandle>;

        std::tuple<
            std::vector<commands::AddComponentCommand<THandle, TWriteComponents>>...,
            std::vector<commands::AddComponentCommand<THandle, components::DirtyComponentSpec<TWriteComponents>>>...
        > addComponents_{};

        std::tuple<
            std::vector<commands::RemoveComponentCommand<THandle, TWriteComponents>>
            ...
        > removeComponents_{};

        template<typename TTuple>
        void drainImpl(EntityMutationManager& mutationManager, TTuple& tuple ) {
            std::apply([&mutationManager](auto& ... args) {
                ([&]() {
                    mutationManager.submitBatch(std::move(args));
                    args.clear();
                }(), ...);
            }, tuple);
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


        void flush(EntityMutationManager& mutationManager) {
            drainImpl(mutationManager, addComponents_);
            drainImpl(mutationManager, removeComponents_);
        }

    };


}