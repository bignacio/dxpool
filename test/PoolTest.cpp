#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <functional>
#include <vector>
#include <memory>

#include "../src/PoolItem.h"
#include "../src/MutexIndexer.h"
#include "../src/Pool.h"
#include "TestTypes.h"


using namespace dxpool;

template<typename PoolType, typename ItemType>
auto verifySimpleTakeReturn(PoolType& pool, const IndexSizeT expectedSize) -> void {
    REQUIRE(expectedSize == pool.Size());

    PoolItem<ItemType> result = pool.Take();
    REQUIRE_FALSE(result.Empty());

    ItemType* obj = result.Get();

    REQUIRE(obj->Value()==ResetableCopyMoveObject<>::DefaultNonCopiableObjectValue);
}


template<typename PoolType, typename ItemType>
auto verifyTakeAll(PoolType& pool) -> void {
    REQUIRE(pool.Size() > 0);

    std::vector<PoolItem<ItemType>> takenItems;

    for(size_t i = 0 ; i < pool.Size() ; i++) {
        PoolItem<ItemType> result = pool.Take();
        REQUIRE_FALSE(result.Empty());
        // keep taken items so they don't get returned automatically when result goes out of scope
        takenItems.push_back(std::move(result));
    }

    REQUIRE(pool.Size() == takenItems.size());
    PoolItem<ItemType> result = pool.Take();
    REQUIRE(result.Empty());
}


template<typename PoolType, typename ItemType>
auto verifyReturnAfterItemOutOfScope(PoolType& pool) -> void {
    REQUIRE(pool.Size() > 0);

    for(size_t i = 0 ; i < pool.Size() ; i++) {
        PoolItem<ItemType> result = pool.Take();
        REQUIRE_FALSE(result.Empty());
        // the item should be returned to the pool automatically via RAII
    }

    PoolItem<ItemType> result = pool.Take();
    REQUIRE_FALSE(result.Empty());
}

template<typename PoolType, typename ...Args>
auto verifyPollItemStateReset(Args... ctorArgs) -> void {
    const int resetValue = 42;
    // keep a pointer to the pool to be created later
    PoolType* poolHandle = nullptr;

    auto customResetCb = [&poolHandle](int* item) {
        // since the pool has size one, here it should be empty as we want reset to be invoked before the item is returned to the pool
        REQUIRE(poolHandle->Take().Empty());

        *item = resetValue;
    };

    PoolType pool(ctorArgs..., customResetCb);
    poolHandle = &pool;
    int* value = nullptr;
    {
        auto item = pool.Take();
        value = item.Get();
    }

    REQUIRE(*value == resetValue);
}

TEST_CASE("Pool item state when returned to pool") { //NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)

    SECTION("Reset on destruction") {
        StaticPool<ResetableNoCopyMoveObject, 1> pool;
        ResetableNoCopyMoveObject* obj = nullptr;
        {
            auto item = pool.Take();
            obj = item.Get();
            REQUIRE_FALSE(item.Empty());
        }

        REQUIRE_FALSE(obj == nullptr);
        // we're using the item after having returned it to the pool
        // While the pointer is still valid, this is not a valid pattern and should not be ued
        REQUIRE(obj->WasReset());
        REQUIRE(obj->Value()==0);
    }

    SECTION("Invoke custom reseter on static pool") {
        verifyPollItemStateReset<StaticPool<int, 1>>();
    }


    SECTION("Invoke custom reseter on runtime pool") {
        verifyPollItemStateReset<RuntimePool<int>>(static_cast<size_t>(1));
    }
}

TEST_CASE("Static pool tests") { //NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    SECTION("Take and return no copy, no move object") {
        constexpr const IndexSizeT poolSize = 3;
        StaticPool<ResetableNoCopyMoveObject, poolSize> pool;
        verifySimpleTakeReturn<decltype(pool), ResetableNoCopyMoveObject>(pool, poolSize);
    }

    SECTION("Take all items") {
        constexpr const IndexSizeT poolSize = 5;
        StaticPool<ResetableNoCopyMoveObject, poolSize> pool;
        verifyTakeAll<decltype(pool), ResetableNoCopyMoveObject>(pool);
    }

    SECTION("Return when out of scope") {
        constexpr const IndexSizeT poolSize = 21;
        StaticPool<ResetableNoCopyMoveObject, poolSize> pool;
        verifyReturnAfterItemOutOfScope<decltype(pool), ResetableNoCopyMoveObject>(pool);
    }
}

TEST_CASE("Runtime pool tests") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    SECTION("Take and return copy and move object") {
        constexpr const IndexSizeT poolSize = 12;
        RuntimePool<ResetableCopyMoveObject<>, MutexIndexer> pool(poolSize);
        verifySimpleTakeReturn<decltype(pool), ResetableCopyMoveObject<>>(pool, poolSize);
    }

    SECTION("Take all items") {
        constexpr const IndexSizeT poolSize = 6;
        RuntimePool<ResetableCopyMoveObject<>, MutexIndexer> pool(poolSize);
        verifyTakeAll<decltype(pool), ResetableCopyMoveObject<>>(pool);
    }

    SECTION("Return when out of scope") {
        constexpr const IndexSizeT poolSize = 18;
        RuntimePool<ResetableCopyMoveObject<>, MutexIndexer> pool(poolSize);
        verifyReturnAfterItemOutOfScope<decltype(pool), ResetableCopyMoveObject<>>(pool);
    }
}