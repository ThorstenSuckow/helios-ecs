#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;

// Internal linkage keeps these helpers distinct from identically named helpers
// in other test TUs (ODR safety for template instantiations).
namespace {

struct TestDomainTag {};
using TestHandle = EntityHandle<TestDomainTag>;
using TestRegistry = EntityRegistry<TestHandle>;

} // namespace


TEST(EntityRegistryTest, create) {

    TestRegistry registry;
    {
        auto handle = registry.create();

        EXPECT_EQ(handle.entityId(), 0);
        EXPECT_EQ(handle.versionId(), 1);
    }

    {
        auto handle = registry.create();

        EXPECT_EQ(handle.entityId(), 1);
        EXPECT_EQ(handle.versionId(), 1);
    }
}

TEST(EntityRegistryTest, isValid) {

    TestRegistry registry;

    auto handle = registry.create();

    EXPECT_TRUE(registry.isValid(handle));
    EXPECT_FALSE(registry.isValid({EntityId{2}, VersionId{1}}));

}


TEST(EntityRegistryTest, destroyAndreuse) {

    TestRegistry registry;

    auto handle = registry.create();

    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 1);


    EXPECT_FALSE(registry.destroy({EntityId{1}, VersionId{2}}));
    EXPECT_TRUE(registry.destroy(handle));

    EXPECT_FALSE(registry.isValid(handle));

    handle = registry.create();

    // reused id, updated version
    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 2);

}
