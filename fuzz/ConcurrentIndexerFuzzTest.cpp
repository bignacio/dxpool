#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>

#include <algorithm>
#include <set>
#include <list>
#include <thread>
#include <chrono>
#include <random>
#include <iostream>

#include "../src/ConcurrentIndexer.h"

using namespace dxpool;
using namespace std;

class ConcurrentIndexTesterThread {
  private:
    int iterations;
    bool failed{false};
    set<size_t> indices;
    ConcurrentIndexer& indexer;
    thread testThread;

    auto runTest() -> void {
        const int maxWaitTime = 13;
        random_device randDevice;
        mt19937 randGen(randDevice());
        uniform_int_distribution<> distrib(1, maxWaitTime);


        for(int i = 0; i < this->iterations; i++) {
            auto result = this->indexer.Next();
            if(!result.Empty()) {
                size_t index = result.Get();

                // we should only see new indices
                if(indices.find(index) != indices.end()) {
                    this->failed= true;
                    return;
                }

                this->indices.insert(index);

                // pretend the thread is doing some work
                const auto waitTime = chrono::microseconds(distrib(randGen));
                this_thread::sleep_for(waitTime);
            }

            // don't always return indices
            bool shouldReturn = (distrib(randGen) % 2) == 0;
            if(!this->indices.empty() && shouldReturn) {
                const size_t index = *this->indices.begin();
                this->indexer.Return(index);
                this->indices.erase(index);
            }
        }

        // now return all indices
        for(auto index: this->indices) {
            this->indexer.Return(index);
        }
    }
  public:
    ConcurrentIndexTesterThread(int runIterations, ConcurrentIndexer& aIndexer):iterations(runIterations), indexer(aIndexer) {
        this->testThread = thread([this] {this->runTest();});
    }

    auto Wait() -> void {
        this->testThread.join();
    }

    auto Failed() const -> bool {
        return this->failed;
    }

    ConcurrentIndexTesterThread(ConcurrentIndexTesterThread&) = delete;
    auto operator=(ConcurrentIndexTesterThread&) -> ConcurrentIndexTesterThread& = delete;
    ConcurrentIndexTesterThread(ConcurrentIndexTesterThread&&) = delete;
    auto operator=(ConcurrentIndexTesterThread&&) -> ConcurrentIndexTesterThread& = delete;

    ~ConcurrentIndexTesterThread() = default;
};


void VerifyFuzzMultipleThreadsAndItems(size_t itemCount, int runIterations, int numThreads) { //NOLINT
    ConcurrentIndexer indexer(itemCount);

    list<ConcurrentIndexTesterThread> testers;

    for(int i = 0 ; i < numThreads ; i++) {
        testers.emplace_back(runIterations, indexer);
    }


    for(auto& tester: testers) {
        tester.Wait();
        REQUIRE_FALSE(tester.Failed());
    }

    // check all indices were properly returned
    set<size_t> indices;
    for(size_t i = 0 ; i < itemCount ; i++) {
        auto result = indexer.Next();

        if(result.Empty()) {
            REQUIRE_FALSE(result.Empty());
        }
        REQUIRE_FALSE(result.Empty());
        indices.insert(result.Get());
    }

    REQUIRE(indices.size()==itemCount);
}

TEST_CASE("Fuzz test concurrent indexer - SMOKE") { //NOLINT

    SECTION("Multiple sizes and threads") {
        const size_t maxItems = 1000;
        const int maxIterations = 10;
        const int maxThreads = 10;
        const int checkInterval = 50;

        for(size_t items  = 1 ; items <= maxItems ; items++) {
            if(items % checkInterval == 0) {
                std::cout << "Running with item count="<<items<< "/"<<maxItems << std::endl;
            }
            for(int iterations  = 1 ; iterations <= maxIterations ; iterations++) {
                for(int numThreads  = 2 ; numThreads <= maxThreads ; numThreads++) {
                    VerifyFuzzMultipleThreadsAndItems(items, iterations, numThreads);
                }
            }
        }
    }
}

TEST_CASE("Fuzz test concurrent indexer - FULL") { //NOLINT
    SECTION("Multiple sizes and threads") {
        const size_t maxItems = 6200;
        const int maxIterations = 120;
        const int maxThreads = 210;
        const int step = 5;
        const int checkInterval = 50;

        for(size_t items  = 1 ; items <= maxItems ; items+=step) {
            if(items % checkInterval == 0) {
                std::cout << "Running with item count="<<items<< "/"<<maxItems << std::endl;
            }
            for(int iterations  = 1 ; iterations <= maxIterations ; iterations += step) {
                for(int numThreads  = 2 ; numThreads <= maxThreads ; numThreads += step) {
                    VerifyFuzzMultipleThreadsAndItems(items, iterations, numThreads);
                }
            }
        }
    }
}