#include <iostream>
#include <WorkerPool.h>
#include <Processor.h>

auto runWithCoreAffinity() -> void {
    dxpool::Processor processor;
    const auto cores = processor.FindAvailableCores();

    dxpool::WorkerPoolBuilder builder;
    auto pool = builder.OnCores(cores)
                .WithThreadsPerCore(1) // create one thread per core
                .Build();

    std::cout << "number of worker threads in the pool: " << pool->Size() << std::endl;
    auto taskWithResult = [](int valueA, int valueB) -> int { return valueA + valueB;};

    auto result = pool->Submit<decltype(taskWithResult), int, int, int>(std::move(taskWithResult), 1, 1);

    std::cout << "task returned " << result.get() << std::endl;

    // fire and forget task, the pool will wait for it to finish when going out of scope
    pool->Submit([&processor] {
        const auto thisCore = processor.FindAvailableCores();
        std::cout << "task running on core " << thisCore.begin()->GetID() << std::endl;
    });
}

auto runWithNUMAAffinity() -> void {
    dxpool::Processor processor;
    const auto node = processor.FindAvailableNumaNodes();

    dxpool::WorkerPoolBuilder builder;
    auto pool = builder.OnNUMANode(*node.begin())
                .WithThreadsPerCore(1)
                .Build();

    std::cout << "number of worker threads in the pool: " << pool->Size() << std::endl;

    pool->Submit([&processor] {
        const auto thisCore = processor.FindAvailableCores();
        const auto thisNUMANode = processor.FindAvailableNumaNodes();
        std::cout << "running on NUMA node " << thisNUMANode.begin()->GetID() << " core " << thisCore.begin()->GetID() << std::endl;
    });

}


auto runAllExamples() -> void {
    std::cout << "-- executing tasks with core affinity" << std::endl;
    runWithCoreAffinity();

    std::cout << std::endl;
    std::cout << "-- executing tasks with NUMA node affinity" << std::endl;

    runWithNUMAAffinity();
}

auto main() -> int {
    // WorkerPoolBuilder can throw InvalidWorkerPoolBuilderArgumentsError
    try {
        runAllExamples();
    } catch(const dxpool::InvalidWorkerPoolBuilderArgumentsError& ex) {
        std::cerr << "Error running examples: " << ex.what() << std::endl;
    }
    return 0;
}