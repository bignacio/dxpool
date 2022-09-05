#ifndef ATOMIC_INDEXER_H
#define ATOMIC_INDEXER_H

#include <atomic>
#include <thread>
#include <vector>
#include <limits>

#include "Optimizers.h"
#include "TypePolicies.h"
#include "IndexHolder.h"

namespace dxpool {


// best guess at cache line size
const constexpr int AtomicAlignment = 64;
static const IndexSizeT UnusedPosition = 0;

/**
 * @brief Lock-free indexer
 *
 */
class ConcurrentIndexer final {
  private:


    alignas(AtomicAlignment) std::atomic<IndexSizeT> readPos{0};
    alignas(AtomicAlignment) std::atomic<IndexSizeT> writePos{0};

    std::vector<IndexSizeT> indices;
    const IndexSizeT size;
    /**
     * @brief Max possible position value that is a multiple of size
     *
     */
    IndexSizeT maxPositionSize{0};


    /**
     * @brief Helper function wrapping a non C++ atomic load operation
     * We do this to keep the vector using a simple PoolSizeT type for its values
     * Not portable but we'll try and keep it like this for the moment
     */
    inline static auto AtomicLoad(const IndexSizeT* indexAddress) -> IndexSizeT {
        return __atomic_load_n(indexAddress, __ATOMIC_ACQUIRE);
    }

    /**
     * @brief Helper function wrapping non C++ atomic store operation
     * As with load above, we do this to keep the vector using a simple PoolSizeT type for its values
     *
     * Note: clang-tidy seems to think indexAddress can be a pointer to const for some reason
     */
    inline static auto AtomicStore(IndexSizeT* indexAddress, IndexSizeT indexValue) -> void { //NOLINT(readability-non-const-parameter)
        __atomic_store_n(indexAddress, indexValue,__ATOMIC_RELEASE);
    }

    /**
     * @brief Wrap an atomic value back to zero
     *
     * @param position atomic value indicating the position in the index vector
     * @param size current size requiring the value to wrap to zero
     */
    static auto WrapOnOverflow(std::atomic<IndexSizeT>& position, IndexSizeT size) -> void {
        auto curPos = position.load(std::memory_order_acquire);

        while(curPos==size) {
            if(position.compare_exchange_weak(curPos, 0, std::memory_order_acq_rel)) {
                return;
            }
            curPos = position.load(std::memory_order_acquire);
        }
    }

  public:
    /**
     * @brief Construct a new Concurrent Indexer with a given size indicacting the maximum number of indices to hold
     *        Total memory used is in fact 2 * poolSize.
     *
     * @param poolSize number of possible indices.
     */
    ConcurrentIndexer(IndexSizeT poolSize):size(poolSize*2), maxPositionSize((std::numeric_limits<IndexSizeT>::max()/this->size) * this->size) {
        // The idea is to have all possible indices given a max pool size tracked in this vector
        // when an element is requested from the pool, the index of the item in the pool is retrieved by returning
        // an index from indices starting from the read position.
        // However, the vector size never changes, only the position of the next index is incremented or decremented.
        // The order of elements in the vector is not important so long as it has unique elements.

        // As indices are being consumed, the read position moves but marker for when we can't read anymore only advances
        // when there are no more indices to read.
        // The same thing happens to returned indices, the write position moves but the end position only advances when we can't write anymore.
        // One important design decision, is that we don't check if we can write over valid read positions since that should never happen
        // if only consumed indices are returned. That's intentional for simplicity and performance reasons.

        this->indices.reserve(this->size);
        this->indices.assign(this->size, UnusedPosition);
        // populate the entire vector with values but only the first half are used for indices
        // this is just so we properly allocate all positions
        for(IndexSizeT i = 0; i < poolSize; ++i) {
            // add one to the indice so we can reserve 0 as UnusedPosition
            this->indices[i] = i+1;
        }


        // set the write position one indice after the end of all valid indices
        writePos = poolSize;
    }

    /**
     * @brief Get the next available index.
     * if there are no more indices, the returning IndexHolder will be empty
     *
     * @return PoolSizeT next available index
     */
    auto Next() -> IndexHolder {
        while(true) {
            // do we always need to load? perhaps not but, better be safe when it comes to atomic operations. I'll save the regrets for later
            IndexSizeT curReadPos = this->readPos.load(std::memory_order_acquire);

            if(curReadPos == this->maxPositionSize) {
                WrapOnOverflow(this->readPos, this->maxPositionSize);
            } else {
                IndexSizeT curWritePos = this->writePos.load(std::memory_order_acquire);

                // There's no more items to read because we read all nothing has been returned
                if(curReadPos == curWritePos) {
                    return {};
                }

                // The action read index needs to be constrained to the size of the buffer
                // Nothing prevents two threads from having the same read index for differents read positions
                // because other threads could keep moving the read position forward
                // so we try to correct that later when reading the value from the vector
                const IndexSizeT curReadIndex = curReadPos % this->size;

                // It's possible that writing (Return) has started (incremented writePos) but the value hasn't been written yet
                // Note to self: this should never happen and if it does there might be a bug somewhere
                if(unlikely(AtomicLoad(&this->indices[curReadIndex]) == UnusedPosition)) {
                    return {};
                }

                auto modified = this->readPos.compare_exchange_weak(curReadPos, curReadPos + 1, std::memory_order_acq_rel);

                if(modified) {
                    IndexSizeT index = AtomicLoad(&this->indices[curReadIndex]);
                    // It's possible we got here because we wrapped but the thread writing to this position
                    // hasn't finished doing so yet. We try to keep the behaviour correct via busy wait
                    // This situation could happen if the size is much smaller than the number of threads calling it
                    while(index == UnusedPosition) {
                        index = AtomicLoad(&this->indices[curReadIndex]);
                        std::this_thread::yield();
                    }

                    AtomicStore(&this->indices[curReadIndex],  UnusedPosition);

                    return {index-1};
                }
            }
        }
    }


    /**
     * @brief Return an index to the pool.
     * There are no checks for validity of the index so callers must ensure the index is within the range of the pool
     * and that they have not been previously returned.
     * Returning more indices than the pool size will result in undefined behavior.
     */
    auto Return(IndexSizeT index) -> void {
        while(true) {
            auto curWritePos = this->writePos.load(std::memory_order_acquire);
            if(curWritePos == this->maxPositionSize) {
                WrapOnOverflow(this->writePos, this->maxPositionSize);
            } else {

                // Note we don't want calls to this method to ever fail (return without writing) so the write position must always be valid
                auto modified = this->writePos.compare_exchange_weak(curWritePos, curWritePos+1, std::memory_order_acq_rel);

                if(modified) {
                    // As in the read position, various threads could get here and curWriteIndex will wrap to the same position in the vector
                    // and we try to correct it later when writing the value back
                    const IndexSizeT curWriteIndex = curWritePos % this->size;

                    while(AtomicLoad(&this->indices[curWriteIndex]) != UnusedPosition) {
                        std::this_thread::yield();
                    }

                    // add back 1 that was subtracted before
                    AtomicStore(&this->indices[curWriteIndex], index+1);


                    return;
                }
            }
        }
    }

    FORBID_COPY_MOVE_ASSIGN(ConcurrentIndexer);

    ~ConcurrentIndexer() = default;

};


} // namespace dxpool
#endif