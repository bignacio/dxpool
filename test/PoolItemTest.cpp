#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <utility>

#include "../src/PoolItem.h"
#include "TestTypes.h"

using namespace dxpool;
using namespace std;

auto NoOpPoolItemDestroyCb(const PoolItem<ResetableNoCopyMoveObject>& _ignoredItem) -> void {
    (void)_ignoredItem;
}

TEST_CASE("Pool item wrapper object", "[poolitem]") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)


    SECTION("Empty object by default") {
        PoolItem<ResetableNoCopyMoveObject> item;
        REQUIRE(item.Empty());
    }

    SECTION("Get with value") {
        const int dataValue = 43;
        ResetableNoCopyMoveObject obj(dataValue);
        PoolItem<ResetableNoCopyMoveObject> item(NoOpPoolItemDestroyCb, &obj, 0);

        REQUIRE_FALSE(item.Empty());
        const ResetableNoCopyMoveObject* retrieved = item.Get();

        REQUIRE(retrieved->Value() == dataValue);
    }

    SECTION("Notify on destruction") {
        const int dataValue = 77;
        const IndexSizeT poolIndex = 302;
        ResetableNoCopyMoveObject obj(dataValue);

        IndexSizeT actualPoolIndex = 0;
        bool onDestroyCalled = false;

        {
            auto onDestroy = [&onDestroyCalled, &actualPoolIndex](const PoolItem<ResetableNoCopyMoveObject>& destroyedItem) {
                onDestroyCalled = true;
                actualPoolIndex = destroyedItem.PoolIndex();
            };
            PoolItem<ResetableNoCopyMoveObject> item(onDestroy, &obj, poolIndex);
        }

        REQUIRE(onDestroyCalled);
        REQUIRE(actualPoolIndex == poolIndex);
    }

    SECTION("Move constructor without data") {
        PoolItem<ResetableNoCopyMoveObject> original;
        PoolItem<ResetableNoCopyMoveObject> moved(std::move(original));

        REQUIRE(original.Empty()); // NOLINT(bugprone-use-after-move)

        REQUIRE(original.Get() == nullptr); // NOLINT(clang-analyzer-cplusplus.Move)

        REQUIRE(moved.Empty());
        REQUIRE(moved.Get()==nullptr);
    }

    SECTION("Move constructor with data") {
        const int dataValue = 552;
        const IndexSizeT poolIndex = 1;
        ResetableNoCopyMoveObject data(dataValue);

        int destroyCount = 0;
        auto onDestroy = [&destroyCount](const PoolItem<ResetableNoCopyMoveObject>& _unusedItem) {
            (void)_unusedItem;
            destroyCount++;
        };

        {
            PoolItem<ResetableNoCopyMoveObject> original(onDestroy, &data, poolIndex);
            PoolItem<ResetableNoCopyMoveObject> moved(std::move(original));

            REQUIRE(original.Empty()); // NOLINT(bugprone-use-after-move)
            REQUIRE(original.Get() == nullptr); // NOLINT(clang-analyzer-cplusplus.Move)

            REQUIRE_FALSE(moved.Empty());
            REQUIRE(moved.Get()->Value() == dataValue);
        }

        // we should only have destroyed one of the items
        REQUIRE(destroyCount == 1);
    }
}