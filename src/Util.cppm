export module Util;

import std;
export import Consts;

namespace Tilted::Util {

template <typename T, std::size_t... Dims> struct TableType {
    using type = T;
};

template <typename T, std::size_t First, std::size_t... Rest>
struct TableType<T, First, Rest...> {
    using type = std::array<typename TableType<T, Rest...>::type, First>;
};

export template <typename T, std::size_t... Dims>
using Table = typename TableType<T, Dims...>::type;

// splitmix64 (Vigna)
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

export template <typename T> struct Empty {};

export template <bool On, typename T>
using Conditional = std::conditional_t<On, T, Empty<T>>;

} // namespace Tilted::Util
