#include <catch2/catch_test_macros.hpp>
#include <string>
#include <set>
#include <algorithm>
#include <future>
#include <thread>
#include <iterator>
#include <mutex>
#include <vector>
#include <chrono>


#include "../src/WorkerPool.h"
#include "../src/Processor.h"

using namespace dxpool;
using namespace std;

struct NumThreadsT {
    unsigned int value;
};

struct NumCoresT {
    unsigned int value;
};

auto makeTestCores(unsigned int numCores) -> set<Core> {
    set<Core> cores;

    for(size_t i = 0 ; i < numCores ; i++) {
        cores.emplace(i);
    }

    return cores;
}

auto verifyCreateThreadsFromCores(NumThreadsT numThreads, NumCoresT numCores) -> void {
    set<Core> cores = makeTestCores(numCores.value);
    WorkerPoolBuilder builder;
    auto pool = builder.WithThreadsPerCore(numThreads.value)
                .OnCores(cores)
                .Build();

    REQUIRE(pool->Size() == static_cast<unsigned long>(numCores.value) * numThreads.value);
}


auto verifyCreateThreadsFromNUMANode(NumThreadsT numThreads, NumCoresT numcores) -> void {
    set<Core> cores = makeTestCores(numcores.value);
    const NUMANode node(0, cores);

    WorkerPoolBuilder builder;
    auto pool = builder.WithThreadsPerCore(numThreads.value)
                .OnNUMANode(node)
                .Build();

    REQUIRE(pool->Size() == numThreads.value * node.Cores().size());
}

auto verifyRunWithCoreAffinity(const set<Core>& targetCores, unique_ptr<WorkerPool>& pool)-> void {
    const Processor processor;


    mutex coreMtx;

    // use a mutex as a form of synchronization point (barrier) for all executing tasks so that we can force
    // exactly one task executing only once on exactly one core
    mutex syncMtx;
    syncMtx.lock();

    vector<Core> actualCores;
    auto task =[&actualCores, &processor, &coreMtx, &syncMtx]() {
        // since threads are pinned to one cores, there should every only be one returned from FindAvailableCores
        // adding all will make the test fail, if there's more than one core
        for(const auto& core: processor.FindAvailableCores()) {
            unique_lock<mutex> coreLock(coreMtx);
            actualCores.push_back(core);
        }

        // wait here until the test says it's good to continue and immediately unlock for other tasks to progress
        syncMtx.lock();
        syncMtx.unlock();
    };

    // create one task for each core
    for(size_t i = 0 ; i < targetCores.size() ; i++) {
        pool->Submit(task);
    }

    while(pool->HasWork()) {
        this_thread::yield();
    }

    // here the pool has consumed all tasks and is processing them
    // we can then unlock the sync point and let all threads in the pool finish the work
    syncMtx.unlock();

    pool->Shutdown();

    sort(actualCores.begin(), actualCores.end());

    REQUIRE(equal(targetCores.begin(), targetCores.end(), actualCores.begin()));


}

TEST_CASE("Worker pool - task execution") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)

    SECTION("Run on any core with result") {
        const int runResult = 644;
        WorkerPoolBuilder builder;
        auto pool = builder.WithThreadsPerCore(1).OnCores({Core{0}}).Build();

        auto task = []() -> int {
            return runResult;
        };

        std::future<int> res = pool->Submit<decltype(task), int>(std::move(task));

        REQUIRE(res.get() == 644);
    }


    SECTION("Run on any core without result") {
        const int expected = 552;
        int updatable{0};
        WorkerPoolBuilder builder;
        auto pool = builder.WithThreadsPerCore(1).OnCores({Core{0}}).Build();

        auto task = [&updatable]()  {
            updatable = expected;
        };

        pool->Submit(task);
        // Wait until work is completed
        // We can't for sure say that the task was executed, only that it has been picked up for execution
        // in this case, HasWork() may return true but the task hasn't finished running yet
        // so we shutdown the pool and wait for all threads to exit
        while(pool->HasWork()) {
            this_thread::yield();
        }

        pool->Shutdown();

        REQUIRE(updatable == expected);
    }

    SECTION("Run with affinity - cores") {
        Processor processor;
        set<Core> targetCores;
        const auto allCores = processor.FindAvailableCores();

        for(auto it = allCores.begin() ; it!= allCores.end();) {
            targetCores.insert(*it);
            it++;
            if(it != allCores.end()) {
                it++;
            }
        }
        WorkerPoolBuilder builder;
        auto pool = builder.OnCores(targetCores).WithThreadsPerCore(1).Build();

        verifyRunWithCoreAffinity(targetCores, pool);
    }

    SECTION("Run with affinity - NUMA node") {
        Processor processor;
        auto nodes = processor.FindAvailableNumaNodes();
        for(const auto& node: nodes) {
            WorkerPoolBuilder builder;
            auto pool = builder.OnNUMANode(node).WithThreadsPerCore(1).Build();
            verifyRunWithCoreAffinity(node.Cores(), pool);
        }
    }

    SECTION("Pool shutdown") {
        WorkerPoolBuilder builder;
        auto pool = builder.
                    WithThreadsPerCore(thread::hardware_concurrency()*2).
                    OnCores({Core{0}}).Build();

        // after the pool is shutdown, the test should finish and not timeout
        pool->Shutdown();
        // destroying a pool that is shutdown should do nothing
    }
}

TEST_CASE("Worker pool builder") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)

    SECTION("Build with cores") {
        const unsigned int threadsPerCore = 7;
        const auto cores = makeTestCores(3);

        WorkerPoolBuilder builder;
        builder.WithThreadsPerCore(threadsPerCore).OnCores(cores);

        REQUIRE(builder.ThreadsPerCore() == threadsPerCore);
        REQUIRE(std::equal(std::begin(builder.Cores()),
                           std::end(builder.Cores()),
                           std::begin(cores)));
        REQUIRE(builder.TargetNUMANode().Empty());
    }

    SECTION("Build with NUMA") {
        const unsigned int threadsPerCore = 7;
        const NUMANode numaNode;

        WorkerPoolBuilder builder;
        builder.WithThreadsPerCore(threadsPerCore).OnNUMANode(numaNode);

        REQUIRE(builder.ThreadsPerCore() == threadsPerCore);
        REQUIRE(builder.TargetNUMANode() == numaNode);
        REQUIRE(builder.Cores().empty());
    }

    SECTION("Throw without threads per core") {
        WorkerPoolBuilder builder;
        const set<Core> cores = makeTestCores(3);
        NUMANode node;
        REQUIRE_THROWS_AS(builder.OnCores(cores).Build(),
                          InvalidWorkerPoolBuilderArgumentsError
                         );

    }

    SECTION("Throw without cores or numa nodes") {
        WorkerPoolBuilder builder;
        NUMANode node;
        REQUIRE_THROWS_AS(builder.WithThreadsPerCore(1).Build(),
                          InvalidWorkerPoolBuilderArgumentsError
                         );

    }

    SECTION("Throw on NUMA and cores since both aren't allowed") {
        WorkerPoolBuilder builder;
        const set<Core> cores = makeTestCores(3);
        NUMANode node{0, cores};
        REQUIRE_THROWS_AS(builder.OnCores(cores).OnNUMANode(node).WithThreadsPerCore(1).Build(),
                          InvalidWorkerPoolBuilderArgumentsError
                         );

    }

    SECTION("Create threads, one thread per core, multiple cores") {
        verifyCreateThreadsFromCores(NumThreadsT{1}, NumCoresT{3});
    }

    SECTION("Create threads, multiple threads per core, 1 core") {
        const NumThreadsT numThreads{7};
        verifyCreateThreadsFromCores(numThreads, NumCoresT{1});
    }

    SECTION("Create threads, multiple threads per core, multiple cores core") {
        verifyCreateThreadsFromCores(NumThreadsT{4}, NumCoresT{2});
    }

    SECTION("Create threads for NUMA node, one per core") {
        verifyCreateThreadsFromNUMANode(NumThreadsT{1}, NumCoresT{3});
    }

    SECTION("Create threads for NUMA node, multiple threads per core") {
        verifyCreateThreadsFromNUMANode(NumThreadsT{3}, NumCoresT{4});
    }

}