/**
 * @file EntityProxy.ixx
 * @brief Proxy for accessing and modifying entity components from Queries.
 */
module;

#include <vector>

export module helios.ecs.entity.EntityProxy;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;

import helios.core.common.traits;

import helios.ecs.command.EntityMutationCommandSink;


export namespace helios::ecs::entity {

    template<typename THandle, typename ... TWriteComponents>
    class EntityProxy {


        THandle handle_;

        using EntityMutationCommandSink = ecs::command::EntityMutationCommandSink<THandle, TWriteComponents...>;

        EntityMutationCommandSink* sink_;
    public:

        explicit EntityProxy(THandle handle, EntityMutationCommandSink* sink)
        : handle_{handle}, sink_{sink} {}

        THandle handle() {
            return handle_;
        }

        template<typename TComponent, typename TValue>
        requires(core::common::traits::IsInList<TComponent, TWriteComponents...>::value)
        void setTrackedValue(TComponent* cmp, TValue&& value) {
            sink_->template markDirty<TComponent>(handle_);
            cmp->setValue(std::forward<TValue>(value));
        }


    };



}