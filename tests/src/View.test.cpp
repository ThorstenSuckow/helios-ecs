#include <gtest/gtest.h>
#include "helios-ecs-config.h"


import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::types;

// Wrapped in an anonymous namespace so these test-local helper types get
// internal linkage. Without this, other test TUs that declare identically
// named helpers (e.g. `MyComponent`, `TestDomainTag`) at global scope would
// share the same mangled names for template instantiations, causing the
// linker to fold differing definitions together (an ODR violation) and
// corrupting `SparseSet`/`View` iteration at runtime.
namespace {

struct TestDomainTag {};
using TestHandle = EntityHandle<TestDomainTag>;
using TestRegistry = EntityRegistry<TestDomainTag>;
using TestEntityManager = EntityManager<TestHandle, TestRegistry, DEFAULT_ENTITY_MANAGER_CAPACITY>;
using TestReflector = ComponentReflector<TestEntityManager>;

using ViewWorld = TypedHandleWorld<TestEntityManager>;


class MyComponent {

    public:
    int value = 0;
    bool remove = true;

    unsigned int currentVersion = 0;
    unsigned int previousVersion = 0;

    bool onRemove() {
        return remove;
    }

    void setValue() {
        currentVersion++;
    }
    bool hasChanges() const noexcept {
        return currentVersion != previousVersion;
    }

    void commit() {
        previousVersion = currentVersion;
    }


};

} // namespace


TEST(View, find) {

    TestReflector::registerType<MyComponent>();

    TestEntityManager em{};

    const auto handle = em.create();
    EXPECT_EQ(handle.entityId, 0);
    EXPECT_EQ(handle.versionId, 1);

    EXPECT_FALSE(em.has<MyComponent>(handle));

    auto* cmp = em.emplace<MyComponent>(handle, 10);
    EXPECT_TRUE(em.has<MyComponent>(handle));

    auto view = View<TestEntityManager, MyComponent>(&em);

    int i = 0;
    for (auto [entity, component] : view.whereEnabled().whereAnyChanged()) {
        i++;
    }
    EXPECT_EQ(i, 0);

    cmp->setValue();
    i = 0;
    for (auto [entity, component] : view.whereEnabled().whereAnyChanged()) {
        i++;
    }
    EXPECT_EQ(i, 1);

    cmp->commit();

    i = 0;
    for (auto [entity, component] : view.whereEnabled().whereAnyChanged()) {
        i++;
    }
    EXPECT_EQ(i, 0);


}