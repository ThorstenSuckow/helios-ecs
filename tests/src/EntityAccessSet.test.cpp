#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::entity;


namespace {


    struct GameObjectHandle{};

    struct ParticleHandle{};

    template<typename TMOwnerHandle>
    struct PositionComponent {
        using HandleType = TMOwnerHandle;
    };

    template<typename TMOwnerHandle>
    struct VelocityComponent {
        using HandleType = TMOwnerHandle;
    };


    struct Foo {

        using EntityAccessSet1 = EntityAccessSet<
            ReadSet<
                PositionComponent<GameObjectHandle>,
                VelocityComponent<ParticleHandle>,
                VelocityComponent<GameObjectHandle>
            >,
            WriteSet<VelocityComponent<GameObjectHandle>>
        >;

        using EntityAccessSet2 = EntityAccessSet<
            ReadSet<PositionComponent<GameObjectHandle>>,
            WriteSet<VelocityComponent<GameObjectHandle>>
        >;

        EntityAccessSet1 entityAccessSet;
        EntityAccessSet2 entityAccessSet2;
    };
};

TEST(EntityAccessSet, Components) {

    EXPECT_TRUE((
        std::same_as<
            ReadSet<
                PositionComponent<GameObjectHandle>,VelocityComponent<ParticleHandle>,VelocityComponent<GameObjectHandle>
            >,
            Foo::EntityAccessSet1::ReadComponentSet
        >
    ));

    EXPECT_TRUE((
        std::same_as<
            WriteSet<
                VelocityComponent<GameObjectHandle>
            >,
            Foo::EntityAccessSet1::WriteComponentSet
        >
    ));
}


TEST(EntityAccessSet, tuples) {

    EXPECT_EQ(2, (std::tuple_size_v<typename Foo::EntityAccessSet1::ReadHandles>));
    EXPECT_EQ(1, (std::tuple_size_v<typename Foo::EntityAccessSet1::WriteHandles>));
    EXPECT_EQ(2, (std::tuple_size_v<typename Foo::EntityAccessSet1::AccessHandles>));

    EXPECT_EQ(1, (std::tuple_size_v<typename Foo::EntityAccessSet2::AccessHandles>));


    // READ
    EXPECT_TRUE((
        std::same_as<
            GameObjectHandle,
            std::tuple_element_t<0, typename Foo::EntityAccessSet1::ReadHandles>
        >
    ));

    EXPECT_TRUE((
        std::same_as<
            ParticleHandle,
            std::tuple_element_t<1, typename Foo::EntityAccessSet1::ReadHandles>
        >
    ));

    // WRITE
    EXPECT_TRUE((
       std::same_as<
           GameObjectHandle,
           std::tuple_element_t<0, typename Foo::EntityAccessSet1::WriteHandles>
       >
   ));


    //ACCESS
    EXPECT_TRUE((
        std::same_as<
            GameObjectHandle,
            std::tuple_element_t<0, typename Foo::EntityAccessSet1::AccessHandles>
            >
    ));

    EXPECT_TRUE((
        std::same_as<
            ParticleHandle,
            std::tuple_element_t<1, typename Foo::EntityAccessSet1::AccessHandles>
        >
    ));


}