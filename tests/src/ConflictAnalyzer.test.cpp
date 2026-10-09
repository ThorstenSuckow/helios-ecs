#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;
import helios.core;


using namespace helios::ecs;
using namespace helios::ecs::entity;
using namespace helios::core::common::types;
using namespace helios::ecs::scheduling::concepts;
using namespace helios::ecs::scheduling::traits;


namespace {

    struct Foo{};
    struct Bar{};
    struct Foobar{};



    struct GameObjectHandle{};

    struct ParticleHandle{};

    struct PositionComponent {
    };

    struct VelocityComponent {
    };


    struct RotationComponent {
    };

    struct ScaleComponent {
    };

    struct HealthComponent {
    };

    struct GameObjectReadPositionSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectReadVelocitySystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<VelocityComponent>,
            WriteSet<>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectWritePositionSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<VelocityComponent>,
            WriteSet<PositionComponent>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectWriteVelocitySystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<VelocityComponent>
        >;
        void update(Q) noexcept {}
    };

    struct ParticleWritePositionSystem {
        using Q = entity::query::Query<
            ParticleHandle,
            ReadSet<VelocityComponent>,
            WriteSet<PositionComponent>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectWriteRotationSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<RotationComponent>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectReadScaleSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<ScaleComponent>,
            WriteSet<>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectWriteScaleSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<VelocityComponent>,
            WriteSet<ScaleComponent>
        >;
        void update(Q) noexcept {}
    };

    struct ParticleWriteScaleSystem {
        using Q = entity::query::Query<
            ParticleHandle,
            ReadSet<VelocityComponent>,
            WriteSet<ScaleComponent>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectWriteHealthSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<HealthComponent>
        >;
        void update(Q) noexcept {}
    };

    struct GameObjectMultiQuerySystem {
        using PositionQ = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<>
        >;
        using VelocityQ = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<VelocityComponent>
        >;
        void update(PositionQ, VelocityQ) noexcept {}
    };

    struct GameObjectReadPositionCallableSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<PositionComponent>,
            WriteSet<>
        >;
        void operator()(Q) noexcept {}
    };

    struct GameObjectWritePositionCallableSystem {
        using Q = entity::query::Query<
            GameObjectHandle,
            ReadSet<VelocityComponent>,
            WriteSet<PositionComponent>
        >;
        void operator()(Q) noexcept {}
    };

};

/**
 * Typed systems: concept result should match the conflict analyzer result.
 */
TEST(ConflictFreeConceptsTest, TypedSystemsMirrorConflictAnalyzerScenarios) {
    static_assert(ConflictFreeTypedSystems<GameObjectReadPositionSystem, GameObjectReadVelocitySystem>);
    static_assert(!ConflictFreeTypedSystems<GameObjectReadPositionSystem, GameObjectWritePositionSystem>);
    static_assert(!ConflictFreeTypedSystems<GameObjectWritePositionSystem, GameObjectWritePositionSystem>);
    static_assert(ConflictFreeTypedSystems<GameObjectWritePositionSystem, ParticleWritePositionSystem>);
    static_assert(ConflictFreeTypedSystems<GameObjectWriteRotationSystem, GameObjectWriteVelocitySystem>);
    static_assert(!ConflictFreeTypedSystems<GameObjectMultiQuerySystem, GameObjectReadVelocitySystem>);

    SUCCEED();
}

/**
 * Callable systems: same checks as above, but via operator().
 */
TEST(ConflictFreeConceptsTest, CallableSystemsMirrorConflictAnalyzerScenarios) {
    static_assert(ConflictFreeCallableSystems<GameObjectReadPositionCallableSystem, GameObjectReadPositionCallableSystem>);
    static_assert(!ConflictFreeCallableSystems<GameObjectReadPositionCallableSystem, GameObjectWritePositionCallableSystem>);

    SUCCEED();
}

/**
 * Small sequential setup with one conflicting and one non-conflicting case.
 */
TEST(ConflictFreeConceptsTest, SequentialGroupsMirrorConflictAnalyzerScenarios) {
    using GroupA = system::Sequential<GameObjectReadPositionSystem, GameObjectWriteVelocitySystem>;
    using GroupB = system::Sequential<GameObjectReadVelocitySystem>;
    using GroupC = system::Sequential<ParticleWritePositionSystem>;

    static_assert(!ConflictFreeSequentialSystems<GroupA, GroupB>);
    static_assert(ConflictFreeSequentialSystems<GroupA, GroupC>);

    SUCCEED();
}

/**
 * Five-group setup: verify non-neighbor conflict detection and a safe variant.
 */
TEST(ConflictFreeConceptsTest, SequentialFiveGroupsParallelCoverage) {
    using Group1 = system::Sequential<GameObjectReadScaleSystem>;
    using Group2 = system::Sequential<ParticleWritePositionSystem>;
    using Group3 = system::Sequential<GameObjectWriteRotationSystem>;
    using Group4 = system::Sequential<GameObjectWriteHealthSystem>;
    using Group5 = system::Sequential<GameObjectWriteScaleSystem>;

    using FreeGroup1 = system::Sequential<GameObjectReadScaleSystem>;
    using FreeGroup2 = system::Sequential<ParticleWritePositionSystem>;
    using FreeGroup3 = system::Sequential<GameObjectWriteRotationSystem>;
    using FreeGroup4 = system::Sequential<GameObjectWriteHealthSystem>;
    using FreeGroup5 = system::Sequential<GameObjectReadVelocitySystem>;

    // Conflict is between Group1 and Group5 (same write target), not neighbors.
    static_assert(!ConflictFreeSequentialSystems<Group1, Group2, Group3, Group4, Group5>);
    static_assert(ConflictFreeSequentialSystems<FreeGroup1, FreeGroup2, FreeGroup3, FreeGroup4, FreeGroup5>);

    SUCCEED();
}

/**
 * Checks two guard rails: concepts require a minimum number of systems,
 * and they evaluate system types after cv/ref qualifiers are stripped.
 */
TEST(ConflictFreeConceptsTest, ArityAndCvRefGuards) {
    static_assert(!ConflictFreeTypedSystems<GameObjectReadPositionSystem>);
    static_assert(!ConflictFreeCallableSystems<GameObjectReadPositionCallableSystem>);

    using SingleGroup = system::Sequential<GameObjectReadPositionSystem>;
    static_assert(ConflictFreeSequentialSystems<SingleGroup>);

    static_assert(ConflictFreeTypedSystems<const GameObjectReadPositionSystem&, ParticleWritePositionSystem&&>);
    static_assert(!ConflictFreeCallableSystems<const GameObjectReadPositionCallableSystem&,
        GameObjectWritePositionCallableSystem&&>);

    SUCCEED();
}

/**
 * Same component, different handles: should stay conflict-free.
 */
TEST(ConflictFreeConceptsTest, SequentialFiveGroupsSameComponentDifferentHandlesStayConflictFree) {
    using Group1 = system::Sequential<GameObjectWriteScaleSystem>;
    using Group2 = system::Sequential<ParticleWritePositionSystem>;
    using Group3 = system::Sequential<GameObjectWriteRotationSystem>;
    using Group4 = system::Sequential<GameObjectWriteHealthSystem>;
    using Group5 = system::Sequential<ParticleWriteScaleSystem>;

    static_assert(ConflictFreeSequentialSystems<Group1, Group2, Group3, Group4, Group5>);

    SUCCEED();
}

/**
 * Mixed setup with one real same-handle conflict: concept must reject it.
 */
TEST(ConflictFreeConceptsTest, SequentialFiveGroupsMixedIsolationWithSingleRealConflict) {
    using Group1 = system::Sequential<GameObjectWriteScaleSystem>;
    using Group2 = system::Sequential<ParticleWriteScaleSystem>;
    using Group3 = system::Sequential<GameObjectWriteHealthSystem>;
    using Group4 = system::Sequential<GameObjectWriteRotationSystem>;
    using Group5 = system::Sequential<GameObjectWriteScaleSystem>;

    // Group1 and Group5 conflict; Group2 stays isolated by handle.
    static_assert(!ConflictFreeSequentialSystems<Group1, Group2, Group3, Group4, Group5>);

    SUCCEED();
}

/**
 * Baseline: read/read on the same handle is conflict-free.
 */
TEST(ConflictAnalyzerTest, ReadReadSameHandleIsConflictFree) {
    static constexpr bool value = HasConflict<
        TypeList<GameObjectReadPositionSystem, GameObjectReadVelocitySystem>
    >::value;

    static_assert(!value);
    EXPECT_FALSE(value);
}

/**
 * Read/write on the same handle must conflict.
 */
TEST(ConflictAnalyzerTest, ReadWriteSameHandleConflicts) {
    static constexpr bool value = HasConflict<
        TypeList<GameObjectReadPositionSystem, GameObjectWritePositionSystem>
    >::value;

    static_assert(value);
    EXPECT_TRUE(value);
}

/**
 * Write/write on the same handle must conflict.
 */
TEST(ConflictAnalyzerTest, WriteWriteSameHandleConflicts) {
    static constexpr bool value = HasConflict<
        TypeList<GameObjectWritePositionSystem, GameObjectWritePositionSystem>
    >::value;

    static_assert(value);
    EXPECT_TRUE(value);
}

/**
 * Same component on different handles should not conflict.
 */
TEST(ConflictAnalyzerTest, SameComponentDifferentHandlesStayConflictFree) {
    static constexpr bool value = HasConflict<
        TypeList<GameObjectWritePositionSystem, ParticleWritePositionSystem>
    >::value;

    static_assert(!value);
    EXPECT_FALSE(value);
}

/**
 * Different write sets on the same handle should not conflict.
 */
TEST(ConflictAnalyzerTest, DifferentWriteComponentsAreConflictFree) {
    static constexpr bool value = HasConflict<
        TypeList<GameObjectWriteRotationSystem, GameObjectWriteVelocitySystem>
    >::value;

    static_assert(!value);
    EXPECT_FALSE(value);
}

/**
 * Multi-query systems must expose conflicts across all query signatures.
 */
TEST(ConflictAnalyzerTest, MultiQuerySystemDetectsConflictsAcrossQueries) {
    static constexpr bool value = HasConflict<
        TypeList<GameObjectMultiQuerySystem, GameObjectReadVelocitySystem>
    >::value;

    static_assert(value);
    EXPECT_TRUE(value);
}

/**
 * Sequential baseline: one conflicting and one safe grouping.
 */
TEST(ConflictAnalyzerTest, SequentialGroupsConflictAndConflictFreeScenarios) {
    using GroupA = system::Sequential<GameObjectReadPositionSystem, GameObjectWriteVelocitySystem>;
    using GroupB = system::Sequential<GameObjectReadVelocitySystem>;
    using GroupC = system::Sequential<ParticleWritePositionSystem>;

    static constexpr bool conflict = HasConflict<TypeList<GroupA, GroupB>>::value;
    static constexpr bool conflictFree = HasConflict<TypeList<GroupA, GroupC>>::value;

    static_assert(conflict);
    static_assert(!conflictFree);
    EXPECT_TRUE(conflict);
    EXPECT_FALSE(conflictFree);
}

/**
 * A single sequential group must be conflict-free.
 */
TEST(ConflictAnalyzerTest, SingleSequentialGroupIsConflictFree) {
    using SingleGroup = system::Sequential<GameObjectReadPositionSystem, GameObjectWriteVelocitySystem>;

    static constexpr bool conflict = HasConflict<TypeList<SingleGroup>>::value;

    static_assert(!conflict);
    EXPECT_FALSE(conflict);
}

/**
 * Five groups: conflict can come from non-neighbor groups.
 */
TEST(ConflictAnalyzerTest, SequentialFiveGroupsNonNeighborConflictAndConflictFreeCases) {
    using Group1 = system::Sequential<GameObjectReadScaleSystem>;
    using Group2 = system::Sequential<ParticleWritePositionSystem>;
    using Group3 = system::Sequential<GameObjectWriteRotationSystem>;
    using Group4 = system::Sequential<GameObjectWriteHealthSystem>;
    using Group5 = system::Sequential<GameObjectWriteScaleSystem>;

    using FreeGroup1 = system::Sequential<GameObjectReadScaleSystem>;
    using FreeGroup2 = system::Sequential<ParticleWritePositionSystem>;
    using FreeGroup3 = system::Sequential<GameObjectWriteRotationSystem>;
    using FreeGroup4 = system::Sequential<GameObjectWriteHealthSystem>;
    using FreeGroup5 = system::Sequential<GameObjectReadVelocitySystem>;

    static constexpr bool hasConflict = HasConflict<
        TypeList<Group1, Group2, Group3, Group4, Group5>
    >::value;

    static constexpr bool conflictFree = HasConflict<
        TypeList<FreeGroup1, FreeGroup2, FreeGroup3, FreeGroup4, FreeGroup5>
    >::value;

    static_assert(hasConflict);
    static_assert(!conflictFree);
    EXPECT_TRUE(hasConflict);
    EXPECT_FALSE(conflictFree);
}

/**
 * Five groups: same component across handles stays conflict-free.
 */
TEST(ConflictAnalyzerTest, SequentialFiveGroupsSameComponentDifferentHandlesStayConflictFree) {
    using Group1 = system::Sequential<GameObjectWriteScaleSystem>;
    using Group2 = system::Sequential<ParticleWritePositionSystem>;
    using Group3 = system::Sequential<GameObjectWriteRotationSystem>;
    using Group4 = system::Sequential<GameObjectWriteHealthSystem>;
    using Group5 = system::Sequential<ParticleWriteScaleSystem>;

    static constexpr bool conflict = HasConflict<
        TypeList<Group1, Group2, Group3, Group4, Group5>
    >::value;

    static_assert(!conflict);
    EXPECT_FALSE(conflict);
}

/**
 * Five groups: one same-handle conflict makes the whole set conflicting.
 */
TEST(ConflictAnalyzerTest, SequentialFiveGroupsMixedIsolationWithSingleRealConflict) {
    using Group1 = system::Sequential<GameObjectWriteScaleSystem>;
    using Group2 = system::Sequential<ParticleWriteScaleSystem>;
    using Group3 = system::Sequential<GameObjectWriteHealthSystem>;
    using Group4 = system::Sequential<GameObjectWriteRotationSystem>;
    using Group5 = system::Sequential<GameObjectWriteScaleSystem>;

    static constexpr bool conflict = HasConflict<
        TypeList<Group1, Group2, Group3, Group4, Group5>
    >::value;

    static_assert(conflict);
    EXPECT_TRUE(conflict);
}

TEST(ConflictAnalyzerTest, SequentialTwoGroupsMultipleSystemsWithSingleRealConflict) {
    using Group1 = system::Sequential<GameObjectWriteScaleSystem, GameObjectWriteHealthSystem, ParticleWriteScaleSystem>;
    using Group2 = system::Sequential<ParticleWriteScaleSystem>;

    static constexpr bool conflict = HasConflict<
        TypeList<Group1, Group2>
    >::value;

    static_assert(conflict);
    EXPECT_TRUE(conflict);
}

TEST(ConflictAnalyzerTest, TwoSystemsSameReturnValue) {
    auto s1 = []()-> Foo {return Foo{};};
    auto s2 = []()-> Foo {return Foo{};};
    auto s3 = []()-> Bar {return Bar{};};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(conflict);
    EXPECT_TRUE(conflict);
}

TEST(ConflictAnalyzerTest, TwoSystemsSameArg) {
    auto s1 = [](Foo&)-> Foo {return Foo{};};
    auto s3 = []()-> Foobar {return Foobar{};};
    auto s2 = [](Foo&)-> Bar {return Bar{};};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(conflict);
    EXPECT_TRUE(conflict);
}

TEST(ConflictAnalyzerTest, TwoSystemsSameArgButOneIsConst) {
    auto s1 = [](const Foo&)-> Foo {return Foo{};};
    auto s3 = []()-> Foobar {return Foobar{};};
    auto s2 = [](Foo&)-> Bar {return Bar{};};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(conflict);
    EXPECT_TRUE(conflict);
}


TEST(ConflictAnalyzerTest, TwoSystemsSameArgButBothAreConst) {
    auto s1 = [](const Foo) {};
    auto s3 = [](){};
    auto s2 = [](const Foo&) {};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(!conflict);
    EXPECT_TRUE(!conflict);
}

TEST(ConflictAnalyzerTest, TwoSystemsSameArgInOneGroupButBothAreConst) {
    auto s1 = [](const Foo&)-> Foo {return Foo{};};
    auto s3 = [](const Foo&)-> Foobar {return Foobar{};};
    auto s2 = [](const Bar&)-> Bar {return Bar{};};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(!conflict);
    EXPECT_TRUE(!conflict);
}

TEST(ConflictAnalyzerTest, TwoSystemsSameConstArgSameReturnType) {
    auto s1 = []()-> Foo {return Foo{};};
    auto s3 = []() {};
    auto s2 = [](const Foo&){};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(conflict);
    EXPECT_TRUE(conflict);
}

TEST(ConflictAnalyzerTest, TwoSystemsSameArgSameReturnType) {
    auto s1 = []()-> Foo {return Foo{};};
    auto s3 = []() {};
    auto s2 = [](Foo){};

    auto Group1 = system::Sequential(s1, s3);
    auto Group2 = system::Sequential(s2);

    static constexpr bool conflict = HasConflict<
        TypeList<decltype(Group1), decltype(Group2)>
    >::value;
    static_assert(conflict);
    EXPECT_TRUE(conflict);
}


