#pragma once

namespace Tilted {

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Square Bitboard<M, N>::flipRank(Square s) {
    return ((M - 1) - s / innerCols) * innerCols + s % innerCols;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr std::size_t Bitboard<M, N>::ranks() {
    return M;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr std::size_t Bitboard<M, N>::cols() {
    return N;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::squareToBitboard(Square s) {
    Bitboard result;
    result.data[s / bits] |= Word<M * N>(Word<M * N>(1) << (s % bits));
    return result;
}

// Line masks keyed by any square on the line; the line index is derived
// internally. Member functions wrap a function-local static constexpr table (a
// std::array<Bitboard> data member can't work -- incomplete type in-class).
template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::fileMask(Square s) {
    static constexpr std::array<Bitboard, N> masks = [] {
        std::array<Bitboard, N> m{};
        for (std::size_t ff = 0; ff < N; ++ff)
            for (std::size_t r = 0; r < M; ++r)
                m[ff] |= squareToBitboard(r * innerCols + ff);
        return m;
    }();
    return masks[s % innerCols];
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::rankMask(Square s) {
    static constexpr std::array<Bitboard, M> masks = [] {
        std::array<Bitboard, M> m{};
        for (std::size_t rr = 0; rr < M; ++rr)
            for (std::size_t f = 0; f < N; ++f)
                m[rr] |= squareToBitboard(rr * innerCols + f);
        return m;
    }();
    return masks[s / innerCols];
}

// Diagonal (a1-h8 "/", along northEast/southWest): squares share r + f.
// Antidiagonal (a8-h1 "\", along northWest/southEast): squares share r - f.
template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::diagonalMask(Square s) {
    static constexpr std::array<Bitboard, M + N - 1> masks = [] {
        std::array<Bitboard, M + N - 1> m{};
        for (std::size_t r = 0; r < M; ++r)
            for (std::size_t f = 0; f < N; ++f)
                m[r + f] |= squareToBitboard(r * innerCols + f);
        return m;
    }();
    return masks[s / innerCols + s % innerCols];
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::antiDiagonalMask(Square s) {
    static constexpr std::array<Bitboard, M + N - 1> masks = [] {
        std::array<Bitboard, M + N - 1> m{};
        for (std::size_t r = 0; r < M; ++r)
            for (std::size_t f = 0; f < N; ++f)
                m[r + (N - 1) - f] |= squareToBitboard(r * innerCols + f);
        return m;
    }();
    return masks[s / innerCols + (N - 1) - s % innerCols];
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::boardMask() {
    static constexpr Bitboard mask = [] {
        Bitboard m{};
        for (std::size_t r = 0; r < M; ++r)
            for (std::size_t f = 0; f < N; ++f)
                m |= squareToBitboard(r * innerCols + f);
        return m;
    }();
    return mask;
}

// Dense external square <-> internal bit translations (padding bijection),
// only at the I/O boundary; arithmetic elsewhere uses canonical bit layout.
template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Square Bitboard<M, N>::squareToBit(Square s) {
    static constexpr std::array<Square, M * N> table = [] {
        std::array<Square, M * N> t{};
        for (std::size_t d = 0; d < M * N; ++d)
            t[d] = (d / N) * innerCols + d % N;
        return t;
    }();
    return table[s];
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Square Bitboard<M, N>::bitToSquare(Square b) {
    static constexpr std::array<Square, innerCols * M> table = [] {
        std::array<Square, innerCols * M> t{};
        for (std::size_t d = 0; d < M * N; ++d)
            t[(d / N) * innerCols + d % N] = d;
        return t;
    }();
    return table[b];
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr bool Bitboard<M, N>::operator==(const Bitboard &other) const {
    return data == other.data;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr bool Bitboard<M, N>::empty() const {
    for (Word<M *N> word : data)
        if (word)
            return false;
    return true;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr bool Bitboard<M, N>::test(Square s) const {
    return (data[s / bits] >> (s % bits)) & 1;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr void Bitboard<M, N>::toggle(Square s) {
    data[s / bits] ^= Word<M * N>(Word<M * N>(1) << (s % bits));
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr std::size_t Bitboard<M, N>::count() const {
    std::size_t total = 0;
    for (Word<M *N> word : data)
        total += std::popcount(word);
    return total;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Square Bitboard<M, N>::leastSquare() const {
    for (std::size_t i = 0; i < wordCount; ++i)
        if (data[i])
            return i * bits + std::countr_zero(data[i]);
    return innerCols * M;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Square Bitboard<M, N>::mostSquare() const {
    for (std::size_t i = wordCount; i-- > 0;)
        if (data[i])
            return i * bits + (bits - 1 - std::countl_zero(data[i]));
    return innerCols * M;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Square Bitboard<M, N>::popLeastSquare() {
    for (std::size_t i = 0; i < wordCount; ++i)
        if (data[i]) {
            const std::size_t b = i * bits + std::countr_zero(data[i]);
            data[i] &= Word<M * N>(data[i] - 1);
            return b;
        }
    return innerCols * M;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::operator~() const {
    Bitboard result;
    for (std::size_t i = 0; i < wordCount; ++i)
        result.data[i] = ~data[i];
    result &= boardMask();
    return result;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator|=(const Bitboard &other) {
    for (std::size_t i = 0; i < wordCount; ++i)
        data[i] |= other.data[i];
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator&=(const Bitboard &other) {
    for (std::size_t i = 0; i < wordCount; ++i)
        data[i] &= other.data[i];
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator^=(const Bitboard &other) {
    for (std::size_t i = 0; i < wordCount; ++i)
        data[i] ^= other.data[i];
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N>
Bitboard<M, N>::operator|(const Bitboard &other) const {
    Bitboard result = *this;
    result |= other;
    return result;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N>
Bitboard<M, N>::operator&(const Bitboard &other) const {
    Bitboard result = *this;
    result &= other;
    return result;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N>
Bitboard<M, N>::operator^(const Bitboard &other) const {
    Bitboard result = *this;
    result ^= other;
    return result;
}

// Raw wide-integer arithmetic over the little-endian word array (data[0] least
// significant); carry/borrow propagate across words and overflow wraps. NOT
// board-masked -- for the subtract trick in sliding attacks; callers mask.
template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator+=(const Bitboard &other) {
    Word<M * N> carry = 0;
    for (std::size_t i = 0; i < wordCount; ++i) {
        const Word<M * N> sum = Word<M * N>(data[i] + other.data[i] + carry);
        carry = Word<M * N>(sum < data[i] || (sum == data[i] && carry));
        data[i] = sum;
    }
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator-=(const Bitboard &other) {
    Word<M * N> borrow = 0;
    for (std::size_t i = 0; i < wordCount; ++i) {
        const Word<M * N> diff = Word<M * N>(data[i] - other.data[i] - borrow);
        borrow = Word<M * N>(data[i] < other.data[i] ||
                             (data[i] == other.data[i] && borrow));
        data[i] = diff;
    }
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N>
Bitboard<M, N>::operator+(const Bitboard &other) const {
    Bitboard result = *this;
    result += other;
    return result;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N>
Bitboard<M, N>::operator-(const Bitboard &other) const {
    Bitboard result = *this;
    result -= other;
    return result;
}

// Raw shift treating the word array as one wide integer; top bits drop. NOT
// board-masked (can wrap into padding/off-board) -- callers mask (north/etc).
template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator<<=(Square shift) {
    const std::size_t ws = shift / bits, bs = shift % bits;
    Bitboard result;
    for (std::size_t i = wordCount; i-- > 0;) {
        Word<M * N> v{};
        if (i >= ws) {
            v = Word<M * N>(data[i - ws] << bs);
            if (bs != 0 && i > ws)
                v = Word<M * N>(v |
                                Word<M * N>(data[i - ws - 1] >> (bits - bs)));
        }
        result.data[i] = v;
    }
    *this = result;
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> &Bitboard<M, N>::operator>>=(Square shift) {
    const std::size_t ws = shift / bits, bs = shift % bits;
    Bitboard result;
    for (std::size_t i = 0; i < wordCount; ++i) {
        Word<M * N> v{};
        if (i + ws < wordCount) {
            v = Word<M * N>(data[i + ws] >> bs);
            if (bs != 0 && i + ws + 1 < wordCount)
                v = Word<M * N>(v |
                                Word<M * N>(data[i + ws + 1] << (bits - bs)));
        }
        result.data[i] = v;
    }
    *this = result;
    return *this;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::operator<<(Square shift) const {
    Bitboard result = *this;
    result <<= shift;
    return result;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::operator>>(Square shift) const {
    Bitboard result = *this;
    result >>= shift;
    return result;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::north() const {
    return *this >> innerCols;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::south() const {
    return (*this << innerCols) & boardMask();
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::east() const {
    return (*this & ~fileMask(N - 1)) << 1;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::west() const {
    return (*this & ~fileMask(0)) >> 1;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::northEast() const {
    return north().east();
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::northWest() const {
    return north().west();
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::southEast() const {
    return south().east();
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::southWest() const {
    return south().west();
}

// Reverse all rank-lanes, then shift back so the board stays top-left
// justified.
template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::rankMirror() const {
    constexpr std::size_t ranksPerWord = bits / innerCols;
    constexpr std::size_t trailing = wordCount * bits - innerCols * M;
    Bitboard reversed;
    for (std::size_t i = 0; i < wordCount; ++i) {
        const Word<M * N> w = data[wordCount - 1 - i];
        if constexpr (ranksPerWord == 1)
            reversed.data[i] = w;
        else {
            constexpr Word<M * N> lane =
                Word<M * N>((Word<M * N>(1) << innerCols) - 1);
            Word<M * N> out{};
            for (std::size_t k = 0; k < ranksPerWord; ++k)
                out = Word<M * N>(
                    out | Word<M * N>(Word<M * N>((w >> (k * innerCols)) & lane)
                                      << ((ranksPerWord - 1 - k) * innerCols)));
            reversed.data[i] = out;
        }
    }
    if constexpr (trailing != 0)
        return reversed >> trailing;
    else
        return reversed;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
std::ostream &operator<<(std::ostream &os, const Bitboard<M, N> &board) {
    constexpr std::size_t ic = std::bit_ceil(N);
    for (std::size_t r = 0; r < M; ++r) {
        for (std::size_t f = 0; f < N; ++f)
            os << (board.test(r * ic + f) ? '1' : '.');
        os << '\n';
    }
    return os;
}

} // namespace Tilted
