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

        using EntityAccessSet3= EntityAccessSet<
            ReadSet<PositionComponent<GameObjectHandle>>,
            WriteSet<>
        >;

        using EntityAccessSet4 = EntityAccessSet<
            ReadSet<PositionComponent<GameObjectHandle>>,
            WriteSet<VelocityComponent<ParticleHandle>>
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