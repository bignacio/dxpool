#ifndef INDEXER_TEMPLATE_TEST_H
#define INDEXER_TEMPLATE_TEST_H

#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <mutex>
#include <set>
#include <vector>
#include <list>
#include <thread>
#include <atomic>

using namespace std;

template<typename Indexer>
class IndexerFixture {
  private:
    // helper function for tests that use Indexer concurrently
    static auto GetAndReturnIndexForTesting(Indexer* indexer, set<size_t>* indices, mutex* mtxIndices) {
        for(auto result = indexer->Next() ; !result.Empty(); result = indexer->Next()) {
            // give other threads a chance to get an index. This simulates some processing after the index is returned
            this_thread::yield();
            const size_t returnedIndex = result.Get();

            // return and get another index to be added to the set
            indexer->Return(returnedIndex);
            result = indexer->Next();
            this_thread::yield();

            if(!result.Empty()) {
                const size_t index = result.Get();
                lock_guard<mutex> lock(*mtxIndices);

                // the index must not have been seen before
                bool indexExists = indices->find(index) == indices->end();
                REQUIRE(indexExists);

                indices->insert(index);
            }
        }
    }

    static auto RunGetAndReturnIndicesMultiThreaded(size_t maxSize, int threadCount) -> void { // NOLINT(bugprone-easily-swappable-parameters)
        set<size_t> indices;
        mutex mtxIndices;
        list<thread> threads;

        Indexer indexer(maxSize);

        // each thread will get and return an index until none is available

        for(int i = 0; i < threadCount; ++i) {
            threads.emplace_back(IndexerFixture::GetAndReturnIndexForTesting, &indexer, &indices, &mtxIndices);
        }

        // wait for all threads to finish
        for(auto& indexerThread : threads) {
            indexerThread.join();
        }

        // now run our checks. The number of indices collected should be exactly the maximum size of the indexer
        // and all indices should have been collected without gaps
        REQUIRE(indices.size() == maxSize);
        for(size_t i = 0; i < maxSize; ++i) {
            indices.erase(i);
        }

        REQUIRE(indices.empty());
    }
  public:
    // Get all indices until empty
    static auto GetAllIndices() -> void {
        const size_t maxSize = 37;
        set<size_t> indices;

        Indexer indexer(maxSize);

        for(size_t i = 0; i < maxSize; ++i) {
            const size_t index = indexer.Next().Get();
            indices.insert(index);
        }

        REQUIRE(indexer.Next().Empty());
        REQUIRE(indices.size() == maxSize);
        for(size_t i = 0; i < maxSize; ++i) {
            indices.erase(i);
        }

        REQUIRE(indices.empty());
    }

    // get single index and return it to the indexer
    auto GetAndReturnOneIndex() -> void {
        Indexer indexer(1);
        const size_t initialIndex = indexer.Next().Get();

        REQUIRE(initialIndex==0);

        // trying to get another index should fail
        auto mayHaveIndex = indexer.Next();
        REQUIRE(mayHaveIndex.Empty());

        indexer.Return(initialIndex);

        auto result = indexer.Next();
        REQUIRE_FALSE(result.Empty());
        const size_t returnedIndex = result.Get();

        REQUIRE(initialIndex == returnedIndex);
    }

    auto GetAndReturnOneIndexMultipleTimes() -> void {
        Indexer indexer(1);
        const int iterations = 77;

        for(int i = 0 ; i < iterations ; i++) {
            const size_t index = indexer.Next().Get();
            REQUIRE(index==0);
            indexer.Return(index);
        }

    }


    // Get various indices and return them to the indexer
    static auto GetAndreturnVariousIndices() -> void {
        const size_t maxSize = 17;

        Indexer indexer(maxSize);

        // Start emptying all indices
        for(size_t i = 0; i < maxSize; ++i) {
            indexer.Next();
        }

        // Now return them all. We should obtain the returned index in the following Next call
        bool allEqual = true;

        size_t iterations = 0;
        for(size_t i = 0; i < maxSize; ++i) {
            indexer.Return(i);
            const size_t index = indexer.Next().Get();

            allEqual = allEqual && (index == i);
            iterations++;
        }

        // make sure we iterated at least once. Or else checking allEqual maybe incorrect
        REQUIRE(iterations == maxSize);
        REQUIRE(allEqual);
    }

    // Get index no more indices available
    static auto GetIndexNoMoreIndices() -> void {
        const size_t maxSize = 3;
        Indexer indexer(maxSize);

        // when there are no more indices available, the index is equal to max size
        for(size_t i = 0 ; i < maxSize ; i++) {
            indexer.Next();
        }

        auto mayHaveIndex = indexer.Next();

        REQUIRE(mayHaveIndex.Empty());
    }

    // Get all indices until empty using multiple threads
    static auto GetIndicesMultiThreaded() -> void {
        const int threadCount  = 22;
        const int maxSize = 567;

        set<size_t> indices;
        mutex mtxIndices;
        list<thread> threads;

        Indexer indexer(maxSize);

        // each thread will try to get an index until none is available
        auto getIndexFn = [&]() -> void {
            for(auto result = indexer.Next() ; !result.Empty(); result = indexer.Next()) {
                const size_t index = result.Get();
                {
                    lock_guard<mutex> lock(mtxIndices);

                    // the index must not have been seen before
                    REQUIRE(indices.find(index) == indices.end());

                    indices.insert(index);
                }
            }
        };

        for(int i = 0; i < threadCount; ++i) {
            threads.emplace_back(getIndexFn);
        }

        // wait for all threads to finish
        for(auto& indexerThread : threads) {
            indexerThread.join();
        }

        // now run our checks. The number of indices collected should be exactly the maximum size of the indexer
        // and all indices should have been collected without gaps
        REQUIRE(indices.size() == maxSize);
        REQUIRE(indexer.Next().Empty());

        for(size_t i = 0; i < maxSize; ++i) {
            indices.erase(i);
        }

        REQUIRE(indices.empty());
    }

    // Get and return indices with multiple threads
    static auto GetAndReturnIndicesMultiThreaded() -> void {
        const int threadCount  = 22;
        const int maxSize = 567;

        RunGetAndReturnIndicesMultiThreaded(maxSize, threadCount);
    }

    // Get and return indices with multiple threads
    static auto GetAndReturnIndicesMoreThreadsThanItems() -> void {
        const int threadCount  = 13;
        const int maxSize = 5;

        RunGetAndReturnIndicesMultiThreaded(maxSize, threadCount);
    }
};


#endif