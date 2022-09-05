#ifndef INDEX_HOLDER_H
#define INDEX_HOLDER_H

#include <cstddef>

namespace dxpool {

using IndexSizeT = std::size_t;

/**
 * @brief Helper class to track indice values allowing representing empty index
 *
 */
class IndexHolder final {
  private:
    IndexSizeT value{0};
    bool empty{true};
  public:
    /**
    * @brief Construct a new IndexHolder object initialized with the index value
    *
    * @param idxValue the index to be stored
    */
    constexpr IndexHolder(IndexSizeT idxValue) : value(idxValue), empty(false) {
    }


    /**
    * @brief Construct an emtpy IndexHolder object
    *
    */
    constexpr IndexHolder() = default;


    /**
     * @brief Returns true if there's no index available
     *
     */
    auto Empty() const -> bool   {
        return this->empty;
    }

    /**
     * @brief Returns the index being held by the holder object.
     * If there's index set, the returned value is so Empty() should always be called first.
     *
     * @return the index value set in this holder
     */
    auto Get() const -> IndexSizeT {
        return this->value;
    }

    /**
     * @brief Default move constructor
     *
     */
    IndexHolder(IndexHolder&&) noexcept = default;
    /**
     * @brief Default move operator
     *
     * @return IndexHolder&
     */
    auto operator=(IndexHolder&&) noexcept -> IndexHolder& = default;

    // copy construction and assignment are forbidden
    IndexHolder(IndexHolder&) = delete;
    auto operator=(IndexHolder&) -> IndexHolder& = delete;

    ~IndexHolder() = default;
};

}

#endif // INDEX_HOLDER_H