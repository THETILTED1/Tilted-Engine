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

// Hyperbola quintessence on one line: the slider's ray in both directions,
// first blocker included. rankMirror flips the line so the negative ray falls
// out of the same subtract trick as the positive -- valid for files/diagonals.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> lineAttacks(const Bitboard<M, N> &slider,
                                     const Bitboard<M, N> &occ,
                                     const Bitboard<M, N> &mask) {
    const Bitboard<M, N> o = occ & mask;
    const Bitboard<M, N> up = o - (slider << 1);
    const Bitboard<M, N> down =
        (o.rankMirror() - (slider.rankMirror() << 1)).rankMirror();
    return (up ^ down) & mask;
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> BishopAttacks(Square s, const Bitboard<M, N> &occ) {
    const Bitboard<M, N> slider = Bitboard<M, N>::squareToBitboard(s);
    return lineAttacks(slider, occ, Bitboard<M, N>::diagonalMask(s)) |
           lineAttacks(slider, occ, Bitboard<M, N>::antiDiagonalMask(s));
}

// Compile-time dispatch: the switch collapses to one case per instantiation.
// Leapers ignore occ; composites OR the primitives; sliders and the hobbled/
// hopping pieces consume occ. Pawns are color-dependent -- see PawnAttacks.
template <Piece P, std::size_t M, std::size_t N>
constexpr Bitboard<M, N> PieceAttacks(Square s, const Bitboard<M, N> &occ) {
    static_assert(P != Piece::Pawn,
                  "pawns are color-dependent; use PawnAttacks");
    switch (P) {
    // Standard pieces.
    case Piece::Knight:
        return KnightAttacks<M, N>(s);
    case Piece::Bishop:
        return BishopAttacks<M, N>(s, occ);
    case Piece::Rook:
        return RookAttacks<M, N>(s, occ);
    case Piece::Queen:
        return BishopAttacks<M, N>(s, occ) | RookAttacks<M, N>(s, occ);
    case Piece::King:
        return WazirAttacks<M, N>(s) | FerzAttacks<M, N>(s);
    // Fairy pieces (ascending value).
    case Piece::Alfil:
        return AlfilAttacks<M, N>(s);
    case Piece::Dabbaba:
        return DabbabaAttacks<M, N>(s);
    case Piece::Ferz:
        return FerzAttacks<M, N>(s);
    case Piece::Wazir:
        return WazirAttacks<M, N>(s);
    case Piece::Camel:
        return CamelAttacks<M, N>(s);
    case Piece::Grasshopper:
        return GrasshopperAttacks<M, N>(s, occ);
    case Piece::Horse:
        return HorseAttacks<M, N>(s, occ);
    case Piece::Wildebeest:
        return KnightAttacks<M, N>(s) | CamelAttacks<M, N>(s);
    case Piece::Dragon:
        return BishopAttacks<M, N>(s, occ) | WazirAttacks<M, N>(s);
    case Piece::General:
        return WazirAttacks<M, N>(s) | FerzAttacks<M, N>(s) |
               KnightAttacks<M, N>(s);
    case Piece::Archbishop:
        return BishopAttacks<M, N>(s, occ) | KnightAttacks<M, N>(s);
    case Piece::Chancellor:
        return RookAttacks<M, N>(s, occ) | KnightAttacks<M, N>(s);
    case Piece::Amazon:
        return BishopAttacks<M, N>(s, occ) | RookAttacks<M, N>(s, occ) |
               KnightAttacks<M, N>(s);
    default:
        std::unreachable();
    }
}

} // namespace Tilted::Attacks
