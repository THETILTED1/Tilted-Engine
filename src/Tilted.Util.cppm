export module Tilted.Util;

import std;

// Foundation utilities (namespace Tilted::Util): engine-agnostic, parameterized
// helpers -- meta-programming aliases now, concurrency/PRNG later. A leaf module.

namespace Tilted::Util {

// Unexported helper: builds the nested std::array type (hidden by the module boundary).
template <typename T, std::size_t... Dims>
struct TableType {
    using type = T;
};

template <typename T, std::size_t First, std::size_t... Rest>
struct TableType<T, First, Rest...> {
    using type = std::array<typename TableType<T, Rest...>::type, First>;
};

// Table<T, D0..Dk>: (k+1)-dim nested std::array, row-major (leftmost extent
// outermost, so Table<T,A,B> indexes t[a][b]); Table<T> is T.
export template <typename T, std::size_t... Dims>
using Table = typename TableType<T, Dims...>::type;

} // namespace Tilted::Util
