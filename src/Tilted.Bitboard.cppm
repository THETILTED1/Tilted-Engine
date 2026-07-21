export module Tilted.Bitboard;

import std;
export import Tilted.Consts;

namespace Tilted {

template <std::size_t L>
using Word = std::conditional_t<
    (L <= 16), std::uint16_t,
    std::conditional_t<(L <= 32), std::uint32_t, std::uint64_t>>;

// Top-left justified: square (rank r, file f) is bit r*innerCols + f, so bit 0
// is a8/a14. A Square IS that internal bit index -- the canonical coordinate
// everywhere. The dense external M*N numbering survives only in the
// squareToBit/bitToSquare tables, at the I/O boundary (algebraic, FEN/UCI).
export template <std::size_t M, std::size_t N>
    requires(N <= 64)
class Bitboard {
  public:
    Bitboard() = default;

    static constexpr std::size_t ranks();
    static constexpr std::size_t cols();
    static constexpr Bitboard squareToBitboard(Square s);
    static constexpr Square flipRank(Square s);

    constexpr bool operator==(const Bitboard &other) const;
    constexpr bool empty() const;
    constexpr bool test(Square s) const;
    constexpr void toggle(Square s);

    constexpr Square popLeastSquare();
    constexpr Square leastSquare() const;
    constexpr Square mostSquare() const;
    constexpr std::size_t count() const;

    constexpr Bitboard operator~() const;

    constexpr Bitboard &operator|=(const Bitboard &other);
    constexpr Bitboard &operator&=(const Bitboard &other);
    constexpr Bitboard &operator^=(const Bitboard &other);
    constexpr Bitboard operator|(const Bitboard &other) const;
    constexpr Bitboard operator&(const Bitboard &other) const;
    constexpr Bitboard operator^(const Bitboard &other) const;

    constexpr Bitboard &operator<<=(Square shift);
    constexpr Bitboard &operator>>=(Square shift);
    constexpr Bitboard operator<<(Square shift) const;
    constexpr Bitboard operator>>(Square shift) const;

    constexpr Bitboard north() const;
    constexpr Bitboard south() const;
    constexpr Bitboard east() const;
    constexpr Bitboard west() const;
    constexpr Bitboard northEast() const;
    constexpr Bitboard northWest() const;
    constexpr Bitboard southEast() const;
    constexpr Bitboard southWest() const;

    constexpr Bitboard rankMirror() const;

  private:
    static constexpr std::size_t bits = 8 * sizeof(Word<M * N>);
    static constexpr std::size_t innerCols = std::bit_ceil(N);
    static constexpr std::size_t wordCount = (innerCols * M + bits - 1) / bits;

    std::array<Word<M * N>, wordCount> data{};
};

// Human-readable dump; definition in Bitboard.inl, declaration exported here.
export template <std::size_t M, std::size_t N>
    requires(N <= 64)
std::ostream &operator<<(std::ostream &os, const Bitboard<M, N> &board);

} // namespace Tilted

// Member/free-function definitions, #included into the purview so they land in
// the BMI (importers need them reachable for constexpr). Not a standalone TU.
#include "Bitboard.inl"
