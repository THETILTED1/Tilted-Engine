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

// Precomputed masks: compile-time data, indexed at runtime with no call.
template <std::size_t M, std::size_t N>
    requires(N <= 64)
inline constexpr std::array<Bitboard<M, N>, N> fileMasks = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Bitboard<M, N>, N> masks{};
    for (std::size_t f = 0; f < N; ++f)
        for (std::size_t r = 0; r < M; ++r)
            masks[f] |= Bitboard<M, N>::squareToBitboard(r * ic + f);
    return masks;
}();

template <std::size_t M, std::size_t N>
    requires(N <= 64)
inline constexpr std::array<Bitboard<M, N>, M> rankMasks = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Bitboard<M, N>, M> masks{};
    for (std::size_t r = 0; r < M; ++r)
        for (std::size_t f = 0; f < N; ++f)
            masks[r] |= Bitboard<M, N>::squareToBitboard(r * ic + f);
    return masks;
}();

template <std::size_t M, std::size_t N>
    requires(N <= 64)
inline constexpr Bitboard<M, N> boardMask = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    Bitboard<M, N> mask{};
    for (std::size_t r = 0; r < M; ++r)
        for (std::size_t f = 0; f < N; ++f)
            mask |= Bitboard<M, N>::squareToBitboard(r * ic + f);
    return mask;
}();

// Dense external square <-> internal bit translations (the padding bijection),
// used only at the I/O boundary (algebraic, FEN/UCI, compact per-square
// tables). The bit layout is canonical everywhere else; arithmetic never uses
// these.
template <std::size_t M, std::size_t N>
    requires(N <= 64)
inline constexpr std::array<Square, M * N> squareToBit = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Square, M * N> t{};
    for (std::size_t s = 0; s < M * N; ++s)
        t[s] = (s / N) * ic + s % N;
    return t;
}();

template <std::size_t M, std::size_t N>
    requires(N <= 64)
inline constexpr std::array<Square, std::bit_ceil(N) * M> bitToSquare = [] {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Square, ic * M> t{};
    for (std::size_t s = 0; s < M * N; ++s)
        t[(s / N) * ic + s % N] = s;
    return t;
}();

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
    result &= boardMask<M, N>;
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

// Raw bitwise shift over the internal bit layout: the whole word array is one
// wide integer, carrying across word lines. Bits shifted past the top word are
// dropped, but nothing is board-masked -- a shift can wrap a bit into a padding
// column or an off-board square. Callers that need a clean board (and the
// board-edge semantics themselves) mask the result: see north/south/east/west.
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
    return (*this << innerCols) & boardMask<M, N>;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::east() const {
    return (*this & ~fileMasks<M, N>[N - 1]) << 1;
}

template <std::size_t M, std::size_t N>
    requires(N <= 64)
constexpr Bitboard<M, N> Bitboard<M, N>::west() const {
    return (*this & ~fileMasks<M, N>[0]) >> 1;
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
