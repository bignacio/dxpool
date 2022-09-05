#ifndef PROCESSOR_LINUX_H
#define PROCESSOR_LINUX_H

#include <thread>
#include <pthread.h>
#include <sched.h>
#include <set>
#include <map>
#include <functional>

#include "Core.h"
#include "NUMANode.h"
#include "ProcessorOperator.h"

namespace dxpool {

/**
 * @brief The ProcessorLinux class provides information about CPU cores and NUMA nodes on Linux.
 *
 */
class ProcessorLinux final: public ProcessorOperator {
  private:
    static constexpr const size_t CPUSetSize = sizeof(cpu_set_t);
    static constexpr const unsigned int MaxCoreCount = 1<<10; // max 1024 cores

    static auto getCurrentThreadCoresAffinityMask() -> cpu_set_t {
        cpu_set_t cpuSetMask;

        CPU_ZERO_S(CPUSetSize, &cpuSetMask);

        if(pthread_getaffinity_np(pthread_self(), ProcessorLinux::CPUSetSize, &cpuSetMask) == 0) {
            return cpuSetMask;
        }

        return cpu_set_t{};
    }

    static auto ForEachCPUSet(const std::function<void(unsigned int)>& cpuIDFn) -> void {
        auto cpuSetMask = ProcessorLinux::getCurrentThreadCoresAffinityMask();

        for(unsigned int i = 0 ; i < ProcessorLinux::MaxCoreCount; i++) {
            if(CPU_ISSET_S(i, ProcessorLinux::CPUSetSize, &cpuSetMask)) { // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                cpuIDFn(i);
            }
        }
    }
  public:

    /**
     * @see ProcessorOperator::FindAvailableCores
     */
    auto FindAvailableCores() const -> std::set<Core> override {
        std::set<Core> cores;

        ProcessorLinux::ForEachCPUSet([&cores](unsigned int coreID) {
            cores.emplace(coreID);
        });

        return cores;
    }

    /**
     * @see ProcessorOperator::FindAvailableNumaNodes
     */
    auto FindAvailableNumaNodes() const -> std::set<NUMANode> override {
        std::map<unsigned int, std::set<Core>> nodeCoreMap;

        auto originalCPUSetMask = ProcessorLinux::getCurrentThreadCoresAffinityMask();

        auto collectNUMANodes = [&nodeCoreMap](unsigned int coreID) {
            cpu_set_t currentCoreMask;
            CPU_ZERO(&currentCoreMask);
            CPU_SET_S(coreID, ProcessorLinux::CPUSetSize, &currentCoreMask); //NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

            int result = pthread_setaffinity_np(pthread_self(), ProcessorLinux::CPUSetSize, &currentCoreMask);

            if(result == 0) {
                unsigned int currentCoreID{0};
                unsigned int numaNodeID{0};
                if(getcpu(&currentCoreID, &numaNodeID) == 0) {
                    auto coresInNode = nodeCoreMap.find(numaNodeID);
                    if(coresInNode == nodeCoreMap.end()) {
                        nodeCoreMap[numaNodeID] = {};
                    }

                    nodeCoreMap[numaNodeID].insert(currentCoreID);
                }
            }
        };

        ProcessorLinux::ForEachCPUSet(collectNUMANodes);

        std::set<NUMANode> nodes;

        for(const auto& entry: nodeCoreMap) {
            NUMANode node(entry.first, entry.second);
            nodes.insert(node);
        }

        // restore original thread affinity
        pthread_setaffinity_np(pthread_self(), ProcessorLinux::CPUSetSize, &originalCPUSetMask);
        return nodes;
    }

    /**
     * @see Processor::SetThreadAffinity
     */
    auto SetThreadAffinity(const std::set<Core>& cores) const -> bool override {
        cpu_set_t cpuSetMask;

        CPU_ZERO_S(CPUSetSize, &cpuSetMask);

        for(const auto& core: cores ) {
            CPU_SET_S(core.GetID(), ProcessorLinux::CPUSetSize, &cpuSetMask); //NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        return pthread_setaffinity_np(pthread_self(), ProcessorLinux::CPUSetSize, &cpuSetMask) == 0;
    }

};

} // namespace dxpool
#endif