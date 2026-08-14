#include <gtest/gtest.h>
#include "helios-ecs-config.h"


import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;

// Wrapped in an anonymous namespace so these test-local helper types get
// internal linkage and their template instantiations are not shared/folded
// with identically named helpers in other test TUs (ODR safety).
namespace {

struct TestDomainTag {};
using TestHandle = EntityHandle<TestDomainTag>;
using TestRegistry = EntityRegistry<TestDomainTag>;
using TestEntityManager = EntityManager<TestHandle>;

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

class MyComponent {

    public:

    using Handle_type = TestHandle;

    int value = 0;


};

} // namespace

TEST(EntityManager, create) {

    TestEntityManager em;

    const auto handle = em.create();
    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 1);
}

TEST(EntityManager, destroy) {


    TestEntityManager em;

    const auto handle = em.create();
    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 1);

    em.emplace<MyComponent>(handle, 10);
    em.emplace<TestEntity>(handle, 10);
    EXPECT_TRUE(em.has<MyComponent>(handle));
    EXPECT_TRUE(em.has<TestEntity>(handle));

    EXPECT_TRUE(em.destroy(handle));

    EXPECT_FALSE(em.has<MyComponent>(handle));
    EXPECT_FALSE(em.has<TestEntity>(handle));
}

TEST(EntityManager, emplace) {

    TestEntityManager em;

    const auto handle = em.create();
    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 1);

    EXPECT_FALSE(em.has<MyComponent>(handle));

    auto* cmp = em.emplace<MyComponent>(handle, 10);

    EXPECT_TRUE(em.has<MyComponent>(handle));
    EXPECT_NE(cmp, nullptr);

    auto* ref = em.get<MyComponent>(handle);

    EXPECT_EQ(ref->value, 10);

    // ... and destroy
    EXPECT_TRUE(em.destroy(handle));

    EXPECT_EQ(em.get<MyComponent>(handle), nullptr);
}

TEST(EntityManager, remove) {

    TestEntityManager em;

    const auto handle = em.create();
    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 1);

    EXPECT_FALSE(em.has<MyComponent>(handle));

    auto* cmp = em.emplace<MyComponent>(handle, 10);
    EXPECT_TRUE(em.has<MyComponent>(handle));

    EXPECT_TRUE(em.remove<MyComponent>(handle));
    EXPECT_FALSE(em.has<MyComponent>(handle));

}