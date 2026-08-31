#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;


namespace {


    struct GameObjectHandle{};

    struct ParticleHandle{};

    template<typename TMOwnerHandle>
    struct PositionComponent {
        using Handle_type = TMOwnerHandle;
    };

    template<typename TMOwnerHandle>
    struct VelocityComponent {
        using Handle_type = TMOwnerHandle;
    };


    struct Foo {

        using EntityAccessSet = EntityAccessSet<
            Read<
                PositionComponent<GameObjectHandle>,
                VelocityComponent<ParticleHandle>,
                VelocityComponent<GameObjectHandle>
            >,
            Write<VelocityComponent<GameObjectHandle>>
        >;

        using EntityAccessSet2 = EntityAccessSet<
            Read<PositionComponent<GameObjectHandle>>,
            Write<VelocityComponent<GameObjectHandle>>
        >;

        EntityAccessSet entityAccessSet;
        EntityAccessSet2 entityAccessSet2;
    };
};

TEST(EntityAccessSet, Components) {

    EXPECT_TRUE((
        std::same_as<
            helios::ecs::common::types::Read<
                PositionComponent<GameObjectHandle>,VelocityComponent<ParticleHandle>,VelocityComponent<GameObjectHandle>
            >,
            Foo::EntityAccessSet::ReadComponentSet
        >
    ));

    EXPECT_TRUE((
        std::same_as<
            helios::ecs::common::types::Write<
                VelocityComponent<GameObjectHandle>
            >,
            Foo::EntityAccessSet::WriteComponentSet
        >
    ));
}


TEST(EntityAccessSet, tuples) {

    EXPECT_EQ(2, (std::tuple_size_v<typename Foo::EntityAccessSet::ReadHandles>));
    EXPECT_EQ(1, (std::tuple_size_v<typename Foo::EntityAccessSet::WriteHandles>));
    EXPECT_EQ(2, (std::tuple_size_v<typename Foo::EntityAccessSet::AccessHandles>));

    EXPECT_EQ(1, (std::tuple_size_v<typename Foo::EntityAccessSet2::AccessHandles>));


    // READ
    EXPECT_TRUE((
        std::same_as<
            GameObjectHandle,
            std::tuple_element_t<0, typename Foo::EntityAccessSet::ReadHandles>
        >
    ));

    EXPECT_TRUE((
        std::same_as<
            ParticleHandle,
            std::tuple_element_t<1, typename Foo::EntityAccessSet::ReadHandles>
        >
    ));

    // WRITE
    EXPECT_TRUE((
       std::same_as<
           GameObjectHandle,
           std::tuple_element_t<0, typename Foo::EntityAccessSet::WriteHandles>
       >
   ));


    //ACCESS
    EXPECT_TRUE((
        std::same_as<
            GameObjectHandle,
            std::tuple_element_t<0, typename Foo::EntityAccessSet::AccessHandles>
            >
    ));

    EXPECT_TRUE((
        std::same_as<
            ParticleHandle,
            std::tuple_element_t<1, typename Foo::EntityAccessSet::AccessHandles>
        >
    ));


}