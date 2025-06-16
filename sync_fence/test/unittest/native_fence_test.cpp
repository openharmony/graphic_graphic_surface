/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "native_fence.h"

#include <chrono>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <linux/sync_file.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <thread>
#include <unistd.h>

namespace OHOS {
namespace {
    constexpr int INVALID_FD = -1;
    constexpr uint32_t TIMEOUT_MS = 5000;
} // namespace

class NativeFenceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/*
 * Function: NativeFenceWaitTest
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: open a valid fence fd.
 *                  2. operation: call OH_NativeFence_Wait with valid fence fd and timeout.
 *                  3. result: OH_NativeFence_Wait returns false because no event occurred after timeout.
 */
TEST_F(NativeFenceTest, NativeFenceWaitTest)
{
    // Test invalid fence fd
    bool result = OH_NativeFence_Wait(INVALID_FD, TIMEOUT_MS);
    EXPECT_FALSE(result);

    // Test valid fence fd
    int fd = open("/dev/GPIO_TEST", O_RDONLY);
    ASSERT_GE(fd, 0);
    bool result2 = false;
    result2 = OH_NativeFence_Wait(fd, 0);
    EXPECT_FALSE(result2);
    auto startTime = std::chrono::steady_clock::now();
    result2 = OH_NativeFence_Wait(fd, TIMEOUT_MS);
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    std::cout << "OH_NativeFence_Wait cost time:   " << duration << "ms" << std::endl;
    EXPECT_FALSE(result2);
    OH_NativeFence_Close(fd);
}

/*
 * Function: NativeFenceWaitWithSignalTest
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: create a valid fence fd by signalfd.
 *                  2. operation: waitThread call OH_NativeFence_Wait with valid fence fd and timeout. \n
 *                                mainThread call kill to send signal after 3 seconds.
 *                  3. result: OH_NativeFence_Wait should return true because has event occurred after 3 seconds.
 */
TEST_F(NativeFenceTest, NativeFenceWaitWithSignalTest)
{
    // Test invalid fence fd
    bool result = OH_NativeFence_Wait(INVALID_FD, TIMEOUT_MS);
    EXPECT_FALSE(result);

    std::atomic<bool> signaled(false);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); // Monitor SIGINT signal (Ctrl C)
    sigaddset(&mask, SIGTERM); // Monitor SIGTERM signal (kill command)
    sigprocmask(SIG_BLOCK, &mask, NULL);
    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd failed");
        exit(1);
    }

    std::thread waitThread([&]() {
        bool result2 = false;
        auto startTime = std::chrono::steady_clock::now();
        result2 = OH_NativeFence_Wait(sfd, TIMEOUT_MS);
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "OH_NativeFence_Wait cost time:   " << duration << "ms" << std::endl;
        EXPECT_TRUE(result2);
        signaled.store(true);
    });

    std::this_thread::sleep_for(std::chrono::seconds(3)); // 3 means main thread sleep 3 seconds.
    pid_t target_pid = getpid();
    int ret = kill(target_pid, SIGINT);
    if (ret < 0) {
        FAIL() << "kill failed: " << strerror(errno);
    }

    // Waiting for waitThread to complete
    waitThread.join();

    // checks the signaled variable to ensure that OH_NativeFence_Wait has returned
    EXPECT_TRUE(signaled.load());
    OH_NativeFence_Close(sfd);
}

/*
 * Function: NativeFenceIsValidTest
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: create a valid and invalid fence fd.
 *                  2. operation: call the OH_NativeFence_IsValid with a fence fd.
 *                  3. result: legitimate ID returns true, illegal ID returns false.
 */
TEST_F(NativeFenceTest, NativeFenceIsValidTest)
{
    // Test invalid fence fd
    bool result = OH_NativeFence_IsValid(INVALID_FD);
    EXPECT_FALSE(result);

    OH_NativeFence_Close(INVALID_FD);

    // Test valid fence fd
    int fd = open("/dev/GPIO_TEST", O_RDONLY);
    ASSERT_GE(fd, 0);
    result = OH_NativeFence_IsValid(fd);
    EXPECT_TRUE(result);
    OH_NativeFence_Close(fd);
}

/*
 * Function: NativeFenceLoopCallInterfaceTest
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: create a valid fence fd.
 *                  2. operation: call the OH_NativeFence_IsValid and OH_NativeFence_Close with 1000 times.
 *                  3. result: Interface execution without crash.
 */
TEST_F(NativeFenceTest, NativeFenceLoopCallInterfaceTest)
{
    auto start = std::chrono::high_resolution_clock::now();
    for (auto i = 0; i != 1000; i++) { // 1000 represents the number of cycles
        bool result = OH_NativeFence_IsValid(INVALID_FD);
        EXPECT_FALSE(result);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "OH_NativeFence_IsValid cost time:   " << duration.count() << "ms" << std::endl;
    ASSERT_GE(duration.count(), 0);

    start = std::chrono::high_resolution_clock::now();
    for (auto i = 0; i != 1000; i++) { // 1000 represents the number of cycles
        int fd = open("/dev/GPIO_TEST", O_RDONLY);
        ASSERT_GE(fd, 0);
        OH_NativeFence_Close(fd);
    }
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "OH_NativeFence_Close cost time:   " << duration.count() << "ms" << std::endl;
    ASSERT_GE(duration.count(), 0);
}

/*
 * Function: NativeFenceWaitForeverWithSignalTest
 * Type: Function
 * Rank: Important(2)
 * EnvConditions: N/A
 * CaseDescription: 1. preSetUp: create a valid fence fd by signalfd.
 *                  2. operation: waitThread call OH_NativeFence_WaitForever with valid fence fd and timeout. \n
 *                                mainThread call kill to send signal after 3 seconds.
 *                  3. result: OH_NativeFence_WaitForever should return true because has event occurred after 3 seconds.
 */
TEST_F(NativeFenceTest, NativeFenceWaitForeverWithSignalTest)
{
    // Test invalid fence fd
    bool result = OH_NativeFence_WaitForever(INVALID_FD);
    EXPECT_FALSE(result);

    std::atomic<bool> signaled(false);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT); // Monitor SIGINT signal (Ctrl C)
    sigaddset(&mask, SIGTERM); // Monitor SIGTERM signal (kill command)
    sigprocmask(SIG_BLOCK, &mask, NULL);

    int sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd failed");
        exit(1);
    }
    std::thread waitThread([&]() {
        bool result2 = false;
        result2 = OH_NativeFence_WaitForever(sfd);
        EXPECT_TRUE(result2);
        signaled.store(true);
    });
    std::this_thread::sleep_for(std::chrono::seconds(3)); // 3 means main thread sleep 3 seconds.
    pid_t target_pid = getpid();
    int ret = kill(target_pid, SIGINT);
    if (ret < 0) {
        FAIL() << "kill failed: " << strerror(errno);
    }

    // Waiting for waitThread to complete
    waitThread.join();

    // checks the signaled variable to ensure that OH_NativeFence_WaitForever has returned
    EXPECT_TRUE(signaled.load());
    OH_NativeFence_Close(sfd);
}
} // namespace OHOS 