#ifndef MUTEX_INDEXER_H
#define MUTEX_INDEXER_H
#include <cstddef>
#include <mutex>
#include <vector>

#include "IndexHolder.h"
#include "TypePolicies.h"

namespace dxpool {

/**
 * @brief Pool item indexer backed by a mutex for locking operations
 *
 * Pool indices start at 0
 */
class MutexIndexer final {
  private:
    std::mutex mutex;
    std::vector<size_t> indices;
    size_t indexPos = 0;
  public:
    /**
     * @brief Construct a new Mutex Indexer object
     * The maximum number of indices must be at least one
     *
     * @param maxSize number of indices. It must be greater than zero
     */
    MutexIndexer(std::size_t maxSize) {
        // The idea is to have all possible indices given a max pool size tracked in this vector
        // when an element is requested from the pool, the index of the item in the pool is retrieved by returning
        // an index from indices starting from the lowest position.
        // However, the vector size never changes, only the position of the next index is incremented or decremented.
        // The order of elements in the vector is not important so long as it has unique elements.

        for(size_t i = 0; i < maxSize; ++i) {
            indices.push_back(i);
        }
    };


    /**
     * @brief Get the next available index
     * if there are no more indices, the returning Result will be empty
     *
     * @return std::size_t next available index
     */
    auto Next() -> IndexHolder {
        std::lock_guard<std::mutex> lock(mutex);

        if(this->indexPos == this->indices.size()) {
            return {};
        }

        const size_t index = indices[this->indexPos];
        this->indexPos++;

        return {index};
    }

    /**
     * @brief Return an index to the pool.
     * There are no checks for validity of the index so callers must ensure the index is within the range of the pool
     * and that they have not been previously returned.
     * Returning more indices than the pool size will result in undefined behavior.
     */
    auto Return(size_t index) -> void {
        std::lock_guard<std::mutex> lock(mutex);
        this->indexPos--;

        indices[this->indexPos] = index;
    }

    FORBID_COPY_MOVE_ASSIGN(MutexIndexer);
    ~MutexIndexer() = default;
};

} // namespace dxpool
#endif