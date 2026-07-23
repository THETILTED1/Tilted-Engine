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

inline constexpr std::size_t MAX_TYPES = 11;

// Largest board the engine supports; bounds the single shared Zobrist table.
// Any Bitboard<M, N> must satisfy M <= MAX_RANKS and N <= MAX_FILES.
inline constexpr std::size_t MAX_RANKS = 14;
inline constexpr std::size_t MAX_FILES = 14;

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
    Atomic,
    Antichess,
    ThreeCheck,
    Horde,
    KingOfTheHill,
    RacingKings,
    Chaturanga,
    Paradigm,
    MiniForest, // 8x8
    XXL,
    Gothic,
    BehindTheMirror, // different
    Setup,
    Tinyhouse,
    Crazyhouse,
    Seirawan, // droppers
    Petrified,
    Spell,
    Jungle,
    Duck,
    Clobber,
    Cloister, // etc
};

// The piece types a variant uses, in canonical order -- the single source of
// truth for both the ordering and the count (see NumPieceTypes below). if
// constexpr (not switch) so each variant's differently-sized array is the only
// live return per instantiation.
template <Variant V> inline constexpr auto PieceMapping() {
    using enum Piece;

    if constexpr (V == Variant::Chess || V == Variant::Atomic ||
                  V == Variant::Antichess || V == Variant::ThreeCheck ||
                  V == Variant::Horde || V == Variant::KingOfTheHill ||
                  V == Variant::Chaturanga || V == Variant::Paradigm ||
                  V == Variant::BehindTheMirror || V == Variant::Setup ||
                  V == Variant::Crazyhouse || V == Variant::Petrified ||
                  V == Variant::Spell || V == Variant::Duck)
        return std::to_array({Pawn, Knight, Bishop, Rook, Queen, King});

    else if constexpr (V == Variant::RacingKings)
        return std::to_array({Knight, Bishop, Rook, Queen, King});

    else if constexpr (V == Variant::MiniForest || V == Variant::Jungle)
        return std::array<Piece, 0>{}; // placeholder

    else if constexpr (V == Variant::XXL) {
        return std::to_array({Pawn, Knight, Bishop, Rook, Queen, King, Camel,
                              General, Archbishop, Chancellor, Amazon});
    }

    else if constexpr (V == Variant::Gothic || V == Variant::Seirawan)
        return std::to_array(
            {Pawn, Knight, Bishop, Rook, Queen, King, Archbishop, Chancellor});

    else if constexpr (V == Variant::Tinyhouse) {
        return std::to_array({Pawn, King, Ferz, Wazir});
    }

    else if constexpr (V == Variant::Clobber) {
        return std::to_array({Pawn, Rook, King, Wazir});
    }

    else if constexpr (V == Variant::Cloister) {
        return std::to_array({Wazir});
    }

    else {
        std::unreachable();
    }
}

// Count derives from the mapping, so the two can never drift.
template <Variant V> inline constexpr std::size_t NumPieceTypes() {
    return PieceMapping<V>().size();
}

} // namespace Tilted
