#ifndef POOL_ITEM_TYPE_H
#define POOL_ITEM_TYPE_H

#include <cstddef>
#include <functional>
#include <type_traits>


#include "IndexHolder.h"
#include "TypePolicies.h"

namespace dxpool {

/**
 * @brief Helper class used to hold and control the lifecycle of items in an object pool
 *
 * @tparam ItemType Type of the item in an object pool
 */
template<typename ItemType>
class PoolItem final {
  public:
    /**
     * @brief Callback to be invoked with PoolItem is destructed
     *
     */
    using OnPoolItemDestroyCallbackT = std::function<void(const PoolItem<ItemType>&)>;

  private:
    OnPoolItemDestroyCallbackT onDestroyCallback;

    ItemType* item{nullptr};
    IndexSizeT index{0};
    bool empty{true};

    static inline auto NoOpItemDestroyCallback(const PoolItem<ItemType>& _ignored) -> void {
        (void) _ignored; // make the linter happy
    }

    static inline auto CheckType() -> void {
        static_assert(!std::is_pointer<ItemType>::value, "ItemType must not be a pointer");
        static_assert(!std::is_reference<ItemType>::value, "ItemType must not be a reference");
    }

  public:
    /**
     * @brief Construct a new Pool Item object with an usable item and its index in the pool
     *
     * @param onDestroyCb callback to be invoked when the pool item is destroyed
     * @param poolItem item retrieved from the pool and ready to be used
     * @param poolIndex index of the item in the pool
     */
    PoolItem(const OnPoolItemDestroyCallbackT onDestroyCb, ItemType* poolItem, IndexSizeT poolIndex) {
        CheckType();
        this->onDestroyCallback = onDestroyCb;
        this->item = poolItem;
        this->index = poolIndex;
        this->empty = (item == nullptr);
    }

    /**
     * @brief The default constructor creates an empty Pool Item object
     *
     */
    PoolItem(): PoolItem(NoOpItemDestroyCallback, nullptr, 0) {
        CheckType();
    }

    /**
     * @brief Move constructor for a pool item. Afert moved, the pool item is considered empty and the item held is null
     * It's allowed to move construct but not move assign or copy pool items
     * @param movedPoolItem
     */
    PoolItem(PoolItem&& movedPoolItem) noexcept {
        this->item = movedPoolItem.item;
        this->empty = movedPoolItem.empty;
        this->index = movedPoolItem.index;
        this->onDestroyCallback = movedPoolItem.onDestroyCallback;

        movedPoolItem.item = nullptr;
        movedPoolItem.empty = true;
        movedPoolItem.onDestroyCallback = NoOpItemDestroyCallback;
    }

    auto operator=(PoolItem<ItemType>&& poolItem) -> PoolItem<ItemType>& = delete;
    PoolItem(PoolItem&) = delete;
    auto operator=(PoolItem<ItemType>&) -> PoolItem<ItemType> = delete;

    /**
     * @brief Returns true if there is no item held by this object (retrieved from the pool)
     *
     */
    auto Empty() const -> bool {
        return this->empty;
    }

    /**
     * @brief Returns a pointer to the item held by this object.
     * If there is no item held (i.e, Empty() is true) the behaviour of this function is undefined
     *
     */
    auto Get() const -> ItemType* {
        return this->item;
    }


    /**
     * @brief Returns the index of the object in the pool. If there is no item held, the returned value is undefined
     *
     * @return IndexSizeT index position
     */
    auto PoolIndex() const -> IndexSizeT {
        return this->index;
    }

    /**
     * @brief Destruction resets the held by this object.
     & Note that the destroy callback is only invoked if there's an item held
     *
     */
    ~PoolItem() {
        this->onDestroyCallback(*this);
    }

};
} //namespace dxpool

#endif // POOL_ITEM_TYPE_H