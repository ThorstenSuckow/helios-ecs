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
import helios.ecs.entity.storage;

import helios.core.common.traits;
import helios.core.common.types;

import helios.ecs.command.commands;
import helios.ecs.component.components;

import helios.ecs.entity.mutation.EntityMutationBuffer;


export namespace helios::ecs::entity::query {

    template<typename THandle, typename TModSet, typename TStructMutSet>
    class EntityProxy {

        using ModifiableComponents = TModSet::list;
        using StructuralMutationComponents = TStructMutSet::list;

        THandle handle_;

        template<typename ...T>
        struct EntityMutationBufferWrap;

        using WriteList = typename core::common::traits::UniqueList<
            core::common::types::TypeList<>,
            typename core::common::traits::ConcatList<
                typename TModSet::list, typename TStructMutSet::list
            >::list
        >::list;

        // EntityMutationBuffer
        template<typename ... TWriteComponents>
        struct EntityMutationBufferWrap<THandle, core::common::types::TypeList<TWriteComponents...>> {
            using type = mutation::EntityMutationBuffer<THandle, TWriteComponents...>;
        };
        using EntityMutationBuffer = EntityMutationBufferWrap<THandle, WriteList>::type;
        EntityMutationBuffer* buffer_;

        // ModifiableComponents
        template<typename T>
        using PtrWrap = T*;
        using ModifiableComponentTuple = typename core::common::traits::ListToTuple<
            typename core::common::traits::WrapElements<PtrWrap, ModifiableComponents>::list
        >::tuple;
        ModifiableComponentTuple modifiableComponents_;

        template<typename T>
        using SparseSetWrap = storage::SparseSet<THandle, T>*;
        using ModifiableSparseSet = typename core::common::traits::ListToTuple<
            typename core::common::traits::WrapElements<SparseSetWrap, ModifiableComponents>::list
        >::tuple;
        ModifiableSparseSet modifiableSparseSets_;

        template<typename TComponent>
        TComponent* modifiableComponent() {
            return std::get<TComponent*>(modifiableComponents_);
        }

        template<typename TComponent>
        storage::SparseSet<THandle, TComponent>* modifiableSparseSet() {
            return std::get<storage::SparseSet<THandle, TComponent>*>(modifiableSparseSets_);
        }

        std::bitset<TModSet::size> tracked_{};
        std::bitset<TStructMutSet::size> added_{};

    public:

        explicit EntityProxy(THandle handle,
            EntityMutationBuffer* buffer,
            ModifiableComponentTuple modifiableComponents,
            ModifiableSparseSet modifiableSparseSets
        )
        : handle_{handle}, buffer_{buffer},
        modifiableComponents_ {modifiableComponents},
        modifiableSparseSets_{modifiableSparseSets} {}

        THandle handle() {
            return handle_;
        }

        template<typename TComponent>
        requires  (core::common::traits::IsInList<TComponent, ModifiableComponents>::value)
        TComponent* track() {

            constexpr auto position = core::common::traits::IsInList<TComponent, ModifiableComponents>::index;
            if (tracked_.test(position)) {
                return modifiableComponent<TComponent>();;
            }
            tracked_[position] = true;

            modifiableSparseSet<TComponent>()->markDirty(handle_.entityId());

            return modifiableComponent<TComponent>();
        }

        template<typename TComponent, typename ... TArgs>
        requires (core::common::traits::IsInList<TComponent, StructuralMutationComponents>::value)
        void add(TArgs&&... args) {

            constexpr auto position = core::common::traits::IsInList<TComponent, StructuralMutationComponents>::index;
            if (added_.test(position)) {
                assert(false && "Component already added to entity.");
                return;
            }
            added_[position] = true;

            using CmdCompType = commands::AddComponentCommand<THandle, TComponent>;

            buffer_->add(CmdCompType{handle_, std::forward<TArgs>(args)...});
        }

    };



}