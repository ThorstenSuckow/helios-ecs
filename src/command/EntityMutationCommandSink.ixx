/**
 * @file EntityMutationCommandSink.ixx
 * @brief Command sink for entity mutation related commands.
 */
module;

#include <tuple>
#include <vector>

export module helios.ecs.command.EntityMutationCommandSink;

import helios.ecs.command.commands;
import helios.ecs.component.components;

export namespace helios::ecs::command {

    template<typename THandle, typename ... TWriteComponents>
    requires (std::same_as<THandle, typename TWriteComponents::HandleType> && ...)
    class EntityMutationCommandSink {

        std::tuple<
            std::vector<commands::AddComponentCommand<components::DirtyComponentSpec<TWriteComponents>>>
            ...
        > trackedComponents_{};

        std::tuple<
            std::vector<commands::AddComponentCommand<TWriteComponents>>
            ...
        > addComponents_{};

        std::tuple<
            std::vector<commands::RemoveComponentCommand<TWriteComponents>>
            ...
        > removeComponents_{};

        template<typename TFunc, typename TTuple>
        void drainImpl(TFunc& func, TTuple& tuple ) {
            std::apply([&func](auto& ... args) {
                ([&]() {
                    for (auto& cmd : args) {
                       std::invoke(func, std::move(cmd));
                    }
                    args.clear();
                }(), ...);
            }, tuple);
        }

    public:

        template<typename TComponent>
        void markDirty(THandle handle) {
            using CmdCompType = commands::AddComponentCommand<components::DirtyComponentSpec<std::remove_cvref_t<TComponent>>>;

            auto& vec = std::get<std::vector<CmdCompType>>(trackedComponents_);
            vec.push_back(CmdCompType{handle});
        }

        template<typename TComponent>
        void addComponent(THandle handle) {
            using CmdCompType = commands::AddComponentCommand<std::remove_cvref_t<TComponent>>;

            auto& vec = std::get<std::vector<CmdCompType>>(addComponents_);
            vec.push_back(CmdCompType{handle});
        }

        template<typename TComponent>
        void removeComponent(THandle handle) {
            using CmdCompType = commands::RemoveComponentCommand<std::remove_cvref_t<TComponent>>;

            auto& vec = std::get<std::vector<CmdCompType>>(removeComponents_);
            vec.push_back(CmdCompType{handle});
        }


        template<typename TFunc>
        void drain(TFunc&& func) {
            drainImpl(func, trackedComponents_);
            drainImpl(func, addComponents_);
            drainImpl(func, removeComponents_);
        }

    };


}