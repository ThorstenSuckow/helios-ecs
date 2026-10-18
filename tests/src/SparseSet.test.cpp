#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;


using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::entity::storage;


// Internal linkage keeps this helper distinct from identically named helpers
// in other test TUs (ODR safety for template instantiations).
namespace {

    struct HandleType {};

class TestEntity {

public:
    int value = 0;

    bool remove_ = true;

    bool onRemove() {
        return remove_;
    }

    bool operator==(const TestEntity& other) const {
        return value == other.value;
    }
};

} // namespace

TEST(SparseSetTest, emplace) {
    SparseSet<HandleType, TestEntity> storage;

    EXPECT_FALSE(storage.contains(EntityId{1}));

    auto* ent = storage.emplace(EntityId{1}, TestEntity{});


    EXPECT_TRUE(storage.contains(EntityId{1}));

    EXPECT_NE(ent, nullptr);

    EXPECT_EQ(storage.emplace(EntityId{1}, TestEntity{}), nullptr);


}

TEST(SparseSetTest, get) {
    SparseSet<HandleType, TestEntity> storage;

    auto* ent = storage.emplace(EntityId{1}, TestEntity{10});

    auto* entGet = storage.get(EntityId{1});

    EXPECT_EQ(ent->value, entGet->value);
}


TEST(SparseSetTest, remove) {
    SparseSet<HandleType, TestEntity> storage;

    EXPECT_NE(storage.emplace(EntityId{1}, TestEntity{10}), nullptr);
    EXPECT_NE(storage.emplace(EntityId{2}, TestEntity{20}), nullptr);
    EXPECT_NE(storage.emplace(EntityId{3}, 30, false), nullptr);
    EXPECT_NE(storage.emplace(EntityId{4}, TestEntity{40}), nullptr);


    EXPECT_TRUE(storage.remove(EntityId{1}));
    EXPECT_EQ(storage.get(EntityId{1}), nullptr);
    EXPECT_EQ(storage.get(EntityId{2})->value, 20);
    EXPECT_EQ(storage.get(EntityId{3})->value, 30);
    EXPECT_EQ(storage.get(EntityId{4})->value, 40);

    storage.get(EntityId{3})->remove_ = true;
    EXPECT_TRUE(storage.remove(EntityId{3}));
    EXPECT_EQ(storage.get(EntityId{1}), nullptr);
    EXPECT_EQ(storage.get(EntityId{2})->value, 20);
    EXPECT_EQ(storage.get(EntityId{3}), nullptr);
    EXPECT_EQ(storage.get(EntityId{4})->value, 40);

}

namespace {

SparseSet<HandleType, TestEntity> filled(std::initializer_list<EntityId> ids) {
    SparseSet<HandleType, TestEntity> storage;
    for (const auto id : ids) {
        EXPECT_NE(storage.emplace(EntityId{id}, TestEntity{}), nullptr);
    }
    return storage;
}

} // namespace

TEST(SparseSetTest, maxEntityIdDenseRemoveMax) {
    SparseSet<HandleType, TestEntity> storage;
    for (EntityId i = 0; i <= 100; ++i) {
        ASSERT_NE(storage.emplace(EntityId{i}, TestEntity{}), nullptr);
    }
    EXPECT_EQ(storage.maxEntityId(), EntityId{100});

    EXPECT_TRUE(storage.remove(EntityId{100}));
    EXPECT_EQ(storage.maxEntityId(), Tombstone);
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), EntityId{99});
}

TEST(SparseSetTest, maxEntityIdMultipleRemovals) {
    SparseSet<HandleType, TestEntity> storage;
    for (EntityId i = 0; i <= 100; ++i) {
        ASSERT_NE(storage.emplace(EntityId{i}, TestEntity{}), nullptr);
    }
    EXPECT_TRUE(storage.remove(EntityId{100}));
    EXPECT_TRUE(storage.remove(EntityId{99}));
    EXPECT_TRUE(storage.remove(EntityId{98}));
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), EntityId{97});
}

TEST(SparseSetTest, maxEntityIdInsertBelowInvalidated) {
    SparseSet<HandleType, TestEntity> storage;
    for (EntityId i = 0; i <= 100; ++i) {
        ASSERT_NE(storage.emplace(EntityId{i}, TestEntity{}), nullptr);
    }
    EXPECT_TRUE(storage.remove(EntityId{100}));
    EXPECT_TRUE(storage.remove(EntityId{5}));
    ASSERT_NE(storage.emplace(EntityId{5}, TestEntity{}), nullptr);
    EXPECT_EQ(storage.maxEntityId(), Tombstone);
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), EntityId{99});
}

TEST(SparseSetTest, maxEntityIdInsertAboveInvalidated) {
    auto storage = filled({1, 2, 100});
    EXPECT_TRUE(storage.remove(EntityId{100}));
    ASSERT_NE(storage.emplace(EntityId{150}, TestEntity{}), nullptr);
    EXPECT_EQ(storage.maxEntityId(), EntityId{150});
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), EntityId{150});
}

TEST(SparseSetTest, maxEntityIdSparseFallback) {
    // budget (= 1 component) is exhausted before reaching id 3 -> fallback to max_element
    auto storage = filled({3, 100000});
    EXPECT_TRUE(storage.remove(EntityId{100000}));
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), EntityId{3});
}

TEST(SparseSetTest, maxEntityIdDownToZero) {
    auto storage = filled({0, 1});
    EXPECT_TRUE(storage.remove(EntityId{1}));
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), EntityId{0});

    EXPECT_TRUE(storage.remove(EntityId{0}));
    storage.finalizeMutations();
    EXPECT_EQ(storage.maxEntityId(), Tombstone);
}

