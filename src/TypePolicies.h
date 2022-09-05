#ifndef TYPE_POLICIES_H
#define TYPE_POLICIES_H

// NOLINTNEXTLINE(bugprone-macro-parentheses,cppcoreguidelines-macro-usage)
#define FORBID_COPY_MOVE_ASSIGN(ClassName)  \
    ClassName(const ClassName&) = delete; \
    ClassName(ClassName&&) = delete; \
    auto operator=(const ClassName&) -> ClassName& = delete; \
    auto operator=(ClassName&&) ->ClassName& = delete

// NOLINTNEXTLINE(bugprone-macro-parentheses,cppcoreguidelines-macro-usage)
#define DEFAULT_COPY_MOVE_ASSIGN(ClassName)  \
    ClassName(const ClassName&) = default; \
    ClassName(ClassName&&) = default; \
    auto operator=(const ClassName&) -> ClassName& = default; \
    auto operator=(ClassName&&) ->ClassName& = default


#endif // TYPE_POLICIES_H