export module Tilted.Bitboard;

import std;
export import Tilted.Consts;

namespace Tilted {

template <std::size_t L>
using Word = std::conditional_t<
    (L <= 16), std::uint16_t,
    std::conditional_t<(L <= 32), std::uint32_t, std::uint64_t>>;

// A Square is the internal bit index r*innerCols + f (bit 0 = a8/a14). Dense
// external M*N numbering survives only in squareToBit/bitToSquare.
export template <std::size_t M, std::size_t N>
    requires(N <= 64)
class Bitboard {
  public:
    Bitboard() = default;

    static constexpr std::size_t ranks();
    static constexpr std::size_t cols();
    static constexpr Bitboard squareToBitboard(Square s);
    static constexpr Square flipRank(Square s);
    static constexpr Bitboard fileMask(Square s);
    static constexpr Bitboard rankMask(Square s);
    static constexpr Bitboard diagonalMask(Square s);
    static constexpr Bitboard antiDiagonalMask(Square s);
    static constexpr Square squareToBit(Square s);
    static constexpr Square bitToSquare(Square b);

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

    constexpr Bitboard &operator+=(const Bitboard &other);
    constexpr Bitboard &operator-=(const Bitboard &other);
    constexpr Bitboard operator+(const Bitboard &other) const;
    constexpr Bitboard operator-(const Bitboard &other) const;

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

    static constexpr Bitboard boardMask();

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
