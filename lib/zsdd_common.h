#ifndef ZSDD_COMMON_H_
#define ZSDD_COMMON_H_
#include <utility>
#include <unordered_set>

namespace zsdd {
using addr_t = long long int;


constexpr addr_t ZSDD_FALSE = -2;
constexpr addr_t ZSDD_EMPTY = -1;
constexpr addr_t ZSDD_NULL = -3;

enum class NodeType : char{LIT, DECOMP, UNUSED};

enum  class Operation : char
{
    NULLOP,
    UNION,
    INTERSECTION,
    DIFFERENCE,
    CHANGE,
    ORTHOGONAL_JOIN,
    FILTER_NOT_CONTAIN,
    FILTER_CONTAIN,
    POWER_SET,
    EXPLICIT_FORM,
};


inline void hash_combine(size_t& seed, size_t value) {
    seed ^= value + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

} // namespce zsdd
#endif // ZSDD_COMMON_H
