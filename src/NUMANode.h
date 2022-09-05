#ifndef NUMA_NODE_H
#define NUMA_NODE_H

#include "TypePolicies.h"
#include "Core.h"

#include <set>
#include <utility>


namespace dxpool {



/**
* @brief Representation of a NUMA node
*
*/
class NUMANode final {
  private:
    std::set<Core> cores;
    unsigned int numaNodeID{0};
    bool empty{true};
  public:
    NUMANode() = default;

    /**
    * @brief Construct a new NUMA node object with an ID, setting its Empty() state to false
    *
    * @param nodeID ID of the NUMA node
    * @param cpuCores set of cores in this NUMA node
    */
    NUMANode(unsigned int nodeID, std::set<Core> cpuCores):cores(std::move(cpuCores)), numaNodeID(nodeID), empty(false) {}

    /**
    * @brief Determines if the NUMA node object has an ID
    *
    * @return true if no NUMA node ID was set
    * @return false if there's a valid NUMA node
    */
    auto Empty() const -> bool {
        return this->empty;
    }

    /**
    * @brief Returns the NUMA node ID set in this object
    *
    * @return the NUMA node ID
    */
    auto GetID() const -> unsigned int {
        return this->numaNodeID;
    }

    /**
    * @brief Sets or changes a NUMA node ID
    *
    * @param nodeID the NUMA node ID
    */
    auto SetID(unsigned int nodeID) -> void {
        this->numaNodeID = nodeID;
    }

    /**
     * @brief Returns the cores associated to this NUMA node
     *
     * @return set of cores
     */
    auto Cores() const -> const std::set<Core>& {
        return this->cores;
    }

    /**
    * @brief NUMA nodes are equal if their ID is the same
    *
    * @param node NUMA node to be compared
    * @return true if the NUMA node IDs are equal
    * @return false if the NUMA node IDs are different
    */
    auto operator==(const NUMANode& node) const -> bool {
        return this->numaNodeID == node.numaNodeID && this->cores == node.cores;
    }

    /**
     * @brief Less than operator compares core IDs of each NUMA node
     *
     * @param node NUMA node to be compared against
     * @return true if this node ID is less than the indicated node ID
     * @return false otherwise
     */
    auto operator<(const NUMANode& node) const -> bool {
        return this->numaNodeID < node.numaNodeID;
    }

};

} // namespace dxpool

#endif