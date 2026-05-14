#include <benchmark/benchmark.h>

#include <cstddef>
#include <vector>

import helios.ecs;

using namespace helios::ecs;
using namespace helios::ecs::types;

struct BenchmarkDomainTag {};
using BenchmarkHandle = EntityHandle<BenchmarkDomainTag>;
using BenchmarkRegistry = EntityRegistry<BenchmarkDomainTag>;
using BenchmarkEntityManager = EntityManager<BenchmarkHandle, BenchmarkRegistry, 10000>;
using BenchmarkReflector = ComponentReflector<BenchmarkEntityManager>;

struct Position {
    float x{0.0F};
    float y{0.0F};
};

struct Velocity {
    float x{0.0F};
    float y{0.0F};
};

struct Health {
    int hp{100};
};

struct MissingTag {
    int value{0};
};

struct EcsFixture {
    BenchmarkRegistry registry;
    BenchmarkEntityManager em;
    std::vector<BenchmarkHandle> entities;

    explicit EcsFixture(const std::size_t count)
        : registry(count), em(registry, count) {
        BenchmarkReflector::registerType<Position>();
        BenchmarkReflector::registerType<Velocity>();
        BenchmarkReflector::registerType<Health>();
        BenchmarkReflector::registerType<MissingTag>();

        entities.reserve(count);

        for (std::size_t i = 0; i < count; ++i) {
            const auto handle = em.create();
            entities.push_back(handle);

            em.emplace<Position>(handle, Position{static_cast<float>(i), static_cast<float>(i)});
            if (i % 2U == 0U) {
                em.emplace<Velocity>(handle, Velocity{1.0F, 1.0F});
            }
            if (i % 3U == 0U) {
                em.emplace<Health>(handle, Health{100});
            }
        }
    }
};

static void BM_ECS_HasVelocity(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    EcsFixture fixture(count);

    for (auto _ : state) {
        std::size_t found = 0;
        for (const auto handle : fixture.entities) {
            if (fixture.em.has<Velocity>(handle)) {
                ++found;
            }
        }
        benchmark::DoNotOptimize(found);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(count));
}

BENCHMARK(BM_ECS_HasVelocity)
    ->Arg(100)
    ->Arg(200)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(2000)
    ->Arg(5000)
    ->Arg(10000)
    ->Arg(20000)
    ->Arg(50000)
    ->Unit(benchmark::kMicrosecond);

static void BM_ECS_GetVelocity(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    EcsFixture fixture(count);

    for (auto _ : state) {
        std::size_t found = 0;
        for (const auto handle : fixture.entities) {
            if (fixture.em.get<Velocity>(handle) != nullptr) {
                ++found;
            }
        }
        benchmark::DoNotOptimize(found);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(count));
}

BENCHMARK(BM_ECS_GetVelocity)
    ->Arg(100)
    ->Arg(200)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(2000)
    ->Arg(5000)
    ->Arg(10000)
    ->Arg(20000)
    ->Arg(50000)
    ->Unit(benchmark::kMicrosecond);

static void BM_ECS_MultiGet(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    EcsFixture fixture(count);

    for (auto _ : state) {
        std::size_t matched = 0;
        for (const auto handle : fixture.entities) {
            const auto* pos = fixture.em.get<Position>(handle);
            const auto* vel = fixture.em.get<Velocity>(handle);
            const auto* hp = fixture.em.get<Health>(handle);

            if (pos && vel && hp) {
                ++matched;
            }
        }
        benchmark::DoNotOptimize(matched);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(count));
}

BENCHMARK(BM_ECS_MultiGet)
    ->Arg(100)
    ->Arg(200)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(2000)
    ->Arg(5000)
    ->Arg(10000)
    ->Arg(20000)
    ->Arg(50000)
    ->Unit(benchmark::kMicrosecond);

static void BM_ECS_HasMissing(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    EcsFixture fixture(count);

    for (auto _ : state) {
        std::size_t found = 0;
        for (const auto handle : fixture.entities) {
            if (fixture.em.has<MissingTag>(handle)) {
                ++found;
            }
        }
        benchmark::DoNotOptimize(found);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(count));
}

BENCHMARK(BM_ECS_HasMissing)
    ->Arg(100)
    ->Arg(200)
    ->Arg(500)
    ->Arg(1000)
    ->Arg(2000)
    ->Arg(5000)
    ->Arg(10000)
    ->Arg(20000)
    ->Arg(50000)
    ->Unit(benchmark::kMicrosecond);


BENCHMARK_MAIN();

