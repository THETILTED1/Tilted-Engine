#pragma once

import Tilted.Std;

namespace Tilted {

using Square = std::size_t;
enum Color { Black, White };

using Score = int;
using Depth = int;

inline constexpr Score VICTORY = 20000;
inline constexpr Score DRAW = 0;

inline constexpr Depth MAX_PLY = 128;

inline constexpr std::size_t MAX_THREADS = 8;

enum class Variant {
    Chess,
    ThreeCheck,
    Horde,
    KingOfTheHill,
    Chaturanga,
    Paradigm,
    MiniForest, // 8x8
    XXL,
    Gothic,
    BehindTheMirror, // different
    Setup,
    TinyHouse,
    CrazyHouse,
    Seirawan, // droppers
    Petrified,
    Jungle,
    Duck,
    Clobber,
    Cloister, // etc
};

template <std::size_t M, std::size_t N>
concept ValidBoard = (M == 4 and N == 4) or (M == 8 and N == 8) or
                     (M == 8 and N == 10) or (M == 14 and N == 14);

} // namespace Tilted
