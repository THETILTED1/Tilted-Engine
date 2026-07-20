#pragma once

namespace Tilted {

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Square Bitboard<M, N>::flipRank(Square s) {
    return ((M - 1) - s / N) * N + s % N;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr std::size_t Bitboard<M, N>::ranks() {
    return M;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr std::size_t Bitboard<M, N>::cols() {
    return N;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::squareToBitboard(Square s) {
    const std::size_t b = (s / N) * innerCols + s % N;
    Bitboard result;
    result.data[b / bits] |= Word<M * N>(Word<M * N>(1) << (b % bits));
    return result;
}

// Precomputed masks: compile-time data, indexed at runtime with no call.
template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
inline constexpr std::array<Bitboard<M, N>, N> fileMasks = [] {
    std::array<Bitboard<M, N>, N> masks{};
    for (std::size_t f = 0; f < N; ++f)
        for (std::size_t r = 0; r < M; ++r)
            masks[f] |= Bitboard<M, N>::squareToBitboard(r * N + f);
    return masks;
}();

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
inline constexpr std::array<Bitboard<M, N>, M> rankMasks = [] {
    std::array<Bitboard<M, N>, M> masks{};
    for (std::size_t r = 0; r < M; ++r)
        for (std::size_t f = 0; f < N; ++f)
            masks[r] |= Bitboard<M, N>::squareToBitboard(r * N + f);
    return masks;
}();

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
inline constexpr Bitboard<M, N> boardMask = [] {
    Bitboard<M, N> mask{};
    for (Square s = 0; s < M * N; ++s)
        mask |= Bitboard<M, N>::squareToBitboard(s);
    return mask;
}();

// External square <-> internal bit translations (the padding bijection). Let
// the contiguous shift remap trade the per-bit padding arithmetic for a table
// load.
template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
inline constexpr std::array<Square, M * N> squareToBit = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Square, M * N> t{};
    for (std::size_t s = 0; s < M * N; ++s)
        t[s] = (s / N) * ic + s % N;
    return t;
}();

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
inline constexpr std::array<Square, std::bit_ceil(N) * M> bitToSquare = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Square, ic * M> t{};
    for (std::size_t s = 0; s < M * N; ++s)
        t[(s / N) * ic + s % N] = s;
    return t;
}();

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr bool Bitboard<M, N>::operator==(const Bitboard &other) const {
    return data == other.data;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr bool Bitboard<M, N>::empty() const {
    for (Word<M *N> word : data)
        if (word)
            return false;
    return true;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr bool Bitboard<M, N>::test(Square s) const {
    const std::size_t b = (s / N) * innerCols + s % N;
    return (data[b / bits] >> (b % bits)) & 1;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr void Bitboard<M, N>::toggle(Square s) {
    const std::size_t b = (s / N) * innerCols + s % N;
    data[b / bits] ^= Word<M * N>(Word<M * N>(1) << (b % bits));
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr std::size_t Bitboard<M, N>::count() const {
    std::size_t total = 0;
    for (Word<M *N> word : data)
        total += std::popcount(word);
    return total;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Square Bitboard<M, N>::leastSquare() const {
    for (std::size_t i = 0; i < wordCount; ++i)
        if (data[i]) {
            const std::size_t b = i * bits + std::countr_zero(data[i]);
            return (b / innerCols) * N + b % innerCols;
        }
    return M * N;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Square Bitboard<M, N>::mostSquare() const {
    for (std::size_t i = wordCount; i-- > 0;)
        if (data[i]) {
            const std::size_t b =
                i * bits + (bits - 1 - std::countl_zero(data[i]));
            return (b / innerCols) * N + b % innerCols;
        }
    return M * N;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Square Bitboard<M, N>::popLeastSquare() {
    for (std::size_t i = 0; i < wordCount; ++i)
        if (data[i]) {
            const std::size_t b = i * bits + std::countr_zero(data[i]);
            data[i] &= Word<M * N>(data[i] - 1);
            return (b / innerCols) * N + b % innerCols;
        }
    return M * N;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::operator~() const {
    Bitboard result;
    for (std::size_t i = 0; i < wordCount; ++i)
        result.data[i] = ~data[i];
    result &= boardMask<M, N>;
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> &Bitboard<M, N>::operator|=(const Bitboard &other) {
    for (std::size_t i = 0; i < wordCount; ++i)
        data[i] |= other.data[i];
    return *this;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> &Bitboard<M, N>::operator&=(const Bitboard &other) {
    for (std::size_t i = 0; i < wordCount; ++i)
        data[i] &= other.data[i];
    return *this;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> &Bitboard<M, N>::operator^=(const Bitboard &other) {
    for (std::size_t i = 0; i < wordCount; ++i)
        data[i] ^= other.data[i];
    return *this;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N>
Bitboard<M, N>::operator|(const Bitboard &other) const {
    Bitboard result = *this;
    result |= other;
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N>
Bitboard<M, N>::operator&(const Bitboard &other) const {
    Bitboard result = *this;
    result &= other;
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N>
Bitboard<M, N>::operator^(const Bitboard &other) const {
    Bitboard result = *this;
    result ^= other;
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::shiftedLeft(std::size_t n) const {
    const std::size_t ws = n / bits, bs = n % bits;
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
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::shiftedRight(std::size_t n) const {
    const std::size_t ws = n / bits, bs = n % bits;
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
    return result;
}

// Move each set bit from external square s to s +/- shift; bits off the board
// drop. With no padding (innerCols == N) this is a plain word shift.
template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> &Bitboard<M, N>::operator<<=(Square shift) {
    if constexpr (innerCols == N)
        *this = shiftedLeft(shift);
    else {
        Bitboard result;
        for (std::size_t i = 0; i < wordCount; ++i) {
            Word<M * N> w = data[i];
            while (w) {
                const std::size_t b = i * bits + std::countr_zero(w);
                w &= Word<M * N>(w - 1);
                const Square to = bitToSquare<M, N>[b] + shift;
                if (to < M * N) {
                    const std::size_t nb = squareToBit<M, N>[to];
                    result.data[nb / bits] |=
                        Word<M * N>(Word<M * N>(1) << (nb % bits));
                }
            }
        }
        *this = result;
    }
    return *this;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> &Bitboard<M, N>::operator>>=(Square shift) {
    if constexpr (innerCols == N)
        *this = shiftedRight(shift);
    else {
        Bitboard result;
        for (std::size_t i = 0; i < wordCount; ++i) {
            Word<M * N> w = data[i];
            while (w) {
                const std::size_t b = i * bits + std::countr_zero(w);
                w &= Word<M * N>(w - 1);
                const Square from = bitToSquare<M, N>[b];
                if (from >= shift) {
                    const Square to = from - shift;
                    const std::size_t nb = squareToBit<M, N>[to];
                    result.data[nb / bits] |=
                        Word<M * N>(Word<M * N>(1) << (nb % bits));
                }
            }
        }
        *this = result;
    }
    return *this;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::operator<<(Square shift) const {
    Bitboard result = *this;
    result <<= shift;
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::operator>>(Square shift) const {
    Bitboard result = *this;
    result >>= shift;
    return result;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::north() const {
    return shiftedRight(innerCols);
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::south() const {
    return shiftedLeft(innerCols) & boardMask<M, N>;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::east() const {
    return (*this & ~fileMasks<M, N>[N - 1]).shiftedLeft(1);
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::west() const {
    return (*this & ~fileMasks<M, N>[0]).shiftedRight(1);
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::northEast() const {
    return north().east();
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::northWest() const {
    return north().west();
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::southEast() const {
    return south().east();
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
constexpr Bitboard<M, N> Bitboard<M, N>::southWest() const {
    return south().west();
}

// Reverse all rank-lanes, then shift back so the board stays top-left
// justified.
template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
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
        return reversed.shiftedRight(trailing);
    else
        return reversed;
}

template <std::size_t M, std::size_t N>
    requires ValidBoard<M, N>
std::ostream &operator<<(std::ostream &os, const Bitboard<M, N> &board) {
    for (std::size_t r = 0; r < M; ++r) {
        for (std::size_t f = 0; f < N; ++f)
            os << (board.test(r * N + f) ? '1' : '.');
        os << '\n';
    }
    return os;
}

} // namespace Tilted
