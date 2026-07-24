export module Tilted.Position;

import std;
import Tilted.Attacks; // transitively re-exports Bitboard + Consts
import Tilted.Move;
import Tilted.Zobrist;

export namespace Tilted {

template <Variant V> using Bits = Bitboard<Ruleset<V>::dims.ranks, Ruleset<V>::dims.cols>;

template <Variant V> struct Castling{
    std::array<std::uint8_t, MAX_HISTORY_LEN> castleRights;

    std::array<Square, 2> kingRookFrom, queenRookFrom;
    // static constexpr std::array<Square, 2> kingRookTo;
    // static constexpr std::array<Square, 2> queenRookTo;
    // static constexpr std::array<Square, 2> kingKingTo;
    // static constexpr std::array<Square, 2> kingQueenTo;

    std::array<Bits<V>, 2> kingSafeMask;
    std::array<Bits<V>, 2> queenSafeMask;

    std::array<Bits<V>, 2> kingOccMask;
    std::array<Bits<V>, 2> queenOccMask;

    std::array<std::uint8_t, Bits<V>::ranks() * std::bit_ceil(Bits<V>::cols())> rightsChange;
};

// Full game state for a variant. Board geometry, piece set, and which optional
// state even exists (castling, en passant, pockets, ...) all come from
// Ruleset<V>, so each variant instantiates a distinct, minimally-sized Position.
template <Variant V> class Position {
  public:
    Position() = default;

    std::array<Bits<V>, Ruleset<V>::types> pieces{};
    std::array<Bits<V>, 2> sides{};
    Color turn;

    // conditional Table<std::size_t, 2, Ruleset<V>::types> pockets;
    // conditional Castling
    // conditional std::array<Square, MAX_HISTORY_LEN> enPassant;
    // conditional std::array<std::size_t, 2> points;

    std::array<Hash, MAX_HISTORY_LEN> hashes;
    std::array<int, MAX_HISTORY_LEN> halfMoves;
    std::array<Move<V>, MAX_HISTORY_LEN> plays;

    std::size_t clock;

    // conditional Bits duck;
    // conditional Bits hill;
    // conditional Bits bricks;

    // Constructors
    int pieceAt(const Square&) const;
    // bool insufficient() const;

    Bits<V> isAttacked(const Square&, const Color&) const;
    Bits<V> isChecked(const Color&) const;

    int sinceReset() const{ return halfMoves[clock]; }

};

} // namespace Tilted
