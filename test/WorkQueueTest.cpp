#include <catch2/catch_test_macros.hpp>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_set>
#include <vector>

#include "../src/WorkQueue.h"

using namespace std;
using namespace dxpool;

TEST_CASE("Work queue") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,readability-function-cognitive-complexity)
    SECTION("Single thread, one task") {
        const int updatedValue = 944;
        int updatable{0};

        auto task = [&updatable]() -> void {
            updatable = updatedValue;
        };


        WorkQueue queue;
        queue.Add(task);

        REQUIRE(queue.HasWork());

        auto dequeuedTask = queue.Take();

        // so far our task should have not been executed
        REQUIRE(updatable==0);
        dequeuedTask();
        REQUIRE(updatable==updatedValue);

        REQUIRE_FALSE(queue.HasWork());
    }

    SECTION("Single thread, multiple tasks task") {
        int updatable{0};

        auto task = [&updatable]() -> void {
            updatable++;
        };

        WorkQueue queue;
        queue.Add(task);
        queue.Add(task);

        auto dequeuedTask1 = queue.Take();
        auto dequeuedTask2 = queue.Take();

        dequeuedTask1();
        dequeuedTask2();
        REQUIRE(updatable == 2);
    }

    SECTION("Multiple threads, multiple tasks task wait before publish") {
        atomic<int> updatable{0};
        WorkQueue queue;

        auto consumeExec = [&] {
            auto task = queue.Take();
            task();
        };

        thread consumer1(consumeExec);
        thread consumer2(consumeExec);

        auto execTask = [&updatable] {
            updatable++;
        };

        queue.Add(execTask);
        queue.Add(execTask);

        consumer1.join();
        consumer2.join();

        REQUIRE(updatable == 2);
    }


    SECTION("Multiple producer, multiple consumer threads, all threads consume") {
        const int consumerThreadCount = 31;
        const int producerThreadCount = 4;


        unordered_set<thread::id> threadsExecuted;
        mutex threadsExecutedMutex;
        WorkQueue queue;

        auto consumerTask = [&] {
            while(true) {
                {
                    lock_guard<mutex> guard(threadsExecutedMutex);
                    if(threadsExecuted.find(this_thread::get_id()) != threadsExecuted.end()) {
                        return;
                    }
                }

                auto task = queue.Take();
                task();
                lock_guard<mutex> guard(threadsExecutedMutex);
                threadsExecuted.insert(this_thread::get_id());
            }
        };

        auto producerTask = [&threadsExecutedMutex, &threadsExecuted, &queue] {
            while(true) {
                {
                    lock_guard<mutex> guard(threadsExecutedMutex);
                    if(threadsExecuted.size() == consumerThreadCount) {
                        return;
                    }
                }

                queue.Add([] {});
            }
        };

        vector<thread> consumerThreads;
        consumerThreads.reserve(consumerThreadCount);
        for(int i = 0 ; i < consumerThreadCount ; i++) {
            consumerThreads.emplace_back(consumerTask);
        }

        vector<thread> producerThreads;
        producerThreads.reserve(consumerThreadCount);
        for(int i = 0 ; i < producerThreadCount ; i++) {
            producerThreads.emplace_back(producerTask);
        }


        bool testRunning = true;
        while(testRunning) {
            {
                lock_guard<mutex> guard(threadsExecutedMutex);
                testRunning = threadsExecuted.size() < consumerThreadCount;
            }
        }


        for(auto& thread: producerThreads) {
            thread.join();
        }

        for(auto& thread: consumerThreads) {
            thread.join();
        }

        REQUIRE(threadsExecuted.size() == consumerThreadCount);
    }

}
