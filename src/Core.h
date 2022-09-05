#ifndef CORE_H
#define CORE_H

#include "TypePolicies.h"

namespace dxpool {


/**
* @brief Representation of a processor Core
*
*/

class Core final {
  private:
    unsigned int coreID{0};

  public:
    Core() = delete;

    DEFAULT_COPY_MOVE_ASSIGN(Core);

    /**
    * @brief Construct a new Core object with a core ID and a NUMA node, setting its Empty() state to false
    *
    * @param pCoreID the core ID
    */
    Core(unsigned int pCoreID):coreID(pCoreID) {}

    /**
    * @brief Returns the core ID set in this object
    *
    * @return the core ID
    */
    auto GetID() const -> unsigned int {
        return this->coreID;
    }

    /**
    * @brief Processor Cores are equal if their ID is the same
    *
    * @param core Core to be compared against
    * @return true if the core IDs are the same
    * @return false if the core IDs are different
    */
    auto operator==(const Core& core) const -> bool {
        return this->coreID == core.coreID;
    }

    /**
     * @brief Less than operator compares core IDs of each core
     *
     * @param core core to be compared against
     * @return true if this core ID is less than the indicated core ID
     * @return false otherwise
     */
    auto operator<(const Core& core) const -> bool {
        return this->coreID < core.coreID;
    }

    ~Core() = default;
};


} // namespace dxpool

#endif