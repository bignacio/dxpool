#include <catch2/catch_test_macros.hpp>
#include <thread>
#include <iterator>

#include "../src/Processor.h"

using namespace dxpool;
using namespace std;

class AffinityRestorer {
  private:
    set<Core> originalCores;
  public:
    AffinityRestorer(set<Core> cores) : originalCores(std::move(cores)) {}

    ~AffinityRestorer() {
        Processor processor;
        processor.SetThreadAffinity(this->originalCores);
    }


    AffinityRestorer(const AffinityRestorer &) = delete;
    AffinityRestorer(AffinityRestorer &&) = delete;
    auto operator=(const AffinityRestorer &) -> AffinityRestorer & = delete;
    auto operator=(AffinityRestorer &&) -> AffinityRestorer & = delete;
};

TEST_CASE("Processor and NUMA nodes") { // NOLINT(cppcoreguidelines-avoid-non-const-global-variables, readability-function-cognitive-complexity)

    SECTION("Get affinity all cores") {
        Processor processor;

        auto cores = processor.FindAvailableCores();

        REQUIRE_FALSE(cores.empty());

        // though hardware_concurrency is not precise, it should be ok in most cases
        // and I want to find out when that is not the case
        REQUIRE(cores.size() == thread::hardware_concurrency());
    }

    SECTION("Get NUMA nodes all cores") {
        Processor processor;

        auto originalCores = processor.FindAvailableCores();

        auto nodes = processor.FindAvailableNumaNodes();

        REQUIRE(!nodes.empty());
        REQUIRE(nodes.begin()->Cores().size() == thread::hardware_concurrency());

        auto coresAfterQueryingNodes = processor.FindAvailableCores();
        REQUIRE(originalCores == coresAfterQueryingNodes);
    }

    SECTION("Set affinity") {
        Processor processor;
        const auto allCores = processor.FindAvailableCores();

        // RAII way to restore original core affinity if the test fails
        const AffinityRestorer restorer(allCores);

        // select a subset of cores
        auto desiredAffinity = set<Core> {(*allCores.begin())};
        // check we can add 2 cores, just in case we're running this on a single core computer
        if(allCores.size()>1) {
            // add last core but it could have been any core, really
            auto lastCore = *std::prev(allCores.end());
            desiredAffinity.insert(lastCore);
        }

        REQUIRE(processor.SetThreadAffinity(desiredAffinity));

        auto actualAffinity = processor.FindAvailableCores();
        REQUIRE(actualAffinity == desiredAffinity);
    }

}