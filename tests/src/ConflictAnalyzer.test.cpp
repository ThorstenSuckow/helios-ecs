#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;
import helios.core;

using namespace helios::core::common::types;
using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::common::ConflictAnalyzer;
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
                VelocityComponent
            >,
            WriteSet<VelocityComponent>
        >;

        using EntityAccessSet2 = EntityAccessSet<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<VelocityComponent>
        >;

        using EntityAccessSet3 = EntityAccessSet<
            ParticleHandle,
            ReadSet<PositionComponent>,
            WriteSet<>
        >;

        using EntityAccessSet4 = EntityAccessSet<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<VelocityComponent>
        >;

    };
};

TEST(ConflictTest, Components) {

    using AccessSet = TypeList<Foo::EntityAccessSet1, Foo::EntityAccessSet2>;

    static constexpr bool value = HasConflict<
        TypeList<Foo::EntityAccessSet1, Foo::EntityAccessSet2>
        >::value;
    EXPECT_TRUE(value);

    static constexpr bool value2 = HasConflict<
            TypeList<Foo::EntityAccessSet1, Foo::EntityAccessSet3>
            >::value;
    EXPECT_FALSE(value2);

    static constexpr bool value3 = HasConflict<
            TypeList<Foo::EntityAccessSet1, Foo::EntityAccessSet3, Foo::EntityAccessSet4>
            >::value;
    EXPECT_TRUE(value3);

}