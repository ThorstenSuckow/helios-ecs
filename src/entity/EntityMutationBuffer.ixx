/**
 * @file EntityMutationBuffer.ixx
 * @brief Command buffer for entity mutation related commands.
 */
module;

#include <tuple>
#include <vector>

export module helios.ecs.entity.EntityMutationBuffer;

import helios.ecs.entity.EntityMutationManager;
import helios.ecs.command.commands;
import helios.ecs.component.components;

export namespace helios::ecs::entity {

    template<typename THandle, typename ... TWriteComponents>
    requires (std::same_as<THandle, typename TWriteComponents::HandleType> && ...)
    class EntityMutationBuffer {

        using EntityMutationManager = EntityMutationManager<THandle>;

        std::tuple<
            std::vector<commands::AddComponentCommand<TWriteComponents>>...,
            std::vector<commands::AddComponentCommand<components::DirtyComponentSpec<TWriteComponents>>>...
        > addComponents_{};

        std::tuple<
            std::vector<commands::RemoveComponentCommand<TWriteComponents>>
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

        template<template <typename> typename TCommand, typename TInner>
        void add(TCommand<TInner>&& cmd) {

            using CommandType = std::remove_cvref_t<TCommand<TInner>>;

            if constexpr (std::same_as<commands::AddComponentCommand<TInner>, CommandType>) {
                auto& vec = std::get<std::vector<CommandType>>(addComponents_);
                vec.push_back(std::move(cmd));

            } else if constexpr (std::same_as<commands::RemoveComponentCommand<TInner>, CommandType>) {
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