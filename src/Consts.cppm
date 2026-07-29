export module Consts;

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

inline constexpr std::size_t MAX_RANKS = 14;
inline constexpr std::size_t MAX_FILES = 14;

// Pocket (pieces in hand) for drop variants: up to MAX_IN_HAND of one kind
// (all 16 pawns) over MAX_DROP_TYPES piece-type slots -- Seirawan's 8 types
// (its gated 7th/8th included) is the widest drop variant.
inline constexpr std::size_t MAX_IN_HAND = 16;
inline constexpr std::size_t MAX_DROP_TYPES = 8;

inline constexpr std::size_t MAX_HISTORY_LEN = 256;

enum class Piece {
    Pawn,        // 1
    Knight,      // 3
    King,        // 3 (non royal)
    Bishop,      // 5
    Rook,        // 5
    Queen,       // 9
    Alfil,       // 1
    Dabbaba,     // 1
    Ferz,        // 1
    Wazir,       // 1
    Horse,       // 2
    Camel,       // 3
    Grasshopper, // 3
    Wildebeest,  // 5
    General,     // 5
    Dragon,      // 7
    Archbishop,  // 7
    Chancellor,  // 7
    Amazon,      // 12
    None,
};

enum class Variant {
    Chess,
    Atomic,
    Antichess,
    ThreeCheck, // KvK insufficient
    Horde,      // pawns with no moves = stalemate, no pawns = checkmate
    KingOfTheHill,
    RacingKings, // taboo, no checks
    Chaturanga,
    Paradigm,
    MiniForest, // points are shared, pawns promote to F as well, 20 for mate,
    XXL, // pawns auto-queen on 8th rank, double push but no enpassant, 3rd from
         // left, 2nd from right castle
    Gothic,
    BehindTheMirror,
    Setup, // 39 points, no return to pocket, checks force blocks in setup
    Tinyhouse,
    Crazyhouse, // can place pieces in stalemate
    Seirawan,   // can only place when first piece move
    Petrified,  // sideways pawns, +20 for mate, "the points are shared", pawns
                // aren't affected by petrified?, king can't take, promoted full
                // value
    Spell,      // unsupported
    Jungle, // pawns promote to EHV, 20 pts for mate, "the points are shared"
    Duck,
    Clobber,
    Cloister, // 6x6 minus center 2x2, Wazirs only, no royal, petrified,
              // stalemate loses, droppable captures
    None
};

constexpr std::size_t VARIANTS = static_cast<std::size_t>(Variant::None);

template <Variant V> inline constexpr auto PieceMapping() {
    using enum Piece;

    if constexpr (V == Variant::Chess || V == Variant::Atomic ||
                  V == Variant::Antichess || V == Variant::ThreeCheck ||
                  V == Variant::Horde || V == Variant::KingOfTheHill ||
                  V == Variant::Chaturanga || V == Variant::Paradigm ||
                  V == Variant::Setup || V == Variant::Crazyhouse ||
                  V == Variant::Petrified || V == Variant::Spell ||
                  V == Variant::Duck)
        return std::to_array({Pawn, Knight, King, Bishop, Rook, Queen});

    // Two King slots: PieceIndex finds the first, so Royal lands on it, and the
    // second is an ordinary capturable king.
    else if constexpr (V == Variant::BehindTheMirror)
        return std::to_array({Pawn, Knight, King, King, Bishop, Rook, Queen});

    else if constexpr (V == Variant::RacingKings)
        return std::to_array({Knight, King, Bishop, Rook, Queen});

    else if constexpr (V == Variant::MiniForest)
        return std::to_array(
            {Pawn, Alfil, Dabbaba, King, Camel, Grasshopper, Bishop});

    else if constexpr (V == Variant::XXL)
        return std::to_array({Pawn, Knight, King, Camel, Bishop, Rook, General,
                              Archbishop, Chancellor, Queen, Amazon});

    else if constexpr (V == Variant::Gothic || V == Variant::Seirawan)
        return std::to_array(
            {Pawn, Knight, King, Bishop, Rook, Archbishop, Chancellor, Queen});

    else if constexpr (V == Variant::Tinyhouse)
        return std::to_array({Pawn, Ferz, Wazir, King});

    else if constexpr (V == Variant::Jungle)
        return std::to_array({Pawn, Knight, Camel, Grasshopper, Wildebeest,
                              Archbishop, Chancellor});

    else if constexpr (V == Variant::Clobber)
        return std::to_array({Pawn, Wazir, King, Rook});

    else if constexpr (V == Variant::Cloister)
        return std::to_array({Wazir});

    else
        std::unreachable();
}

template <Variant V> inline constexpr int PieceIndex(Piece p) {
    constexpr auto ps = PieceMapping<V>();
    for (std::size_t i = 0; i < ps.size(); ++i)
        if (ps[i] == p)
            return static_cast<int>(i);
    return -1;
}

template <Variant V> class Ruleset {
  public:
    static constexpr auto oneOf = [](std::initializer_list<Variant> vs) {
        for (Variant v : vs)
            if (v == V)
                return true;
        return false;
    };

    // Piece set and count, from the single-source mapping.
    static constexpr auto pieces = PieceMapping<V>();
    static constexpr std::size_t types = pieces.size();

    struct Dims {
        std::size_t ranks, cols;
    };
    static constexpr Dims dims = [] {
        if constexpr (oneOf({Variant::XXL, Variant::BehindTheMirror}))
            return Dims{14, 14};
        else if constexpr (oneOf({Variant::Gothic, Variant::Jungle}))
            return Dims{8, 10};
        else if constexpr (V == Variant::Tinyhouse)
            return Dims{4, 4};
        else if constexpr (V == Variant::Clobber)
            return Dims{6, 8};
        else if constexpr (V == Variant::Cloister)
            return Dims{6, 6};
        else
            return Dims{8, 8};
    }();

    static constexpr int Stalemate = [] {
        if constexpr (oneOf({Variant::Antichess, Variant::Tinyhouse}))
            return 1;
        else if constexpr (oneOf({Variant::Chaturanga, Variant::Cloister}))
            return -1;
        else
            return 0;
    }();

    static constexpr int Checky = [] {
        if constexpr (V == Variant::ThreeCheck)
            return 3;
        else if constexpr (V == Variant::BehindTheMirror)
            return 5;
        else
            return 0;
    }();

    static constexpr int Royal = [] {
        if constexpr (oneOf({Variant::Antichess, Variant::Cloister}))
            return -1;
        else if constexpr (V == Variant::Jungle)
            return PieceIndex<V>(Piece::Grasshopper);
        else
            return PieceIndex<V>(Piece::King);
    }();

    static constexpr bool Nonrectangle =
        oneOf({Variant::MiniForest, Variant::BehindTheMirror, Variant::Clobber,
               Variant::Cloister});

    static constexpr bool Regicide =
        oneOf({Variant::MiniForest, Variant::Duck});

    static constexpr bool Compulsory =
        oneOf({Variant::Antichess, Variant::Clobber});

    static constexpr bool Pocket =
        oneOf({Variant::Setup, Variant::Tinyhouse, Variant::Crazyhouse,
               Variant::Seirawan, Variant::Cloister});

    static constexpr bool Petrified =
        oneOf({Variant::Petrified, Variant::Cloister});

    static constexpr bool Points =
        oneOf({Variant::MiniForest, Variant::Petrified, Variant::Jungle});

    static constexpr bool Hill =
        oneOf({Variant::KingOfTheHill, Variant::RacingKings, Variant::Clobber});

    static constexpr bool Insufficient =
        oneOf({Variant::Chess, Variant::Atomic, Variant::ThreeCheck,
               Variant::Chaturanga, Variant::Paradigm}); // In progress

    static constexpr bool EnPassant =
        !oneOf({Variant::RacingKings, Variant::Chaturanga, Variant::MiniForest,
                Variant::Tinyhouse, Variant::Clobber, Variant::Cloister});

    static constexpr bool Castling =
        !oneOf({Variant::Antichess, Variant::RacingKings, Variant::Chaturanga,
                Variant::MiniForest, Variant::BehindTheMirror, Variant::Setup,
                Variant::Tinyhouse, Variant::Jungle, Variant::Clobber,
                Variant::Cloister});

    static constexpr bool Supported =
        oneOf({Variant::Chess, Variant::Antichess, Variant::Horde,
               Variant::Chaturanga, Variant::Paradigm, Variant::XXL,
               Variant::Gothic});
};

} // namespace Tilted
