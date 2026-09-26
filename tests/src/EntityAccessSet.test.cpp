#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::entity;


namespace {


    struct GameObjectHandle{};

    struct ParticleHandle{};


    struct PositionComponent {
    };

    struct VelocityComponent {
    };


    struct Foo {

        using EntityAccessSet1 = EntityAccessSet<
            GameObjectHandle,
            ReadSet<
                PositionComponent,
                VelocityComponent,
                VelocityComponent
            >,
            WriteSet<VelocityComponent>
        >;

        using EntityAccessSet2 = EntityAccessSet<
            ParticleHandle,
            ReadSet<PositionComponent>,
            WriteSet<VelocityComponent>
        >;

        EntityAccessSet1 entityAccessSet;
        EntityAccessSet2 entityAccessSet2;
    };
};

TEST(EntityAccessSet, Components) {

    EXPECT_TRUE((
        std::same_as<
            ReadSet<
                PositionComponent, VelocityComponent,VelocityComponent
            >,
            Foo::EntityAccessSet1::ReadComponentSet
        >
    ));

    EXPECT_TRUE((
        std::same_as<
            WriteSet<
                VelocityComponent
            >,
            Foo::EntityAccessSet1::WriteComponentSet
        >
    ));
}


