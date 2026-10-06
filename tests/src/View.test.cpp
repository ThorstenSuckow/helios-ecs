#include <gtest/gtest.h>
#include "helios-ecs-config.h"


import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::common::types;
using namespace helios::ecs::components;
using namespace helios::ecs::entity;

// Wrapped in an anonymous namespace so these test-local helper types get
// internal linkage. Without this, other test TUs that declare identically
// named helpers (e.g. `MyComponent`, `TestDomainTag`) at global scope would
// share the same mangled names for template instantiations, causing the
// linker to fold differing definitions together (an ODR violation) and
// corrupting `SparseSet`/`Query` iteration at runtime.
namespace {

struct TestDomainTag {};
using TestHandle = EntityHandle<TestDomainTag>;
using TestRegistry = EntityRegistry<TestDomainTag>;
using TestEntityManager = EntityManager<TestHandle>;

using ViewWorld = TypedHandleWorld<TestEntityManager>;



class MyComponent {

    public:


    int value = 0;
    bool remove = true;

    bool onRemove() {
        return remove;
    }

    void setValue() {}

};

} // namespace


TEST(Query, find) {

    TestEntityManager em{};

    const auto handle = em.create();
    EXPECT_EQ(handle.entityId(), 0);
    EXPECT_EQ(handle.versionId(), 1);

    std::ignore = em.ensureSparseSet<MyComponent>();

    EXPECT_FALSE(em.has<MyComponent>(handle));
    EXPECT_FALSE(em.sparseSet<MyComponent>()->isDirty(handle.entityId()));

    auto* cmp = em.emplace<MyComponent>(handle, 10);
    EXPECT_TRUE(em.has<MyComponent>(handle));

    auto query = query::Query<TestHandle, ReadSet<MyComponent>, WriteSet<MyComponent>>(&em);


    cmp->setValue();
    em.sparseSet<MyComponent>()->markDirty(handle.entityId());

    EXPECT_TRUE(em.sparseSet<MyComponent>()->isDirty(handle.entityId()));

    em.clearDirtySet<MyComponent>();

    EXPECT_FALSE(em.sparseSet<MyComponent>()->isDirty(handle.entityId()));



}