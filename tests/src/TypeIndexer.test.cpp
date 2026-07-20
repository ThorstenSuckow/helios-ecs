#include <gtest/gtest.h>
#include "helios-ecs-config.h"


import helios.ecs;

using namespace helios::ecs;


namespace {

struct TestDomainTag {};


} // namespace


TEST(TypeIndexer, find) {

    struct Foo{};
    struct Bar{};

    auto typeId1 = TypeIndexer<TestDomainTag>::typeIndex<Foo>();
    auto typeId2 = TypeIndexer<TestDomainTag>::typeIndex<Bar>();
    auto typeId3 = TypeIndexer<TestDomainTag>::typeIndex<Bar>();
    auto typeId4 = TypeIndexer<TestDomainTag>::typeIndex<Foo>();

    EXPECT_EQ(typeId1, 0);
    EXPECT_EQ(typeId2, 1);
    EXPECT_EQ(typeId3, 1);
    EXPECT_EQ(typeId4, 0);

}