#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::types;


namespace {

    struct TestDomainTag {};
    using TestHandle = EntityHandle<TestDomainTag>;
};

TEST(EntityHandle, create) {

    auto handle = EntityHandle<TestDomainTag>();

    EXPECT_TRUE(handle.entityId() == 0);
    EXPECT_TRUE(handle.versionId() == InvalidVersion);
    EXPECT_TRUE(handle.strongId().value() == 0);
    EXPECT_FALSE(handle.isValid());

#ifdef HELIOS_DEBUG
    EXPECT_DEATH(EntityHandle<TestDomainTag>(0, 0), "EntityHandle must not be constructed with InvalidVersion.");
#endif

#ifdef HELIOS_DEBUG
    EXPECT_DEATH(EntityHandle<TestDomainTag>(1, 0), "EntityHandle must not be constructed with InvalidVersion.");
#endif

    handle = EntityHandle<TestDomainTag>(0, 1);
    EXPECT_TRUE(handle.entityId() == 0);
    EXPECT_TRUE(handle.versionId() == 1);
    EXPECT_TRUE(handle.strongId().value() == 0);
    EXPECT_TRUE(handle.isValid());

    handle = EntityHandle<TestDomainTag>(1, 1);
    EXPECT_TRUE(handle.isValid());
}