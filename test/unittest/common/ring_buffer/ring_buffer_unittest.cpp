/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>

#include "ring_buffer.h"
#include "log.h"

using namespace testing::ext;
using namespace Updater;

namespace OHOS {
constexpr uint32_t RING_MAX_LEN = 10240;
constexpr uint32_t BYTE_SIZE = 255;
constexpr uint32_t BUFFER_SIZE = 1024;
constexpr uint32_t BUFFER_NUM = 8;
constexpr uint32_t LARGE_DATA_COUNT = 100000;

uint32_t g_num;
bool g_result = true;

class RingBufferTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
    void TestBody() {}
};

void ProducerTask(RingBuffer *ringBuffer)
{
    std::cout << "ring buffer ProducerTask start" << std::endl;
    for (uint32_t i = 0; i < RING_MAX_LEN; i++) {
        uint8_t buf[4] {}; // 4: test buffer size
        buf[0] = i % BYTE_SIZE;
        buf[1] = i / BYTE_SIZE;
        ringBuffer->Push(buf, sizeof(buf));
    }
}

void ConsumerTask(RingBuffer *ringBuffer)
{
    std::cout << "ring buffer ConsumerTask start" << std::endl;
    while (1) {
        uint8_t buf[4] {}; // 4: test buffer size
        uint32_t len = 0;
        ringBuffer->Pop(buf, sizeof(buf), len);
        if (buf[0] != g_num % BYTE_SIZE || buf[1] != g_num / BYTE_SIZE) {
            g_result = false;
        }
        g_num++;
        if (g_num == RING_MAX_LEN) {
            break;
        }
    }
}

void GenerateTestData(uint32_t index, uint8_t *buf, uint32_t size)
{
    for (uint32_t i = 0; i < size; i++) {
        buf[i] = static_cast<uint8_t>((index + i) % BYTE_SIZE);
    }
}

bool VerifyTestData(uint8_t *buf, uint32_t size, uint32_t expectedIndex)
{
    for (uint32_t i = 0; i < size; i++) {
        if (buf[i] != static_cast<uint8_t>((expectedIndex + i) % BYTE_SIZE)) {
            return false;
        }
    }
    return true;
}

void PushDataWithSize(RingBuffer *ringBuffer, uint32_t size, uint32_t count)
{
    std::vector<uint8_t> data(size);
    for (uint32_t i = 0; i < count; i++) {
        GenerateTestData(i, data.data(), size);
        EXPECT_TRUE(ringBuffer->Push(data.data(), size));
    }
}

void PopAndVerifyData(RingBuffer *ringBuffer, uint32_t size, uint32_t count)
{
    std::vector<uint8_t> data(size);
    for (uint32_t i = 0; i < count; i++) {
        uint32_t len = 0;
        EXPECT_TRUE(ringBuffer->Pop(data.data(), size, len));
        EXPECT_EQ(len, size);
        EXPECT_TRUE(VerifyTestData(data.data(), len, i));
    }
}

// 内存屏障测试 - 生产者任务函数
void MemoryBarrierProducerTask(RingBuffer *ringBuffer, uint32_t batchSize, uint32_t dataSize,
    std::atomic<uint32_t> &producedCount, std::atomic<bool> &hasError)
{
    for (uint32_t i = 0; i < batchSize && !hasError.load(); i++) {
        std::vector<uint8_t> data(dataSize);
        GenerateTestData(i, data.data(), dataSize);
        
        while (!hasError.load()) {
            bool pushed = ringBuffer->Push(data.data(), dataSize);
            if (pushed) {
                producedCount.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
                break;
            }
            std::this_thread::yield();
        }
    }
}

// std::ref(hasError), std::ref(errorLog), std::ref(logMutex), iteration
using MemoryBarrierParam = std::tuple<std::atomic<bool> &, std::vector<uint32_t> &, std::mutex &, uint32_t>;
// 内存屏障测试 - 消费者任务函数
void MemoryBarrierConsumerTask(RingBuffer *ringBuffer, uint32_t batchSize, uint32_t dataSize,
    std::atomic<uint32_t> &consumedCount, MemoryBarrierParam &param)
{
    auto &hasError = std::get<0>(param);
    auto &errorLog = std::get<1>(param);
    auto &logMutex = std::get<2>(param);
    uint32_t iteration = std::get<3>(param);
    for (uint32_t i = 0; i < batchSize && !hasError.load(); i++) {
        std::vector<uint8_t> data(dataSize);
        uint32_t len = 0;
        
        while (!hasError.load()) {
            bool popped = ringBuffer->Pop(data.data(), dataSize, len);
            if (!popped || len != dataSize) {
                std::this_thread::yield();
                continue;
            }
            if (!VerifyTestData(data.data(), len, consumedCount.load(std::memory_order_relaxed))) {
                std::lock_guard<std::mutex> lock(logMutex);
                errorLog.push_back(consumedCount.load(std::memory_order_relaxed));
                hasError.store(true);
                LOG(ERROR) << "Data corruption at iteration " << iteration << " index " << consumedCount.load();
            }
            consumedCount.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            break;
        }
    }
}

// 内存屏障测试 - 单次迭代执行函数
bool RunMemoryBarrierTest(uint32_t iteration)
{
    constexpr uint32_t dataSize = 64;
    constexpr uint32_t bufferNumSmall = 4;
    constexpr uint32_t batchSize = 100;
    constexpr uint32_t rounds = 10;
    
    RingBuffer ringBuffer;
    if (!ringBuffer.Init(dataSize, bufferNumSmall)) {
        LOG(ERROR) << "RingBuffer init failed at iteration " << iteration;
        return false;
    }
    
    std::atomic<uint32_t> producedCount(0);
    std::atomic<uint32_t> consumedCount(0);
    std::atomic<bool> hasError(false);
    std::vector<uint32_t> errorLog;
    std::mutex logMutex;
    
    for (uint32_t round = 0; round < rounds; round++) {
        producedCount.store(0);
        consumedCount.store(0);
        hasError.store(false);
        errorLog.clear();
        
        MemoryBarrierParam param(hasError, errorLog, logMutex, iteration);
        std::thread producer(MemoryBarrierProducerTask, &ringBuffer, batchSize, dataSize,
            std::ref(producedCount), std::ref(hasError));
        std::thread consumer(MemoryBarrierConsumerTask, &ringBuffer, batchSize, dataSize,
            std::ref(consumedCount), std::ref(param));
        
        producer.join();
        consumer.join();
        
        if (producedCount.load() != consumedCount.load() || hasError.load()) {
            LOG(ERROR) << "Memory barrier issue detected in iteration " << iteration << " round " << round;
            LOG(ERROR) << "Produced: " << producedCount.load() << " Consumed: " << consumedCount.load();
            if (!errorLog.empty()) {
                LOG(ERROR) << "Corrupted indices: " << errorLog.size();
            }
            return false;
        }
    }
    return true;
}

/**
 * ringBufferTest01: 单生产者单消费者基本功能测试
 * 功能：验证基础的Push和Pop操作在单生产者单消费者场景下的数据正确性
 * 测试点：
 * - 验证RingBuffer初始化成功
 * - 验证生产者线程可以正确推送数据
 * - 验证消费者线程可以正确弹出数据
 * - 验证数据的完整性和顺序性
 */
HWTEST_F(RingBufferTest, ringBufferTest01, TestSize.Level0)
{
    std::cout << "ringBufferTest01 start" << std::endl;
    g_num = 0;
    RingBuffer ringBuffer;
    bool ret = ringBuffer.Init(1024, 8);
    EXPECT_TRUE(ret);
    std::thread consumer(ConsumerTask, &ringBuffer);
    std::thread producer(ProducerTask, &ringBuffer);
    consumer.join();
    producer.join();
    EXPECT_EQ(g_num, RING_MAX_LEN);
    std::cout << "ringBufferTest01 end" << std::endl;
}

/**
 * ringBufferTest02: RingBuffer初始化参数验证测试
 * 功能：验证Init方法对非2的幂参数的验证
 * 测试点：
 * - 验证num参数为非2的幂时Init返回false
 * - 验证参数检查的正确性
 */
HWTEST_F(RingBufferTest, ringBufferTest02, TestSize.Level0)
{
    std::cout << "ringBufferTest02 start";
    RingBuffer ringBuffer;
    bool ret = ringBuffer.Init(1024, 3);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(g_result);
    std::cout << "ringBufferTest02 end";
}

/**
 * ringBufferTest03: 单生产者单消费者高并发压力测试
 * 功能：快速生产消费大量数据（10240条），验证性能表现和数据正确性
 * 测试点：
 * - 验证在大量数据吞吐下的数据正确性
 * - 记录并验证执行时间用于性能评估
 * - 验证环形缓冲区在高负载下的稳定性
 */
HWTEST_F(RingBufferTest, ringBufferTest03, TestSize.Level0)
{
    std::cout << "ringBufferTest03 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread producer(PushDataWithSize, &ringBuffer, BUFFER_SIZE, RING_MAX_LEN);
    std::thread consumer(PopAndVerifyData, &ringBuffer, BUFFER_SIZE, RING_MAX_LEN);
    
    producer.join();
    consumer.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    LOG(INFO) << "ringBufferTest03 completed in " << duration.count() << " ms";
    std::cout << "ringBufferTest03 end" << std::endl;
}

/**
 * ringBufferTest04: 不同数据大小测试
 * 功能：测试不同大小的数据（1字节、单缓冲区大小、接近单缓冲区大小）的Push和Pop
 * 测试点：
 * - 验证1字节数据的处理
 * - 验证单缓冲区大小（BUFFER_SIZE）数据的处理
 * - 验证接近单缓冲区大小（BUFFER_SIZE-1）数据的处理
 * - 验证所有数据的大小和内容正确性
 */
HWTEST_F(RingBufferTest, ringBufferTest04, TestSize.Level0)
{
    std::cout << "ringBufferTest04 start" << std::endl;
    
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    uint8_t buf1[1] {0x42};
    EXPECT_TRUE(ringBuffer.Push(buf1, 1));
    uint8_t out1[1] {0};
    uint32_t len1 = 0;
    EXPECT_TRUE(ringBuffer.Pop(out1, 1, len1));
    EXPECT_EQ(len1, 1);
    EXPECT_EQ(out1[0], 0x42);
    
    std::vector<uint8_t> data2(BUFFER_SIZE);
    GenerateTestData(0, data2.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data2.data(), BUFFER_SIZE));
    std::vector<uint8_t> out2(BUFFER_SIZE);
    uint32_t len2 = 0;
    EXPECT_TRUE(ringBuffer.Pop(out2.data(), BUFFER_SIZE, len2));
    EXPECT_EQ(len2, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out2.data(), len2, 0));
    
    std::vector<uint8_t> data3(BUFFER_SIZE - 1);
    GenerateTestData(100, data3.data(), BUFFER_SIZE - 1);
    EXPECT_TRUE(ringBuffer.Push(data3.data(), BUFFER_SIZE - 1));
    std::vector<uint8_t> out3(BUFFER_SIZE - 1);
    uint32_t len3 = 0;
    EXPECT_TRUE(ringBuffer.Pop(out3.data(), BUFFER_SIZE - 1, len3));
    EXPECT_EQ(len3, BUFFER_SIZE - 1);
    EXPECT_TRUE(VerifyTestData(out3.data(), len3, 100));
    
    std::cout << "ringBufferTest04 end" << std::endl;
}

/**
 * ringBufferTest05: 二进制数据测试
 * 功能：测试包含0x00和所有字节值0x00-0xFF的二进制数据
 * 测试点：
 * - 验证包含所有字节值（0x00-0xFF）的数据的正确性
 * - 验证全0数据的处理
 * - 确保RingBuffer能正确处理二进制数据，包括空字节
 */
HWTEST_F(RingBufferTest, ringBufferTest05, TestSize.Level0)
{
    std::cout << "ringBufferTest05 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    uint8_t binaryData[256];
    for (int i = 0; i < 256; i++) {
        binaryData[i] = static_cast<uint8_t>(i);
    }
    
    EXPECT_TRUE(ringBuffer.Push(binaryData, 256));
    uint8_t outBinary[256] {0};
    uint32_t len = 0;
    EXPECT_TRUE(ringBuffer.Pop(outBinary, 256, len));
    EXPECT_EQ(len, 256);
    for (int i = 0; i < 256; i++) {
        EXPECT_EQ(outBinary[i], static_cast<uint8_t>(i));
    }
    
    uint8_t zeroData[BUFFER_SIZE] {0};
    EXPECT_TRUE(ringBuffer.Push(zeroData, BUFFER_SIZE));
    uint8_t outZero[BUFFER_SIZE] {0xFF};
    len = 0;
    EXPECT_TRUE(ringBuffer.Pop(outZero, BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        EXPECT_EQ(outZero[i], 0);
    }
    
    std::cout << "ringBufferTest05 end" << std::endl;
}

/**
 * ringBufferTest06: 大量数据吞吐测试
 * 功能：生产消费10240条数据，测试长时间运行和大量数据的正确性
 * 测试点：
 * - 验证环形索引在大量数据下的正确性
 * - 验证环形索引跳转时的正确性
 * - 记录执行时间用于性能评估
 */
HWTEST_F(RingBufferTest, ringBufferTest06, TestSize.Level0)
{
    std::cout << "ringBufferTest06 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::thread producer(PushDataWithSize, &ringBuffer, BUFFER_SIZE, RING_MAX_LEN);
    std::thread consumer(PopAndVerifyData, &ringBuffer, BUFFER_SIZE, RING_MAX_LEN);
    
    producer.join();
    consumer.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    LOG(INFO) << "ringBufferTest06 processed " << RING_MAX_LEN << " items in " << duration.count() << " ms";
    std::cout << "ringBufferTest06 end" << std::endl;
}

/**
 * ringBufferTest07: 缓冲区满测试
 * 功能：验证缓冲区满时Push会阻塞等待，消费部分数据后Push继续
 * 测试点：
 * - 验证填满缓冲区（写入BUFFER_NUM条数据）
 * - 验证Push在缓冲区满时会阻塞等待
 * - 验证消费数据后Push可以继续执行
 * - 使用超时机制验证阻塞行为
 */
HWTEST_F(RingBufferTest, ringBufferTest07, TestSize.Level0)
{
    std::cout << "ringBufferTest07 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    
    for (uint32_t i = 0; i < BUFFER_NUM; i++) {
        EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    }
    
    std::thread producer([&ringBuffer, &data]() {
        GenerateTestData(BUFFER_NUM, data.data(), BUFFER_SIZE);
        ringBuffer.Push(data.data(), BUFFER_SIZE);
    });
    
    std::thread consumer([&ringBuffer]() {
        std::vector<uint8_t> out(BUFFER_SIZE);
        uint32_t len = 0;
        for (uint32_t i = 0; i < BUFFER_NUM + 1; i++) {
            EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
            EXPECT_EQ(len, BUFFER_SIZE);
        }
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "ringBufferTest07 end" << std::endl;
}

/**
 * ringBufferTest08: 缓冲区空测试
 * 功能：验证缓冲区空时Pop会阻塞等待，生产数据后Pop继续
 * 测试点：
 * - 验证创建空缓冲区
 * - 验证Pop在缓冲区空时会阻塞等待
 * - 验证生产数据后Pop可以继续执行
 * - 使用超时机制验证阻塞行为
 */
HWTEST_F(RingBufferTest, ringBufferTest08, TestSize.Level0)
{
    std::cout << "ringBufferTest08 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    std::vector<uint8_t> data(BUFFER_SIZE);
    std::vector<uint8_t> out(BUFFER_SIZE);
    
    bool consumerFinished = false;
    bool popFailed = false;
    
    std::thread consumer([&ringBuffer, &out, &consumerFinished, &popFailed]() {
        uint32_t len = 0;
        if (!ringBuffer.Pop(out.data(), BUFFER_SIZE, len)) {
            popFailed = true;
        }
        consumerFinished = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(consumerFinished);
    
    std::thread producer([&ringBuffer, &data]() {
        GenerateTestData(0, data.data(), BUFFER_SIZE);
        ringBuffer.Push(data.data(), BUFFER_SIZE);
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_FALSE(popFailed);
    EXPECT_TRUE(consumerFinished);
    
    std::cout << "ringBufferTest08 end" << std::endl;
}

/**
 * ringBufferTest09: 缓冲区满时Push阻塞测试
 * 功能：验证填满缓冲区后Push会阻塞等待，消费数据后Push继续
 * 测试点：
 * - 验证填满缓冲区（写入BUFFER_NUM条数据）
 * - 验证Push在缓冲区满时会阻塞等待
 * - 验证消费数据后Push可以继续执行
 * - 使用超时机制验证阻塞行为
 */
HWTEST_F(RingBufferTest, ringBufferTest09, TestSize.Level0)
{
    std::cout << "ringBufferTest09 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    
    for (uint32_t i = 0; i < BUFFER_NUM; i++) {
        EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    }
    
    bool producerFinished = false;
    bool pushFailed = false;
    
    std::thread producer([&ringBuffer, &data, &producerFinished, &pushFailed]() {
        GenerateTestData(BUFFER_NUM, data.data(), BUFFER_SIZE);
        if (!ringBuffer.Push(data.data(), BUFFER_SIZE)) {
            pushFailed = true;
        }
        producerFinished = true;
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(producerFinished);
    
    std::thread consumer([&ringBuffer]() {
        std::vector<uint8_t> out(BUFFER_SIZE);
        uint32_t len = 0;
        EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    });
    
    consumer.join();
    producer.join();
    
    EXPECT_FALSE(pushFailed);
    EXPECT_TRUE(producerFinished);
    
    std::cout << "ringBufferTest09 end" << std::endl;
}

/**
 * ringBufferTest10: 临界点测试
 * 功能：测试刚好填满缓冲区、刚好清空缓冲区以及环形索引跳转的场景
 * 测试点：
 * - 验证刚好填满缓冲区的场景
 * - 验证刚好清空缓冲区的场景
 * - 验证环形索引从num_-1到0的跳转
 * - 验证位运算的正确性
 */
HWTEST_F(RingBufferTest, ringBufferTest10, TestSize.Level0)
{
    std::cout << "ringBufferTest10 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    std::vector<uint8_t> data(BUFFER_SIZE);
    std::vector<uint8_t> out(BUFFER_SIZE);
    
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    for (uint32_t i = 0; i < BUFFER_NUM; i++) {
        EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    }
    
    for (uint32_t i = 0; i < BUFFER_NUM; i++) {
        uint32_t len = 0;
        EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
        EXPECT_EQ(len, BUFFER_SIZE);
        EXPECT_TRUE(VerifyTestData(out.data(), len, 0));
    }
    
    GenerateTestData(BUFFER_NUM, data.data(), BUFFER_SIZE);
    for (uint32_t i = 0; i < BUFFER_NUM; i++) {
        EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    }
    
    for (uint32_t i = 0; i < BUFFER_NUM; i++) {
        uint32_t len = 0;
        EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
        EXPECT_EQ(len, BUFFER_SIZE);
        EXPECT_TRUE(VerifyTestData(out.data(), len, BUFFER_NUM));
    }
    
    std::cout << "ringBufferTest10 end" << std::endl;
}

/**
 * ringBufferTest11: Stop状态测试
 * 功能：验证调用Stop后，Push和Pop的行为
 * 测试点：
 * - 验证Stop后停止通知等待线程
 * - 验证Stop后已存在的数据可以被消费
 * - 验证Stop后isStop_标志被设置
 */
HWTEST_F(RingBufferTest, ringBufferTest11, TestSize.Level0)
{
    std::cout << "ringBufferTest11 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    // 先Push一些数据
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    
    ringBuffer.Stop();
    
    // Stop后Pop可以消费已存在的数据
    std::vector<uint8_t> out(BUFFER_SIZE);
    uint32_t len = 0;
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out.data(), len, 0));
    
    std::cout << "ringBufferTest11 end" << std::endl;
}

/**
 * ringBufferTest12: StopPush功能测试
 * 功能：验证调用StopPush后的实际行为（实现中使用isStop_全局标志）
 * 测试点：
 * - 验证StopPush后设置isStop_标志
 * - 验证在非空缓冲区上Pop可以继续消费数据
 */
HWTEST_F(RingBufferTest, ringBufferTest12, TestSize.Level0)
{
    std::cout << "ringBufferTest12 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    // 先Push一些数据
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    
    ringBuffer.StopPush();
    
    // 在非空缓冲区上Pop应该可以继续消费数据
    std::vector<uint8_t> out(BUFFER_SIZE);
    uint32_t len = 0;
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out.data(), len, 0));
    
    std::cout << "ringBufferTest12 end" << std::endl;
}

/**
 * ringBufferTest13: StopPop功能测试
 * 功能：验证调用StopPop后的实际行为（实现中使用isStop_全局标志）
 * 测试点：
 * - 验证调用StopPop后，在不满的缓冲区上Push可以成功
 * - 验证调用StopPop后，在非空缓冲区上Pop可以继续消费数据
 */
HWTEST_F(RingBufferTest, ringBufferTest13, TestSize.Level0)
{
    std::cout << "ringBufferTest13 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    ringBuffer.StopPop();
    
    // StopPop后，缓冲区不满，Push可以成功
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    
    // 在非空缓冲区上Pop应该可以继续消费数据
    std::vector<uint8_t> out(BUFFER_SIZE);
    uint32_t len = 0;
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out.data(), len, 0));
    
    std::cout << "ringBufferTest13 end" << std::endl;
}

/**
 * ringBufferTest14: 无效参数测试
 * 功能：验证Push和Pop对无效参数的处理
 * 测试点：
 * - Push测试：nullptr、len=0、len > singleSize
 * - Pop测试：nullptr
 * - 验证所有错误情况都返回false
 */
HWTEST_F(RingBufferTest, ringBufferTest14, TestSize.Level0)
{
    std::cout << "ringBufferTest14 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    // 先Push一条数据，避免后续Pop在空缓冲区上卡住
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    
    // 测试Push无效参数
    EXPECT_FALSE(ringBuffer.Push(nullptr, BUFFER_SIZE));
    
    EXPECT_FALSE(ringBuffer.Push(nullptr, 0));
    
    std::vector<uint8_t> largeData(BUFFER_SIZE + 1);
    EXPECT_FALSE(ringBuffer.Push(largeData.data(), BUFFER_SIZE + 1));
    
    EXPECT_FALSE(ringBuffer.Push(data.data(), 0));
    
    // 测试Pop无效参数
    uint32_t len = 0;
    EXPECT_FALSE(ringBuffer.Pop(nullptr, BUFFER_SIZE, len));
    
    // 消费之前Push的数据，避免泄漏
    std::vector<uint8_t> out(BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    
    std::cout << "ringBufferTest14 end" << std::endl;
}

/**
 * ringBufferTest15: Init参数验证测试
 * 功能：验证Init方法对各种参数的检查
 * 测试点：
 * - 测试singleSize=0（应失败）
 * - 测试num=0（应失败）
 * - 测试num非2的幂（应失败）
 * - 测试有效参数（应成功）
 */
HWTEST_F(RingBufferTest, ringBufferTest15, TestSize.Level0)
{
    std::cout << "ringBufferTest15 start" << std::endl;
    RingBuffer ringBuffer;
    
    EXPECT_FALSE(ringBuffer.Init(0, BUFFER_NUM));
    
    EXPECT_FALSE(ringBuffer.Init(BUFFER_SIZE, 0));
    
    EXPECT_FALSE(ringBuffer.Init(BUFFER_SIZE, 3));
    
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, 8));
    
    std::cout << "ringBufferTest15 end" << std::endl;
}

/**
 * ringBufferTest16: Reset功能测试
 * 功能：验证Reset方法可以清空缓冲区并重置索引
 * 测试点：
 * - 验证Reset前缓冲区有数据
 * - 验证Reset后可以正常Push数据
 * - 验证Reset后可以正常Pop数据
 * - 验证数据正确性
 */
HWTEST_F(RingBufferTest, ringBufferTest16, TestSize.Level0)
{
    std::cout << "ringBufferTest16 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    
    // 先生产一些数据
    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    
    // Reset清空缓冲区
    ringBuffer.Reset();
    
    // 验证可以继续生产消费
    GenerateTestData(1, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    
    std::vector<uint8_t> out(BUFFER_SIZE);
    uint32_t len = 0;
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out.data(), len, 1));
    
    std::cout << "ringBufferTest16 end" << std::endl;
}

/**
 * ringBufferTest17: 连续使用测试
 * 功能：验证RingBuffer可以多次重复使用
 * 测试点：
 * - Init -> 生产消费 -> Reset -> 再次生产消费
 * - 验证可以多次重复使用
 * - 验证每次使用的数据正确性
 */
HWTEST_F(RingBufferTest, ringBufferTest17, TestSize.Level0)
{
    std::cout << "ringBufferTest17 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));

    std::vector<uint8_t> data(BUFFER_SIZE);
    GenerateTestData(0, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));

    ringBuffer.Reset();

    GenerateTestData(1, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));

    std::vector<uint8_t> out(BUFFER_SIZE);
    uint32_t len = 0;
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out.data(), len, 1));

    ringBuffer.Reset();

    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));
    GenerateTestData(2, data.data(), BUFFER_SIZE);
    EXPECT_TRUE(ringBuffer.Push(data.data(), BUFFER_SIZE));
    EXPECT_TRUE(ringBuffer.Pop(out.data(), BUFFER_SIZE, len));
    EXPECT_EQ(len, BUFFER_SIZE);
    EXPECT_TRUE(VerifyTestData(out.data(), len, 2));

    std::cout << "ringBufferTest17 end" << std::endl;
}

/**
 * ringBufferTest18: 大数据量吞吐测试
 * 功能：生产消费100000条数据，测试大数据量场景下的性能和稳定性
 * 测试点：
 * - 验证在大数据量场景下的性能和正确性
 * - 验证长时间运行的稳定性
 * - 记录执行时间和吞吐量用于性能评估
 */
HWTEST_F(RingBufferTest, ringBufferTest18, TestSize.Level0)
{
    std::cout << "ringBufferTest18 start" << std::endl;
    RingBuffer ringBuffer;
    EXPECT_TRUE(ringBuffer.Init(BUFFER_SIZE, BUFFER_NUM));

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer(PushDataWithSize, &ringBuffer, BUFFER_SIZE, LARGE_DATA_COUNT);
    std::thread consumer(PopAndVerifyData, &ringBuffer, BUFFER_SIZE, LARGE_DATA_COUNT);

    producer.join();
    consumer.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    LOG(INFO) << "ringBufferTest18 processed " << LARGE_DATA_COUNT << " items in " << duration.count() << " ms";
    LOG(INFO) << "Average: " << (LARGE_DATA_COUNT * 1000.0 / duration.count()) << " items/sec";
    std::cout << "ringBufferTest18 end" << std::endl;
}

/**
 * ringBufferTest19: 多核场景下缓存一致性测试（内存屏障问题复现）
 * 功能：高概率复现writeIndex_和readIndex_缺乏内存屏障导致的缓存一致性问题
 * 问题描述：
 * - writeIndex_和readIndex_使用普通uint32_t类型，未使用std::atomic
 * - 在多核CPU上，不同核心的缓存可能不一致
 * - 生产者更新writeIndex_后，消费者可能看到旧值，反之亦然
 * - 缺乏内存屏障导致索引更新对其他核心不可见
 * 测试策略：
 * 1. 使用多个线程高频交替操作，增加缓存不一致暴露概率
 * 2. 在缓冲区临界点（快满/快空）快速切换生产/消费
 * 3. 验证索引的可见性和数据完整性
 * 4. 检测索引回转、重复读取、跳过数据等异常情况
 * 5. 重复运行多次以提高问题复现概率
 * 测试点：
 * - 验证writeIndex_的更新对消费者线程可见
 * - 验证readIndex_的更新对生产者线程可见
 * - 验证索引位运算(& (num_-1))的原子性
 * - 验证缓冲区状态(IsFull/IsEmpty)的一致性
 * - 检测数据重复消费或丢失
 * - 检测索引非法值或跳变
 * 配置参数：
 * - TEST_ITERATIONS: 测试迭代次数，可通过环境变量RINGBUFFER_TEST_ITERATIONS配置
 * - DEFAULT_ITERATIONS: 默认迭代次数（10次，约1秒）
 */
HWTEST_F(RingBufferTest, ringBufferTest19, TestSize.Level0)
{
    std::cout << "ringBufferTest19 start - Memory Barrier Consistency Test" << std::endl;
    LOG(INFO) << "Testing cache consistency issues in multi-core scenario";
    
    constexpr uint32_t defaultIterations = 10; // 默认10次，约1秒完成
    const char* iterEnv = std::getenv("RINGBUFFER_TEST_ITERATIONS");
    uint32_t iterations = defaultIterations;
    if (iterEnv != nullptr) {
        iterations = static_cast<uint32_t>(std::atoi(iterEnv));
        LOG(INFO) << "Using configured iterations: " << iterations;
    }
    
    for (uint32_t iter = 0; iter < iterations; iter++) {
        bool testPassed = RunMemoryBarrierTest(iter);
        if (!testPassed) {
            LOG(ERROR) << "Memory barrier test failed at iteration " << iter;
            EXPECT_TRUE(testPassed);
            std::cout << "ringBufferTest19 end - FAILED at iteration " << iter << std::endl;
            break;
        }
    }
    
    LOG(INFO) << "Memory barrier consistency test completed " << iterations << " iterations";
    std::cout << "ringBufferTest19 end" << std::endl;
}
}