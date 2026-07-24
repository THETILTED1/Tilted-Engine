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

// Pocket (pieces in hand) for drop variants: up to MAX_IN_HAND of one kind
// (all 16 pawns) over MAX_DROP_TYPES piece-type slots -- Seirawan's 8 types
// (its gated 7th/8th included) is the widest drop variant.
inline constexpr std::size_t MAX_IN_HAND = 16;
inline constexpr std::size_t MAX_DROP_TYPES = 8;

inline constexpr std::size_t MAX_HISTORY_LEN = 256;

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
    None,
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
    MiniForest,
    XXL,
    Gothic,
    BehindTheMirror,
    Setup,
    Tinyhouse,
    Crazyhouse,
    Seirawan,
    Petrified,
    Spell,
    Jungle,
    Duck,
    Clobber,
    Cloister,
    None
};

// The piece types a variant uses, in canonical order -- the single source of
// truth for both the ordering and the count
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

    else if constexpr (V == Variant::MiniForest)
        return std::to_array({Pawn, Bishop, King, Alfil, Dabbaba, Camel, Grasshopper});

    else if constexpr (V == Variant::Jungle)
        return std::to_array({Pawn, Knight, Camel, Grasshopper, Wildebeest, Archbishop, Chancellor});

    else if constexpr (V == Variant::XXL)
        return std::to_array({Pawn, Knight, Bishop, Rook, Queen, King, Camel,
                              General, Archbishop, Chancellor, Amazon});

    else if constexpr (V == Variant::Gothic || V == Variant::Seirawan)
        return std::to_array(
            {Pawn, Knight, Bishop, Rook, Queen, King, Archbishop, Chancellor});

    else if constexpr (V == Variant::Tinyhouse)
        return std::to_array({Pawn, King, Ferz, Wazir});

    else if constexpr (V == Variant::Clobber) 
        return std::to_array({Pawn, Rook, King, Wazir});

    else if constexpr (V == Variant::Cloister)
        return std::to_array({Wazir});

    else 
        std::unreachable();
    
}

// Index of `p` in variant V's canonical piece list, or -1 if the variant
// doesn't use that piece.
template <Variant V> inline constexpr int PieceIndex(Piece p) {
    constexpr auto ps = PieceMapping<V>();
    for (std::size_t i = 0; i < ps.size(); ++i)
        if (ps[i] == p)
            return static_cast<int>(i);
    return -1;
}

// Compile-time variant configuration. Each Variant instantiates a distinct
// Ruleset type, so members may differ in type and size across variants
template <Variant V> class Ruleset {
  public:
    // Membership scaffold: is this variant one of the listed set?
    static constexpr auto oneOf = [](std::initializer_list<Variant> vs) {
        for (Variant v : vs)
            if (v == V)
                return true;
        return false;
    };
    
    // Piece set and count, from the single-source mapping.
    static constexpr auto pieces = PieceMapping<V>();
    static constexpr std::size_t types = pieces.size();

    // Board dimensions as one value, so both come from a single initializer;
    // use dims.ranks / dims.cols, or `auto [ranks, cols] = Ruleset<V>::dims`.
    struct Dims {
        std::size_t ranks, cols;
    };
    static constexpr Dims dims = [] {
        if constexpr (oneOf({Variant::Gothic, Variant::Jungle}))
            return Dims{8, 10};
        else if constexpr (oneOf({Variant::XXL, Variant::BehindTheMirror}))
            return Dims{14, 14};
        else if constexpr (V == Variant::Tinyhouse)
            return Dims{4, 4};
        else
            return Dims{8, 8};

        // clobber, cloister unknown
    }();

    static constexpr int stalemate = [] {
        if constexpr (oneOf({Variant::Antichess, Variant::Tinyhouse}))
            return 1;
        else if constexpr (oneOf({Variant::Chaturanga}))
            return -1;
        else
            return 0;
    }(); // TODO: per-variant stalemate result

    static constexpr int checky = [] {
        if constexpr (V == Variant::ThreeCheck)
            return 3;
        else if constexpr (V == Variant::BehindTheMirror)
            return 5;
        else
            return 0;
    }();

    static constexpr int royal = [] {
        if constexpr (oneOf({Variant::Antichess, Variant::BehindTheMirror}))
            return -1;
        else if constexpr (V == Variant::Jungle)
            return PieceIndex<V>(Piece::Grasshopper);
        else
            return PieceIndex<V>(Piece::King);
    }();

    static constexpr bool pocket = oneOf({Variant::Crazyhouse, Variant::Tinyhouse, Variant::Seirawan, Variant::Setup, Variant::Cloister});

    static constexpr bool petrified = oneOf({Variant::Petrified, Variant::Cloister});
    // clobber, cloister?

    static constexpr bool points = oneOf({Variant::MiniForest, Variant::Petrified});

    static constexpr bool enPassant = !oneOf({Variant::RacingKings, Variant::Chaturanga, Variant::MiniForest,
        Variant::XXL, Variant::BehindTheMirror, Variant::Tinyhouse, Variant::Jungle, Variant::Clobber, Variant::Cloister});

    static constexpr bool castling = !oneOf({Variant::Antichess, Variant::RacingKings, Variant::Chaturanga,
        Variant::MiniForest, Variant::BehindTheMirror, Variant::Tinyhouse, Variant::Jungle, Variant::Clobber, 
        Variant::Cloister});

};


} // namespace Tilted
