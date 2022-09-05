#include <catch2/catch_test_macros.hpp>
#include "../src/Core.h"

using namespace dxpool;

TEST_CASE("Core object representation") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)

    SECTION("Core with ID") {
        const unsigned int coreID = 44;
        Core core(coreID);

        REQUIRE(core.GetID() == coreID);
    }

    SECTION("Core object equal operator") {
        Core core1(1);
        Core core2(2);
        Core coreEqualsTo1(1);

        REQUIRE_FALSE(core1 == core2);
        REQUIRE(core1 == coreEqualsTo1);
    }

    SECTION("Core object less than operator") {
        Core core1(1);
        Core core2(2);

        REQUIRE(core1<core2);
    }
}
