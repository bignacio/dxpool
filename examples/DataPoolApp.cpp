
#include "IndexHolder.h"
#include "MutexIndexer.h"
#include "ConcurrentIndexer.h"
#include <Pool.h>

#include <string>
#include <iostream>
#include <array>
#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

template<size_t BufferSize>
class StaticBuffer {
  private:
    std::array<char, BufferSize> buffer{};
    size_t position{0};
  public:
    StaticBuffer() = default;
    StaticBuffer(const StaticBuffer &) = delete;
    auto operator=(const StaticBuffer &) -> StaticBuffer & = delete;
    auto operator=(StaticBuffer &&) -> StaticBuffer & = delete;
    StaticBuffer(StaticBuffer &&) = delete;

    auto Add(char cData) -> void {
        this->buffer.at(this->position) = cData;
        this->position++;
    }

    auto Get(size_t pos) const -> char {
        return this->buffer.at(pos);
    }

    auto Size() const -> size_t {
        return this->position;
    }

    auto Reset() -> void {
        // here we would reset values of StaticBufferPool::buffer as needed. I'll chose to set all values to zero
        // and reset the position
        std::fill(this->buffer.begin(), this->buffer.end(), 0);
        this->position = 0;
    }

    ~StaticBuffer() = default;
};

class DynamicBuffer {
  private:
    std::vector<char> buffer;

  public:
    DynamicBuffer() {
        const size_t dynamicBufferSize = 64;
        std::fill_n(std::back_inserter(this->buffer), dynamicBufferSize, 0);
    }

    DynamicBuffer(const DynamicBuffer &) = default;
    DynamicBuffer(DynamicBuffer &&) = default;
    auto operator=(const DynamicBuffer &) -> DynamicBuffer & = default;
    auto operator=(DynamicBuffer &&) -> DynamicBuffer & = default;
    ~DynamicBuffer() = default;
};

/**
 *  Simple example using a custom pooled buffer, backed by an std::array.
 */
auto selectEvenNumbersStaticPool(const std::vector<char>& numbers) -> void {
    // number of items in the pool
    const size_t poolSize = 8;
    // length of the buffer each item in the pool will hold
    const size_t bufferSize = 64;

    // StaticBuffer is not copy or move constructible and contains an std::array,
    // forcing us to use use a dxpool::StaticPool with compile time size
    dxpool::StaticPool<StaticBuffer<bufferSize>, poolSize, dxpool::MutexIndexer> pool;

    auto item = pool.Take();
    if(!item.Empty()) {
        // do something useful with the buffer
        StaticBuffer<bufferSize>* buffer = item.Get();

        for(const auto cData: numbers) {
            if (cData % 2 == 0) {
                buffer->Add(cData);
            }
        }

        for(size_t pos = 0 ; pos < buffer->Size() ; pos++) {
            std::cout << buffer->Get(pos) << " ";
        }
        std::cout << std::endl;
    }

    // item will be returned to the pool here via RAII
}

/**
 *  Simple example using a custom pooled buffer, backed by an std::vector.
 */
auto exampleRuntimePool() -> void {
    // number of items in the pool
    size_t poolSize = 2;

    // DynamicBuffer is move constructible so it can be used wither with
    // a dxpool::StaticPool or dxpool::RuntimePool where poolSize can be determined at run time
    dxpool::RuntimePool<DynamicBuffer, dxpool::MutexIndexer> pool(poolSize);

    auto item = pool.Take();
    if(!item.Empty()) {

        // do something useful with the buffer
        DynamicBuffer* buffer = item.Get();
        (void)buffer;

    }

    // item will be returned to the pool here via RAII
}

/**
* Example using a type without requiring a wrapper implementation to reset its state
*/
auto exampleCustomReseter() -> void {
    auto stringReseter = [](std::string* value) {
        value->assign("");
    };
    dxpool::RuntimePool<std::string> pool(1, stringReseter);

    {
        auto item = pool.Take();

        if(!item.Empty()) {
            std::string* value = item.Get();
            value->assign("hello");
            std::cout << "string item before reset is: '" << *value << "'" << std::endl;
        }

        // item is returnd to the pool and reset here
    }

    auto item = pool.Take();
    std::cout << "string item after reset is: '" << *item.Get() << "'" << std::endl;
}


/**
* Example using a type without reseter and with a concurrent indexer
*/
auto exampleNoReseterConcurrentIndexer() -> void {
    // pool of ints without a custom reseter. It will use a no-op reseter
    dxpool::StaticPool<int, 1, dxpool::ConcurrentIndexer> pool;

    auto item = pool.Take();

    if(!item.Empty()) {
        // act on item
    }
}

auto main() -> int {
    selectEvenNumbersStaticPool({'a','b','c','d'});

    exampleRuntimePool();

    exampleCustomReseter();

    exampleNoReseterConcurrentIndexer();
    return 0;
}