export module Tilted.Consts;

import std;

export namespace Tilted {

using Square = std::size_t;
enum Color { Black, White };

using Score = int;
using Depth = int;

using Hash = std::uint64_t;

inline constexpr Score VICTORY = 20000;
inline constexpr Score DRAW = 0;

inline constexpr Depth MAX_PLY = 128;

inline constexpr std::size_t MAX_THREADS = 8;

// Standard pieces first, then fairy pieces in ascending order of value.
enum class Piece {
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
    Alfil,
    Dabbaba,
    Ferz,
    Wazir,
    Camel,
    Grasshopper,
    Horse,
    Wildebeest,
    Dragon,
    General,
    Archbishop,
    Chancellor,
    Amazon,
};

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

} // namespace Tilted
