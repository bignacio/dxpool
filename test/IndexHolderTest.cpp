#include <catch2/catch_test_macros.hpp>

#include "../src/IndexHolder.h"

using namespace dxpool;
using namespace std;

TEST_CASE("Optional index holder wrapper", "[indexholder]") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    SECTION("Get empty by default") {
        IndexHolder holder;
        REQUIRE(holder.Empty());
    }

    SECTION("Get with value") {
        const IndexSizeT value{7129};
        IndexHolder holder(value);

        REQUIRE_FALSE(holder.Empty());
        REQUIRE(holder.Get() == value);
    }

}