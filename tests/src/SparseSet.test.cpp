#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;


using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::storage;


// Internal linkage keeps this helper distinct from identically named helpers
// in other test TUs (ODR safety for template instantiations).
namespace {

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
    SparseSet<TestEntity> storage;

    EXPECT_FALSE(storage.contains(EntityId{1}));

    auto* ent = storage.emplace(EntityId{1}, TestEntity{});


    EXPECT_TRUE(storage.contains(EntityId{1}));

    EXPECT_NE(ent, nullptr);

    EXPECT_EQ(storage.emplace(EntityId{1}, TestEntity{}), nullptr);


}

TEST(SparseSetTest, get) {
    SparseSet<TestEntity> storage;

    auto* ent = storage.emplace(EntityId{1}, TestEntity{10});

    auto* entGet = storage.get(EntityId{1});

    EXPECT_EQ(ent->value, entGet->value);
}


TEST(SparseSetTest, remove) {
    SparseSet<TestEntity> storage;

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