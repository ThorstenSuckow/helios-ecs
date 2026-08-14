#include <gtest/gtest.h>
#include "helios-ecs-config.h"

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;


namespace {

    struct TestDomainTag {};
    using TestHandle = EntityHandle<TestDomainTag>;
};

TEST(EntityHandle, create) {

    auto handle = TestHandle();

    EXPECT_TRUE(handle.entityId() == 0);
    EXPECT_TRUE(handle.versionId() == InvalidVersion);
    EXPECT_FALSE(handle.isValid());

#ifdef HELIOS_DEBUG
    EXPECT_DEATH(TestHandle(0, 0), "EntityHandle must not be constructed with InvalidVersion.");
#endif

#ifdef HELIOS_DEBUG
    EXPECT_DEATH(TestHandle(1, 0), "EntityHandle must not be constructed with InvalidVersion.");
#endif

    handle = TestHandle(0, 1);
    EXPECT_TRUE(handle.entityId() == 0);
    EXPECT_TRUE(handle.versionId() == 1);
    EXPECT_TRUE(handle.isValid());

    handle = TestHandle(1, 1);
    EXPECT_TRUE(handle.isValid());
}