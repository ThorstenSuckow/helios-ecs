/**
 * @file EntityProxy.ixx
 * @brief Proxy for accessing and modifying entity components from Queries.
 */
module;

#include <bitset>
#include <cassert>

export module helios.ecs.entity.query.EntityProxy;

import helios.ecs.entity.EntityManager;
import helios.ecs.entity.Entity;

import helios.core.common.traits;
import helios.core.common.types;

import helios.ecs.command.commands;
import helios.ecs.component.components;

import helios.ecs.entity.mutation.EntityMutationBuffer;


export namespace helios::ecs::entity::query {

    template<typename THandle, typename TMutableSet, typename TAddSet>
    class EntityProxy {

        using MutableComponents = TMutableSet::ComponentList;
        using AddComponents = TAddSet::ComponentList;

        THandle handle_;

        template<typename ...T>
        struct EntityMutationBufferWrap;

        using WriteList = typename core::common::traits::UniqueList<
            core::common::types::TypeList<>,
            typename core::common::traits::ConcatList<
                typename TMutableSet::ComponentList, typename TAddSet::ComponentList
            >::list
        >::list;

        // EntityMutationBuffer
        template<typename ... TWriteComponents>
        struct EntityMutationBufferWrap<THandle, core::common::types::TypeList<TWriteComponents...>> {
            using type = mutation::EntityMutationBuffer<THandle, TWriteComponents...>;
        };
        using EntityMutationBuffer = EntityMutationBufferWrap<THandle, WriteList>::type;
        EntityMutationBuffer* buffer_;

        // MutableComponents
        template<typename T>
        using PtrWrap = T*;
        using MutableComponentTuple = typename core::common::traits::ListToTuple<
            typename core::common::traits::WrapElements<PtrWrap, MutableComponents>::list
        >::tuple;
        MutableComponentTuple mutableComponents_;

        template<typename TComponent>
        TComponent* mutableComponent() {
            return std::get<TComponent*>(mutableComponents_);
        }

        std::bitset<TMutableSet::size> tracked_{};
        std::bitset<TAddSet::size> added_{};

    public:

        explicit EntityProxy(THandle handle, EntityMutationBuffer* buffer, MutableComponentTuple mutableComponents)
        : handle_{handle}, buffer_{buffer}, mutableComponents_ {mutableComponents} {}

        THandle handle() {
            return handle_;
        }
        
        template<typename TComponent>
        requires  (core::common::traits::IsInList<TComponent, MutableComponents>::value)
        TComponent* track() {

            constexpr auto position = core::common::traits::IsInList<TComponent, MutableComponents>::index;
            if (tracked_.test(position)) {
                return mutableComponent<TComponent>();;
            }
            tracked_[position] = true;

            using CmdCompType = commands::AddComponentCommand<components::DirtyComponentSpec<std::remove_cvref_t<TComponent>>>;
            buffer_->add(CmdCompType{handle_});
            
            return mutableComponent<TComponent>();
        }
        
        template<typename TComponent, typename ... TArgs>
        requires (core::common::traits::IsInList<TComponent, AddComponents>::value)
        void add(TArgs&&... args) {

            constexpr auto position = core::common::traits::IsInList<TComponent, AddComponents>::index;
            if (added_.test(position)) {
                assert(false && "Component already added to entity.");
                return;
            }
            added_[position] = true;

            using CmdCompType = commands::AddComponentCommand<TComponent>;

            buffer_->add(CmdCompType{handle_, std::forward<TArgs>(args)...});
        }

    };



}