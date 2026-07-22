export module Tilted.Attacks;

import std;
export import Tilted.Bitboard;

export namespace Tilted::Attacks {

// Simple leapers: occupancy-independent -- the piece reaches every square its
// move vector hits (all sign/axis reflections), ignoring what's between.

// Wazir (0,1): one step orthogonally.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> WazirAttacks(Square s);

// Ferz (1,1): one step diagonally.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> FerzAttacks(Square s);

// Dabbaba (0,2): a two-square orthogonal jump.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> DabbabaAttacks(Square s);

// Alfil (2,2): a two-square diagonal jump.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> AlfilAttacks(Square s);

// Knight (1,2): the standard leaper.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> KnightAttacks(Square s);

// Camel (1,3): the long knight of Tamerlane chess.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> CamelAttacks(Square s);

// Pawn captures (the two forward diagonals). Kept apart from PieceAttacks and
// given a color, since pawns are special-cased in every variant.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> PawnAttacks(Color c, Square s);

// Occupancy-dependent pieces: the reachable set depends on the current board,
// so each takes the full occupancy alongside the source square.

// Horse: the Xiangqi knight. Moves as a (1,2) leaper but is blocked ("hobbled")
// by a piece on the orthogonally adjacent square along its leg.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> HorseAttacks(Square s,
                                      const Bitboard<M, N> &occupancy);

// Bishop: slides diagonally until it hits (and includes) the first blocker.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> BishopAttacks(Square s,
                                       const Bitboard<M, N> &occupancy);

// Rook: slides orthogonally until it hits (and includes) the first blocker.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> RookAttacks(Square s, const Bitboard<M, N> &occupancy);

// Grasshopper: travels along queen lines but must hop over exactly one piece,
// the hurdle, landing on the square immediately beyond it.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> GrasshopperAttacks(Square s,
                                            const Bitboard<M, N> &occupancy);

// Unified dispatch. The piece is a template parameter, so the switch below
// collapses to a single case per instantiation -- no runtime branch.
template <Piece P, std::size_t M, std::size_t N>
constexpr Bitboard<M, N> PieceAttacks(Square s, const Bitboard<M, N> &occ);

} // namespace Tilted::Attacks

// Table definitions, #included into the purview for constexpr reachability.
#include "Attacks.inl"