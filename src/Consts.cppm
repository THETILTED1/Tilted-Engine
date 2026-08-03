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

inline const std::string PieceChars = "PNKBRQISFWUCGVMDHEA";

inline const std::array<std::string, static_cast<std::size_t>(Piece::None)>
    PieceNames{"Pawn",   "Knight",     "King",        "Bishop",     "Rook",
               "Queen",  "Alfil",      "Dabbaba",     "Ferz",       "Wazir",
               "Horse",  "Camel",      "Grasshopper", "Wildebeest", "General",
               "Dragon", "Archbishop", "Chancellor",  "Amazon"};

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

} // namespace Tilted
