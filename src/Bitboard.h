#pragma once

#include "Consts.h"

namespace Tilted {

template <std::size_t L>
using Word = std::conditional_t<
    (L <= 16), std::uint16_t,
    std::conditional_t<(L <= 32), std::uint32_t, std::uint64_t>>;

// Top-left justified: square (rank r, file f) is internal bit r*innerCols + f,
// so bit 0 is a8/a14. The public interface speaks external M*N squares.
template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
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

    constexpr Bitboard shiftedLeft(std::size_t n) const;
    constexpr Bitboard shiftedRight(std::size_t n) const;

    std::array<Word<M * N>, wordCount> data{};
};

} // namespace Tilted

#include "Bitboard.inl"
