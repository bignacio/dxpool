#ifndef PROCESSOR_UNSUPPORTED_H
#define PROCESSOR_UNSUPPORTED_H

#include <set>
#include <stdexcept>

#include "Core.h"
#include "NUMANode.h"
#include "ProcessorOperator.h"

namespace dxpool {


class PlatformUnsupportedError: public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

/**
 * @brief Default processor operator for unsupported platforms
 *
 */
class ProcessorUnsupported final: public ProcessorOperator {
  public:

    /**
     * @brief Unsupported method call. Will throw PlatformUnsupportedError
     *
     * @return
     */
    auto FindAvailableCores() const -> std::set<Core> override {
        throw PlatformUnsupportedError("not implemented for this platform");
    };

    /**
    * @brief Unsupported method call. Will throw PlatformUnsupportedError
    *
    * @return
    */
    auto FindAvailableNumaNodes() const -> std::set<NUMANode> override {
        throw PlatformUnsupportedError("not implemented for this platform");
    }

    /**
    * @brief Unsupported method call. Will throw PlatformUnsupportedError
    *
    * @return
    */
    auto SetThreadAffinity(const std::set<Core>& cores) const -> bool {
        throw PlatformUnsupportedError("not implemented for this platform");
    }

};

#endif // PROCESSOR_UNSUPPORTED_H