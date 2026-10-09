#include <gtest/gtest.h>
#include <atomic>
#include <future>
#include "helios-ecs-config.h"


import helios.ecs;
import helios.core;

namespace {

    struct TestDomainTag {};

    auto s1NonAtomic = 0;
    auto s2NonAtomic = 0;
    auto s3NonAtomic = 0;

    auto s1Atomic = std::atomic<int>{0};
    auto s2Atomic = std::atomic<int>{0};
    auto s3Atomic = std::atomic<int>{0};

} // namespace


TEST(SequentialScheduler, find) {

    auto ecsDataContainer =
        helios::ecs::common::container::EcsDataContainer{};

    ecsDataContainer.emplace<helios::core::thread::ThreadPool>(2);
    auto scheduler =
        helios::ecs::scheduling::Scheduler{};

    int s1State = 0;
    int s2State = 0;
    int s3State = 0;

    auto s1 = [&]() {
        EXPECT_EQ(s1State, 0);
        EXPECT_EQ(s2State, 0);
        EXPECT_EQ(s3State, 0);

        ++s1State;
    };

    auto s2 = [&]() {
        EXPECT_EQ(s1State, 1);
        EXPECT_EQ(s2State, 0);
        EXPECT_EQ(s3State, 0);

        ++s2State;
    };

    auto s3 = [&]() {
        EXPECT_EQ(s1State, 1);
        EXPECT_EQ(s2State, 1);
        EXPECT_EQ(s3State, 0);

        ++s3State;
    };

    auto s4 = [&]() {
        EXPECT_EQ(s1State, 1);
        EXPECT_EQ(s2State, 1);
        EXPECT_EQ(s3State, 1);

        s1State = 0;
        s2State = 0;
        s3State = 0;
    };

    scheduler.init(ecsDataContainer);

    scheduler.beginSchedule()
        .add(
            helios::ecs::system::Sequential(s1, s2, s3, s4)
        )
        .endSchedule();

    for (int i = 0; i < 100; ++i) {
        scheduler.update();
    }

    EXPECT_EQ(s1State, 0);
    EXPECT_EQ(s2State, 0);
    EXPECT_EQ(s3State, 0);
}

TEST(Scheduler, ExecutesSequentialBranchesInParallel) {

    auto ecsDataContainer =
       helios::ecs::common::container::EcsDataContainer{};

    ecsDataContainer.emplace<helios::core::thread::ThreadPool>(2);
    auto scheduler =
        helios::ecs::scheduling::Scheduler{};

    int leftState = 0;
    int rightState = 0;

    bool leftSequential = true;
    bool rightSequential = true;

    std::atomic<bool> ranConcurrently{true};

    std::promise<void> leftStartedPromise;
    std::promise<void> rightStartedPromise;

    const auto leftStarted  = leftStartedPromise.get_future();
    const auto rightStarted = rightStartedPromise.get_future();

    auto s1 = [&]() {
        if (leftState != 0) {
            leftSequential = false;
        }

        leftState = 1;
        // signal right state
        leftStartedPromise.set_value();

        // The other branch must be able to start before this system completes.
        if (rightStarted.wait_for(std::chrono::seconds(1))
            == std::future_status::timeout) {
            ranConcurrently = false;
            }
    };

    auto s2 = [&]() {
        // Systems within the left branch must execute sequentially.
        if (leftState != 1) {
            leftSequential = false;
        }

        leftState = 2;
    };

    auto s3 = [&]() {
        if (rightState != 0) {
            rightSequential = false;
        }

        rightState = 1;
        // signal left branch
        rightStartedPromise.set_value();

        // The other branch must be able to start before this system completes.
        if (leftStarted.wait_for(std::chrono::seconds(1))
            == std::future_status::timeout) {
            ranConcurrently = false;
            }
    };

    auto s4 = [&]() {
        // Systems within the right branch must execute sequentially.
        if (rightState != 1) {
            rightSequential = false;
        }

        rightState = 2;
    };

    // Both Sequential groups form independent branches of the same
    // parallel execution group. Systems within each branch remain ordered.
    scheduler.beginSchedule()
        .add(
            helios::ecs::system::Sequential(s1, s2),
            helios::ecs::system::Sequential(s3, s4)
        )
        .endSchedule();


    scheduler.init(ecsDataContainer);

    scheduler.update();

    // Verify the sequential order within each branch and that the
    // two branches were active concurrently.
    EXPECT_TRUE(leftSequential);
    EXPECT_TRUE(rightSequential);
    EXPECT_TRUE(ranConcurrently);

    EXPECT_EQ(leftState, 2);
    EXPECT_EQ(rightState, 2);
}