#ifndef TEST_TYPES_H
#define TEST_TYPES_H

#include "../src/TypePolicies.h"


template<bool ResetFlag=true>
class ResetableCopyMoveObject {
  private:
    int value{0};
    bool doReset{ResetFlag};
    bool wasReset{false};
  public:
    const constexpr static int DefaultNonCopiableObjectValue = 18;

    ResetableCopyMoveObject():value(DefaultNonCopiableObjectValue) {}
    ResetableCopyMoveObject(int dataValue):value(dataValue) {};

    ResetableCopyMoveObject(const ResetableCopyMoveObject&) = default;
    auto operator=(const ResetableCopyMoveObject&) -> ResetableCopyMoveObject& = default;

    ResetableCopyMoveObject(ResetableCopyMoveObject&&)  noexcept = default;
    auto operator=(ResetableCopyMoveObject&&)  noexcept -> ResetableCopyMoveObject& = default;

    auto Value() const -> int {
        return this->value;
    }

    auto WasReset() const -> bool {
        return this->wasReset;
    }

    auto Reset() -> void {
        if(this->doReset) {
            this->wasReset = true;
            this->value=0;
        }
    }

    ~ResetableCopyMoveObject() = default;
};

class ResetableNoCopyMoveObject: public ResetableCopyMoveObject<true> {
  public:
    ResetableNoCopyMoveObject(int dataValue): ResetableCopyMoveObject(dataValue) {};
    ResetableNoCopyMoveObject(): ResetableCopyMoveObject(DefaultNonCopiableObjectValue) {}

    FORBID_COPY_MOVE_ASSIGN(ResetableNoCopyMoveObject);

    ~ResetableNoCopyMoveObject() = default;
};

#endif // TEST_TYPES_H