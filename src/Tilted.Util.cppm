export module Tilted.Util;

import std;

// Foundation utilities: engine-agnostic infrastructure depending only on
// Tilted.Std, so any unit may import it without introducing a cycle. Its public
// surface is grouped under namespace Tilted::Util (like Tilted::Attacks) and
// will house meta-programming aliases, concurrency primitives, and PRNG state as
// the engine grows. Deliberately not a grab-bag -- bitboard bit-ops live on
// Bitboard<M, N>, and domain types stay in their own units. Anything added here
// stays parameterized rather than reaching for engine constants/types (slot
// counts and seeds are passed in, never pulled from Consts), or the leaf stops
// being a leaf.
//
// The module name (Tilted.Util) and the namespace (Tilted::Util) are independent
// in C++ -- they coincide here by choice, not necessity (cf. module Tilted.Std,
// whose contents live in std). In a module the `export` keyword is the
// public/private boundary, so an unexported helper needs no `detail` namespace
// to hide it: naming an importer's Tilted::Util::TableType simply fails.

namespace Tilted::Util {

// Unexported helper: builds the nested std::array type. Hidden from importers by
// the module boundary, not by its namespace.
template <typename T, std::size_t... Dims>
struct TableType {
    using type = T;
};

template <typename T, std::size_t First, std::size_t... Rest>
struct TableType<T, First, Rest...> {
    using type = std::array<typename TableType<T, Rest...>::type, First>;
};

// Table<T, D0, ..., Dk> is the (k+1)-dimensional std::array
//   std::array<std::array<...std::array<T, Dk>..., D1>, D0>.
// Row-major: the leftmost extent is the outermost array, so a Table<T, A, B> is
// indexed t[a][b] with a in [0, A). Table<T> degenerates to T.
export template <typename T, std::size_t... Dims>
using Table = typename TableType<T, Dims...>::type;

} // namespace Tilted::Util
