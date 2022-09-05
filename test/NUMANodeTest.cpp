#include <catch2/catch_test_macros.hpp>
#include "../src/NUMANode.h"

using namespace dxpool;

TEST_CASE("NUMA node object representation") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)

    SECTION("NUMA node is empty by default") {
        NUMANode node;

        REQUIRE(node.Empty());
    }

    SECTION("NUMA node with ID") {
        const unsigned int nodeID = 22;
        NUMANode node(nodeID, std::set<Core> {});

        REQUIRE(node.GetID() == nodeID);

        NUMANode nodeAfterSetID;
        nodeAfterSetID.SetID(nodeID);

        REQUIRE(nodeAfterSetID.GetID() == nodeID);
    }

    SECTION("NUMA node with Cores") {
        const unsigned int nodeID = 1;
        const std::set<Core> cores{Core(1), Core(2)};

        NUMANode node(nodeID, cores);

        REQUIRE(node.GetID() == nodeID);
        REQUIRE(cores == node.Cores());

        NUMANode nodeAfterSetID;
        nodeAfterSetID.SetID(nodeID);

        REQUIRE(nodeAfterSetID.GetID() == nodeID);
    }

    SECTION("NUMA node object equal operator") {
        const std::set<Core> cores1{Core(1)};
        const std::set<Core> cores2{Core(2)};

        NUMANode node1(1, cores1);
        NUMANode node2(2, cores2);
        NUMANode nodeEqualsTo2(2, cores2);
        NUMANode nodeID2DifferentCores(2, cores1);

        REQUIRE_FALSE(node1 == node2);
        REQUIRE_FALSE(node2 == nodeID2DifferentCores);

        REQUIRE(node2 == nodeEqualsTo2);
    }

    SECTION("NUMA node less than operator") {
        NUMANode node1(1, {});
        NUMANode node2(2, {});

        REQUIRE(node1<node2);
    }
}
