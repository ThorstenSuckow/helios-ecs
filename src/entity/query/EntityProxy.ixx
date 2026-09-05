/**
 * @file EntityProxy.ixx
 * @brief Proxy for accessing and modifying entity components from Queries.
 */
module;

#include <vector>

export module helios.ecs.entity.query.EntityProxy;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;

import helios.core.common.traits;

import helios.ecs.command.commands;
import helios.ecs.component.components;

import helios.ecs.entity.mutation.EntityMutationBuffer;


export namespace helios::ecs::entity::query {

    template<typename THandle, typename ... TWriteComponents>
    class EntityProxy {


        THandle handle_;

        using EntityMutationBuffer = mutation::EntityMutationBuffer<THandle, TWriteComponents...>;

        EntityMutationBuffer* buffer_;
    public:

        explicit EntityProxy(THandle handle, EntityMutationBuffer* buffer)
        : handle_{handle}, buffer_{buffer} {}

        THandle handle() {
            return handle_;
        }

        template<typename TComponent, typename TValue>
        requires(core::common::traits::IsInList<TComponent, TWriteComponents...>::value)
        void setTrackedValue(TComponent* cmp, TValue&& value) {

            using CmdCompType = commands::AddComponentCommand<components::DirtyComponentSpec<std::remove_cvref_t<TComponent>>>;
            buffer_->add(CmdCompType{handle_});


            cmp->setValue(std::forward<TValue>(value));
        }


    };



}