#include <cassert>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "../src/ConcurrentIndexer.h"
#include "../src/Pool.h"
#include "../src/PoolItem.h"

#include "catch2/catch_message.hpp"

using namespace dxpool;
using namespace std;

struct ResetableInt {
    int value{0};

    auto Reset() ->void {
        this->value = 0;
    }
};


template <typename PoolType>
class PoolBenchFixture {
  public:
    static const int PoolOperationsIterations = 1000;

  private:
    vector<thread> threads;
    PoolType& pool;
    mutex readyMutex;
    condition_variable readyCondVar;
    atomic<size_t> tasksCompleted{0};
    atomic<bool> isRunning{true};

    // negative run cycle means not running
    int runCycle{-1};

    auto startRun() -> void {
        // only run if the thread run cycle is the same as the fixture run cycle
        // that is because the benchmark framework will execute runBenchmark multiple times
        int threadRunCycle{0};
        while(isRunning) {
            unique_lock<mutex> readyLock(this->readyMutex);
            this->readyCondVar.wait(readyLock, [this, threadRunCycle] {return this->runCycle==threadRunCycle || !this->isRunning;});
            if(!isRunning) {
                return;
            }

            PoolBenchFixture<PoolType>::RunPoolOperationsBenchmark(this->pool,PoolOperationsIterations);

            this->tasksCompleted++;
            threadRunCycle++;
        }
    }


  public:
    PoolBenchFixture(const PoolBenchFixture &) = delete;
    PoolBenchFixture(PoolBenchFixture &&) = delete;
    auto operator=(const PoolBenchFixture &) -> PoolBenchFixture & = delete;
    auto operator=(PoolBenchFixture &&) -> PoolBenchFixture & = delete;

    static inline auto RunPoolOperationsBenchmark(PoolType& pool, int iterations) -> void {
        for(int i = 0 ; i < iterations ; i++) {
            auto item = pool.Take();

            (void)item; // it shouldn't be need since the destructor has side effects

            // item will be returned to the pool here
        }
    }

    PoolBenchFixture(int numThreads, PoolType& dataPool):pool(dataPool) {
        for(int i = 0 ; i < numThreads ; i++) {
            this->threads.emplace_back([this] {this->startRun();});
        }
    }

    auto runBenchmark() -> void {
        {
            lock_guard<mutex> readyGuard(this->readyMutex);
            this->tasksCompleted = 0;
            this->runCycle++;

        }

        this->readyCondVar.notify_all();

        // wait for this cycle to complete
        while(this->tasksCompleted!=this->threads.size()) {
            this_thread::yield();
        }
    }

    ~PoolBenchFixture() {
        // unblock threads
        {
            lock_guard<mutex> readyGuard(this->readyMutex);
            this->isRunning=false;
        }

        this->readyCondVar.notify_all();
        for(size_t i = 0 ; i < this->threads.size() ; i++) {
            this->threads[i].join();
        }
    };
};

template<typename PoolType>
auto execStaticBenchmark(Catch::Benchmark::Chronometer& meter, int threadCount) -> void {
    PoolType pool;
    PoolBenchFixture<PoolType> fixture(threadCount, pool);
    meter.measure([&fixture] { fixture.runBenchmark(); });
}

template<IndexSizeT PoolSize>
auto execStaticMutexPoolBench(Catch::Benchmark::Chronometer& meter, int threadCount) -> void {
    execStaticBenchmark<StaticPool<ResetableInt, PoolSize, MutexIndexer>>(meter, threadCount);
}

template<IndexSizeT PoolSize>
auto execStaticConcurrentPoolBench(Catch::Benchmark::Chronometer& meter, int threadCount) -> void {
    execStaticBenchmark<StaticPool<ResetableInt, PoolSize, ConcurrentIndexer>>(meter, threadCount);
}

TEST_CASE("static pool, different indexers, baseline", "[bench][static][baseline]") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    BENCHMARK_ADVANCED("size 1, mutex indexer, main thread")(Catch::Benchmark::Chronometer meter) {
        using PoolType = StaticPool<ResetableInt, 1>;
        meter.measure([] {
            PoolType pool;
            PoolBenchFixture<PoolType>::RunPoolOperationsBenchmark(pool, PoolBenchFixture<PoolType>::PoolOperationsIterations);
        });
    };

    BENCHMARK_ADVANCED("size 1, concurrent indexer, main thread")(Catch::Benchmark::Chronometer meter) {
        using PoolType = StaticPool<ResetableInt, 1, ConcurrentIndexer>;
        meter.measure([] {
            PoolType pool;
            PoolBenchFixture<PoolType>::RunPoolOperationsBenchmark(pool, PoolBenchFixture<PoolType>::PoolOperationsIterations);
        });
    };

}

TEST_CASE("static pool, different indexers, multi-threaded", "[bench][static]") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    const int poolSize1k = 1024;

    BENCHMARK_ADVANCED("size 1, mutex indexer, main thread")(Catch::Benchmark::Chronometer meter) {
        using PoolType = StaticPool<ResetableInt, 1>;
        meter.measure([] {
            PoolType pool;
            PoolBenchFixture<PoolType>::RunPoolOperationsBenchmark(pool, PoolBenchFixture<PoolType>::PoolOperationsIterations);
        });
    };

    BENCHMARK_ADVANCED("size 1, concurrent indexer, main thread")(Catch::Benchmark::Chronometer meter) {
        using PoolType = StaticPool<ResetableInt, 1, ConcurrentIndexer>;
        meter.measure([] {
            PoolType pool;
            PoolBenchFixture<PoolType>::RunPoolOperationsBenchmark(pool, PoolBenchFixture<PoolType>::PoolOperationsIterations);
        });
    };

    BENCHMARK_ADVANCED("size 1, mutex indexer, single thread")(Catch::Benchmark::Chronometer meter) {
        execStaticMutexPoolBench<1>(meter, 1);
    };

    BENCHMARK_ADVANCED("size 1, concurrent indexer, single thread")(Catch::Benchmark::Chronometer meter) {
        execStaticConcurrentPoolBench<1>(meter, 1);
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, single thread")(Catch::Benchmark::Chronometer meter) {
        execStaticMutexPoolBench<poolSize1k>(meter, 1);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, single thread")(Catch::Benchmark::Chronometer meter) {
        execStaticConcurrentPoolBench<poolSize1k>(meter, 1);
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, 2 threads")(Catch::Benchmark::Chronometer meter) {
        execStaticMutexPoolBench<poolSize1k>(meter, 2);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, 2 threads")(Catch::Benchmark::Chronometer meter) {
        execStaticConcurrentPoolBench<poolSize1k>(meter, 2);
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, 12 threads")(Catch::Benchmark::Chronometer meter) {
        const int threadCount = 12;
        execStaticMutexPoolBench<poolSize1k>(meter, threadCount);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, 12 threads")(Catch::Benchmark::Chronometer meter) {
        const int threadCount = 12;
        execStaticConcurrentPoolBench<poolSize1k>(meter, threadCount);
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, hardware concurrency threads")(Catch::Benchmark::Chronometer meter) {
        execStaticMutexPoolBench<poolSize1k>(meter, static_cast<int>(thread::hardware_concurrency()));
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, hardware concurrency threads")(Catch::Benchmark::Chronometer meter) {
        execStaticConcurrentPoolBench<poolSize1k>(meter, static_cast<int>(thread::hardware_concurrency()));
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, 64 threads")(Catch::Benchmark::Chronometer meter) {
        const int threadCount = 64;
        execStaticMutexPoolBench<poolSize1k>(meter, threadCount);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, 64 threads")(Catch::Benchmark::Chronometer meter) {
        const int threadCount = 64;
        execStaticConcurrentPoolBench<poolSize1k>(meter, threadCount);
    };
}

template<typename PoolType>
auto execRuntimeBenchmark(size_t poolSize, Catch::Benchmark::Chronometer& meter, int threadCount) -> void {
    PoolType pool(poolSize);
    PoolBenchFixture<PoolType> fixture(threadCount, pool);
    meter.measure([&fixture] { fixture.runBenchmark(); });
}

auto execRuntimeMutexPoolBench(size_t poolSize, Catch::Benchmark::Chronometer& meter, int threadCount) -> void {
    execRuntimeBenchmark<RuntimePool<ResetableInt, MutexIndexer>>(poolSize, meter, threadCount);
}

auto execRuntimeConcurrentPoolBench(size_t poolSize, Catch::Benchmark::Chronometer& meter, int threadCount) -> void {
    execRuntimeBenchmark<RuntimePool<ResetableInt, ConcurrentIndexer>>(poolSize, meter, threadCount);
}

TEST_CASE("runtime pool, different indexers, multi-threaded", "[bench][runtime]") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    const int poolSize1k = 1024;

    BENCHMARK_ADVANCED("size 1K, mutex indexer, single thread")(Catch::Benchmark::Chronometer meter) {
        execRuntimeMutexPoolBench(poolSize1k, meter, 1);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, single thread")(Catch::Benchmark::Chronometer meter) {
        execRuntimeConcurrentPoolBench(poolSize1k, meter, 1);
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, 2 threads")(Catch::Benchmark::Chronometer meter) {
        execRuntimeMutexPoolBench(poolSize1k, meter, 2);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, 2 threads")(Catch::Benchmark::Chronometer meter) {
        execRuntimeConcurrentPoolBench(poolSize1k, meter, 2);
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, hardware concurrency threads")(Catch::Benchmark::Chronometer meter) {
        execRuntimeMutexPoolBench(poolSize1k, meter, static_cast<int>(thread::hardware_concurrency()));
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, hardware concurrency threads")(Catch::Benchmark::Chronometer meter) {
        execRuntimeConcurrentPoolBench(poolSize1k, meter, static_cast<int>(thread::hardware_concurrency()));
    };

    BENCHMARK_ADVANCED("size 1K, mutex indexer, 64 threads")(Catch::Benchmark::Chronometer meter) {
        const int threadCount = 64;
        execRuntimeMutexPoolBench(poolSize1k, meter, threadCount);
    };

    BENCHMARK_ADVANCED("size 1K, concurrent indexer, 64 threads")(Catch::Benchmark::Chronometer meter) {
        const int threadCount = 64;
        execRuntimeConcurrentPoolBench(poolSize1k, meter, threadCount);
    };

}
