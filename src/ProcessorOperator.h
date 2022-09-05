#ifndef _PROCESSOR_OPERATOR_H
#define _PROCESSOR_OPERATOR_H

#include <set>

#include "Core.h"
#include "NUMANode.h"

namespace dxpool {

/**
 * @brief Interface to be implemented for handling processor (CPU) operations on specific platforms
 */
class ProcessorOperator {
  public:

    ProcessorOperator() = default;
    /**
     * @brief Default copy constructor
     *
     */
    ProcessorOperator(const ProcessorOperator &) = default;
    /**
     * @brief Default move constructor
     *
     */
    ProcessorOperator(ProcessorOperator &&) = default;
    /**
     * @brief Default copy assignment
     *
     */
    auto operator=(const ProcessorOperator &) -> ProcessorOperator & = default;
    /**
     * @brief Default move assignment
     *
     */
    auto operator=(ProcessorOperator &&) -> ProcessorOperator & = default;

    /**
     * @brief Find all available cores that the process can execute on.
     * If the process is restricted to a subset of cores, only those cores will be returned.
     *
     * @return set of available cores
     */
    virtual auto FindAvailableCores() const -> std::set<Core> = 0;

    /**
     * @brief Find all all NUMA nodes and related cores that this process can execute on
     * Only NUMA nodes where the process can run on will be returned.
     * This means that even if various NUMA nodes are available, if the process cannot execute on any core associated
     * to those nodes, the NUMA node will not be included in the result set.
     *
     * @return std::set<NUMANode>
     */
    virtual auto FindAvailableNumaNodes() const -> std::set<NUMANode> = 0;

    /**
     * @brief Set the core affinity of the running thread
     *
     * @param cores to which the affinity will be set
     * @return true if it was possible to set the core affinity
     * @return false otherwise
     */
    virtual auto SetThreadAffinity(const std::set<Core>& cores) const -> bool = 0;

    virtual ~ProcessorOperator() = default;
};


} // namespace dxpool
#endif