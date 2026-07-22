#pragma once

namespace Tilted::Attacks {

// Build a leaper table by applying `step` to a lone bit on each on-board
// square. `step` composes the Bitboard direction shifts, which mask at the
// edges, so entries never wrap or stray into a padding column -- no runtime
// mask needed.
template <std::size_t M, std::size_t N, typename Step>
constexpr auto makeLeaperTable(Step step) {
    constexpr std::size_t ic = std::bit_ceil(N);
    std::array<Bitboard<M, N>, M * ic> table{};
    for (std::size_t r = 0; r < M; ++r)
        for (std::size_t f = 0; f < N; ++f) {
            const Square s = r * ic + f;
            table[s] = step(Bitboard<M, N>::squareToBitboard(s));
        }
    return table;
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> WazirAttacks(Square s) {
    static constexpr auto table =
        makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
            return b.north() | b.south() | b.east() | b.west();
        });
    return table[s];
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> FerzAttacks(Square s) {
    static constexpr auto table =
        makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
            return b.northEast() | b.northWest() | b.southEast() |
                   b.southWest();
        });
    return table[s];
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> DabbabaAttacks(Square s) {
    static constexpr auto table =
        makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
            return b.north().north() | b.south().south() | b.east().east() |
                   b.west().west();
        });
    return table[s];
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> AlfilAttacks(Square s) {
    static constexpr auto table =
        makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
            return b.northEast().northEast() | b.northWest().northWest() |
                   b.southEast().southEast() | b.southWest().southWest();
        });
    return table[s];
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> KnightAttacks(Square s) {
    static constexpr auto table =
        makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
            return b.north().north().east() | b.north().north().west() |
                   b.south().south().east() | b.south().south().west() |
                   b.east().east().north() | b.east().east().south() |
                   b.west().west().north() | b.west().west().south();
        });
    return table[s];
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> CamelAttacks(Square s) {
    static constexpr auto table =
        makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
            return b.north().north().north().east() |
                   b.north().north().north().west() |
                   b.south().south().south().east() |
                   b.south().south().south().west() |
                   b.east().east().east().north() |
                   b.east().east().east().south() |
                   b.west().west().west().north() |
                   b.west().west().west().south();
        });
    return table[s];
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> PawnAttacks(Color c, Square s) {
    // [color][square]. Under the a8-at-top layout White advances north (toward
    // the 8th rank), Black south; order matches enum Color { Black, White }.
    static constexpr std::array<
        std::array<Bitboard<M, N>, M * std::bit_ceil(N)>, 2>
        table = {
            makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
                return b.southEast() | b.southWest();
            }),
            makeLeaperTable<M, N>([](const Bitboard<M, N> &b) {
                return b.northEast() | b.northWest();
            }),
        };
    return table[c][s];
}

} // namespace Tilted::Attacks
