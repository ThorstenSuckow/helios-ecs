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

        using QueryAccessSet1 = QueryAccessSet<
            GameObjectHandle,
            ReadSet<
                PositionComponent,
                VelocityComponent,
                VelocityComponent
            >,
            WriteSet<VelocityComponent>
        >;

        using QueryAccessSet2 = QueryAccessSet<
            ParticleHandle,
            ReadSet<PositionComponent>,
            WriteSet<VelocityComponent>
        >;

        QueryAccessSet1 entityAccessSet;
        QueryAccessSet2 entityAccessSet2;
    };
};

TEST(QueryAccessSet, Components) {

    EXPECT_TRUE((
        std::same_as<
            ReadSet<
                PositionComponent, VelocityComponent,VelocityComponent
            >,
            Foo::QueryAccessSet1::ReadSet
        >
    ));

    EXPECT_TRUE((
        std::same_as<
            WriteSet<
                VelocityComponent
            >,
            Foo::QueryAccessSet1::WriteSet
        >
    ));
}


