# dxpool - An object and worker pool library


`dxpool` is a header only object and worker pool library for high performance applications.

Objects stored in the pool can be of any type (arrays, primitive types, etc) and can be retrieved from and returned to the pool without memory allocation overhead.


The worker pool implementation uses pool of threads with user defined core and/or NUMA node affinity, making it ideal for thread-per-core concurrency models.

## Contributing

See [the contribution guide](CONTRIBUTING.md) for details on how to propose and submit changes to this project, always observing [the code of conduct](CODE-OF-CONDUCT.md).

## Supported compilers and platforms

`dxpool` is compatible with C++11 on the following compilers
* gcc
* clang

Supported processors:
* x86-64

Operating systems:
* Linux
* Windows (at the moment, worker pools are only supported on Linux)


Though C++11 is supported, C++17 is recommended for its copy elision guarantees.

## Download and usage

### Manual installation

`dxpool` is a header only that has no external dependencies. The easiest way to manually install the library is to download the release zip file, decompress it in the desired location and added to the project's include path.

### Using `CMake`
The easiest way to include `dxpool` in a `CMake` project is via `FetchContent`:

```
FetchContent_Declare(
  dxpool
  GIT_REPOSITORY https://github.com/bignacio/dxpool.git
  GIT_TAG        0.1.0
)
FetchContent_MakeAvailable(dxpool)

# where TARGET_NAME is the target using dxpool
target_include_directories(<TARGET_NAME> PRIVATE ${DXPOOL_INCLUDE_DIR})
```

The include path is available through the variable `DXPOOL_INCLUDE_DIR`

### Using Conan

*Coming soon*

## The object pool

Objects are instantiated and added to the pool at pool's construction time. Currently object types **must be default constructible**. This means the pool size (number of items in the pool) will not change through its lifetime.

This an intentional design decision, for the time being, and applies best to uses cases where memory usage must be consistent and deterministic.

The `StaticPool` statically allocates the pool size and its elements. In this case, the pool size is provided as a template parameter.

The `RuntimePool` permits developers to specify the size of the pool at runtime but in this case, the type of the objects in the pool must be *move constructible (note the pool size still won't change after its construction).

### Concurrency support

All object pools are thread safe and allow different mechanisms for data safety via what is called an `Indexer`.

The `MutexIndexer` is the default mechanism and, as the name suggests, is backed by an `std::mutex`.

The **experimental** `ConcurrentIndexer` users lock free patterns for data safety and even though it is not lock free (due to the presence of a spin lock), the `ConcurrentIndexer` substantially reduces the probability that a thread would block making it ideal for high concurrency situations.

The `ConcurrentIndexer` can be between 20% and 30% than the `MutexIndexer` faster due to fewer points of contention accessing the pool.

You can use the [benchmark tests](benchmark) to verify the nominal execution performance on your target systems.

### Retrieving and returning items

Items retrieved from the pool are wrapped in a `PoolType` object that has `Empty() == true` if there are no available objects in the pool. Retrieving items from the pool will never block waiting for an available item.

Items are returned to the pool automatically through RIIA and allow for custom code to be invoked and reset items before returning them to the pool.

For more details consult [examples](examples), [tests](test) and the API [documentation](https://bignacio.github.io/dxpool).

### Putting it all together

Declaring and using an static pool
```
#include "IndexHolder.h"
#include "MutexIndexer.h"
#include "ConcurrentIndexer.h"
#include <Pool.h>

// a static pool containing 50 strings using the MutexIndexer
dxpool:StaticPool<std::string, 50, MutexIndexer> pool;

auto item = pool.Take();
if(!item.Empty()){
  std::string* stringFromThePool = item.Get();
}
// item will be returned to the pool here when out of scope

....

// a runtime size defined pool containing 10 of array buffers using the ConcurrentIndexer
dxpool::RuntimePool<std::array<char, 10>, ConcurrentIndexer> pool(10);
// use the pool here
...
```


## The worker pool

The worker pool creates a fixed set of threads, each with specific core affinity.

The `Processor` interface allow the interrogation of supported cores and NUMA nodes that can then be passed to an instance of the `WorkerPoolBuilder` which in turn will create the pool threads with the desire affinity for specific cores or an entire NUMA node.
The builder allows creating a set of workers for either a NUMA node or a set of cores but not both. The builder will throw `InvalidWorkerPoolBuilderArgumentsError` in case of a mismatch in the pool configuration.

Even though a worker pool can have threads with affinity across NUMA nodes, it is recommended a single worker pool per node for memory intensive applications.

There are 2 ways the method `Submit` can be used. In the simplest case, a `WorkQueue::WorkerTask` can be added to the work queue to be executed as a fire-and-forget task.

The other way is to invoke `Submit` with a given task and its arguments, where the result of the execution can be later obtained via the returned `std::promise`. For example

```
auto sumTask = [](int valueA, int valueB) -> int { return valueA + valueB;};
auto result = pool->Submit<decltype(sumTask), int, int, int>(std::move(sumTask), 1, 1);

std::cout << "the sum is " << result.get() << std::endl;

```

Worker pools have thread safety guarantees and can be used concurrently by multiple threads.

Examples of creating and using worker pools
```
#include <WorkerPool.h>

// a worker pool of 1 thread for all available cores
dxpool::Processor processor;
const auto cores = processor.FindAvailableCores();

dxpool::WorkerPoolBuilder builder;
auto pool = builder.OnCores(cores) // use all cores
            .WithThreadsPerCore(1) // create one thread per core
            .Build();


// a worker pool with 2 threads for the first NUMA node available

#include <WorkerPool.h>
dxpool::Processor processor;
const auto node = processor.FindAvailableNumaNodes();

dxpool::WorkerPoolBuilder builder;
auto pool = builder.OnNUMANode(*node.begin())
                .WithThreadsPerCore(2)
                .Build();
```

For more details consult [examples](examples), [tests](test) and the API [documentation](https://bignacio.github.io/dxpool).


### Notes on task execution

Upon calling `Submit` tasks are added to a task queue and executed when there's an available worker.

This means tasks are not executed immediately and can even starve if workers never become available.

There's also a performance cost for submitting tasks so make sure to benchmark your code frequently.
Increasing concurrency and parallelism do not always improve system performance.