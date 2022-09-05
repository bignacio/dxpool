#ifndef DATA_POOL_H
#define DATA_POOL_H

#include <type_traits>
#include <vector>
#include <array>
#include <algorithm>
#include <functional>

#include "IndexHolder.h"
#include "TypePolicies.h"
#include "MutexIndexer.h"
#include "ConcurrentIndexer.h"
#include "PoolItem.h"

namespace dxpool {

template <typename MaybeResetable> auto IsResetableTypeCondition(char) -> decltype(std::declval<MaybeResetable>().Reset(), std::true_type {});
template <typename MaybeResetable> auto IsResetableTypeCondition(...) -> std::false_type;

template <typename MaybeResetable>
using IsResetableType = decltype(IsResetableTypeCondition<MaybeResetable>(0));



/**
 * @brief A fixed size, thread safe data and object pool
 *
 * The pool starts with a fixed number of items and it cannot be resized dynamically.
 * Items in the pool are never moved or haved their memory position changed.
 * This means that consumers will always work with references to items, while the pool
 * is responsible for item lifetime.
 *
 * This means that the type of items in the pool need to be trivially constructible.
 * For the full benefits of cache locality, avoid sharing pools between threads on different cores.
 *
 * ItemType's destructor is only invoked when the pool is destroyed.
 *
 * A pool cannot be copied or moved.
 *
 * @tparam ItemType type of the data to be stored
 * @tparam Indexer type of indexer to be used to when retrieving and returning objects.
           The indexer defines the concurrency behaviour when using the pool in a multi-threaded context
 * @tparam FixedPoolSize size of the pool, if specified as fixed
 */

template<typename ItemType, typename ItemContainerType, typename Indexer = MutexIndexer, IndexSizeT FixedPoolSize = 0>
class Pool final {
  private:
    using CustomResetCallbackT = std::function<void(ItemType*)>;
    CustomResetCallbackT customResetCallback= [](ItemType*) {};

    ItemContainerType items = {};
    Indexer indexer;

    template<typename ResetableType,  typename std::enable_if<IsResetableType<ResetableType>::value>::type* = nullptr>
    auto InvokeReset(ResetableType* item)-> void {
        item->Reset();
    }

    template<typename NonResetableType,  typename std::enable_if<!IsResetableType<NonResetableType>::value>::type* = nullptr>
    auto InvokeReset(NonResetableType* item) -> void {
        this->customResetCallback(item);
    }

  public:
    /**
     * @brief Construct a new object Pool if ItemType is move constructible
     *
     * @param numItems number of items in the pool
     */
    template<typename CTORItemType = ItemType, typename = typename std::enable_if<std::is_move_constructible<CTORItemType>::value>::type>
    Pool(std::size_t  numItems): indexer(numItems) {
        for(size_t i = 0 ; i < numItems ; i++) {
            this->items.emplace_back();
        }
    }

    /**
    * @brief Construct a new object Pool if ItemType is move constructible
    *
    * @param numItems number of items in the pool
    * @param resetCb item state reset callback to be invoked immediately before the item is returned to the pool
    */
    template<typename CTORItemType = ItemType, typename = typename std::enable_if<std::is_move_constructible<CTORItemType>::value>::type>
    Pool(std::size_t  numItems, const CustomResetCallbackT& resetCb):Pool(numItems) {
        this->customResetCallback = resetCb;
    }


    /**
     * @brief Default static pool constructor
     *
     */
    Pool(): indexer(FixedPoolSize) {

    }

    /**
     * @brief Static pool constructor with custom reset callback
     *
     * @param resetCb item state reset callback to be invoked immediately before the item is returned to the pool
     */
    Pool(const CustomResetCallbackT& resetCb): Pool() {
        this->customResetCallback = resetCb;
    }

    /**
     * @brief Remove and return an element from the pool, if not empty
     *
     * @return PoolItem<ItemType> will be e mpty if there are no more items in the pool
     */
    inline auto Take() -> PoolItem<ItemType> {
        auto holder = this->indexer.Next();

        if(holder.Empty()) {
            return {};
        }

        auto returnToPoolFn = [this](const PoolItem<ItemType>& item) -> void{
            this->InvokeReset(item.Get());
            this->indexer.Return(item.PoolIndex());
        };

        const auto index = holder.Get();
        auto* item = &this->items.at(index);
        return PoolItem<ItemType>(returnToPoolFn, item, index);
    }

    /**
     * @brief Returns the total size of the pool when created
     *
     * @return size_t size of the pool
     */
    auto Size() const -> size_t {
        return this->items.size();
    }

    FORBID_COPY_MOVE_ASSIGN(Pool);
    virtual ~Pool() = default;
}; // class Pool


/**
 * @brief Alias for a pool backed by an static array with static size
 *
 * @tparam ItemType Type of the items in the pool
 * @tparam Indexer type of indexer to be used to when retrieving and returning objects.
            The indexer defines the concurrency behaviour when using the pool in a multi-threaded context
 * @tparam Size Size of pool specified at compile time
 */
template<typename ItemType, IndexSizeT Size, typename Indexer = MutexIndexer>
using StaticPool= Pool<ItemType, std::array<ItemType, Size>, Indexer, Size>;

/**
 * @brief Alias for a pool backed by a vector. The size of the pool however cannot be modified once created
 *
 * @tparam ItemType Type of the items in the pool
 * @tparam Indexer type of indexer to be used to when retrieving and returning objects.
           The indexer defines the concurrency behaviour when using the pool in a multi-threaded context
 */
template<typename ItemType, typename Indexer = MutexIndexer>
using RuntimePool= Pool<ItemType, std::vector<ItemType>, Indexer>;


} // namespace dxpool
#endif
