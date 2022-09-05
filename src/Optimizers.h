#ifndef OPTIMIZER_H
#define OPTIMIZER_H

namespace dxpool {

template<typename Exp>
constexpr auto unlikely(Exp exp) -> bool {
    return __builtin_expect (exp, 0);
}

template<typename Exp>
constexpr auto likely(Exp exp) -> bool {
    return __builtin_expect (exp, 1);
}

} // namespace dxpool

#endif // OPTIMIZER_H