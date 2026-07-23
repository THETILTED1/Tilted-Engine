export module Tilted.Util;

import std;
import Tilted.Consts;

// Foundation utilities (namespace Tilted::Util)

namespace Tilted::Util {

template <typename T, std::size_t... Dims> struct TableType {
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

// splitmix64 (Vigna): counter-based, so any seed is valid and the whole device
// runs in constant evaluation -- one stream can fill the Zobrist tables
export class Random {
  public:
    constexpr Random(Hash seed = 0x9E3779B97F4A7C15ULL) : state(seed) {}

    constexpr Hash operator()() {
        Hash z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

  private:
    Hash state;
};

struct Empty{};

} // namespace Tilted::Util
