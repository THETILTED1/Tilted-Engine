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

// Rook rank slide from the wide-integer ops alone. East falls out of the
// subtract trick; west fills from the rook down to the nearest lower blocker
// (rook - mostBit), with a file-0 sentinel so an open rank slides to the edge.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> rankAttacks(Square s, const Bitboard<M, N> &occ) {
    constexpr std::size_t ic = std::bit_ceil(N);
    const Bitboard<M, N> rank = Bitboard<M, N>::rankMask(s);
    const Bitboard<M, N> r = Bitboard<M, N>::squareToBitboard(s);
    const Bitboard<M, N> one = Bitboard<M, N>::squareToBitboard(0);
    const Bitboard<M, N> o = occ & rank;
    const Bitboard<M, N> east = ((o - (r << 1)) ^ o) & rank;
    const Bitboard<M, N> fileZero =
        Bitboard<M, N>::squareToBitboard((s / ic) * ic);
    const Bitboard<M, N> below = (o & (r - one)) | fileZero;
    const Bitboard<M, N> west =
        r - Bitboard<M, N>::squareToBitboard(below.mostSquare());
    return east | west;
}

template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> RookAttacks(Square s, const Bitboard<M, N> &occ) {
    const Bitboard<M, N> slider = Bitboard<M, N>::squareToBitboard(s);
    return lineAttacks(slider, occ, Bitboard<M, N>::fileMask(s)) |
           rankAttacks(s, occ);
}

// Horse (Xiangqi knight): a knight hobbled by a piece on its orthogonal leg.
// Each knight target sits diagonally beyond exactly one leg (a Wazir square),
// so removing the ferz spread of the occupied legs drops precisely the blocked
// targets; the legs' inward diagonals land on other Wazir squares and are
// inert.
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> HorseAttacks(Square s, const Bitboard<M, N> &occ) {
    const Bitboard<M, N> legs = WazirAttacks<M, N>(s) & occ;
    const Bitboard<M, N> blocked = legs.northEast() | legs.northWest() |
                                   legs.southEast() | legs.southWest();
    return KnightAttacks<M, N>(s) & ~blocked;
}

// Grasshopper: hops over the first piece on each queen line to the square just
// beyond it. lineAttacks stops at that first blocker, so `& occ` isolates the
// two hurdles per line; the `above` half-mask picks the far side, and the
// matching direction operator steps one square past the hurdle (off-board
// steps and hurdle-less directions vanish under the edge masks / empty sets).
template <std::size_t M, std::size_t N>
constexpr Bitboard<M, N> GrasshopperAttacks(Square s,
                                            const Bitboard<M, N> &occ) {
    using B = Bitboard<M, N>;
    const B gen = B::squareToBitboard(s);
    const B above = B{} - (gen << 1); // bits strictly beyond the source
    const B file = lineAttacks(gen, occ, B::fileMask(s)) & occ;
    const B rank = rankAttacks(s, occ) & occ; // rank needs the non-mirror slide
    const B diag = lineAttacks(gen, occ, B::diagonalMask(s)) & occ;
    const B anti = lineAttacks(gen, occ, B::antiDiagonalMask(s)) & occ;
    return (file & above).south() | (file & ~above).north() |
           (rank & above).east() | (rank & ~above).west() |
           (diag & above).southWest() | (diag & ~above).northEast() |
           (anti & above).southEast() | (anti & ~above).northWest();
}

// Value-dependent, so the trailing else's static_assert fires only if an
// unhandled piece is actually instantiated (not for every handled one).
template <Piece> constexpr bool unhandledPiece = false;

// Compile-time dispatch. An if-constexpr chain (not a switch) so each
// instantiation compiles ONLY the requested piece's branch -- e.g.
// PieceAttacks<King, 4, 4> never pulls in the Amazon/Camel machinery, nor the
// yet-undefined GrasshopperAttacks. Leapers ignore occ; composites OR the
// primitives; sliders and the hobbled piece consume occ. Pawns are
// color-dependent -- see PawnAttacks.
template <Piece P, std::size_t M, std::size_t N>
constexpr Bitboard<M, N> PieceAttacks(Square s, const Bitboard<M, N> &occ) {
    static_assert(P != Piece::Pawn,
                  "pawns are color-dependent; use PawnAttacks");
    // Standard pieces.
    if constexpr (P == Piece::Knight)
        return KnightAttacks<M, N>(s);
    else if constexpr (P == Piece::Bishop)
        return BishopAttacks<M, N>(s, occ);
    else if constexpr (P == Piece::Rook)
        return RookAttacks<M, N>(s, occ);
    else if constexpr (P == Piece::Queen)
        return BishopAttacks<M, N>(s, occ) | RookAttacks<M, N>(s, occ);
    else if constexpr (P == Piece::King)
        return WazirAttacks<M, N>(s) | FerzAttacks<M, N>(s);
    // Fairy pieces (ascending value).
    else if constexpr (P == Piece::Alfil)
        return AlfilAttacks<M, N>(s);
    else if constexpr (P == Piece::Dabbaba)
        return DabbabaAttacks<M, N>(s);
    else if constexpr (P == Piece::Ferz)
        return FerzAttacks<M, N>(s);
    else if constexpr (P == Piece::Wazir)
        return WazirAttacks<M, N>(s);
    else if constexpr (P == Piece::Camel)
        return CamelAttacks<M, N>(s);
    else if constexpr (P == Piece::Grasshopper)
        return GrasshopperAttacks<M, N>(s, occ);
    else if constexpr (P == Piece::Horse)
        return HorseAttacks<M, N>(s, occ);
    else if constexpr (P == Piece::Wildebeest)
        return KnightAttacks<M, N>(s) | CamelAttacks<M, N>(s);
    else if constexpr (P == Piece::Dragon)
        return BishopAttacks<M, N>(s, occ) | WazirAttacks<M, N>(s);
    else if constexpr (P == Piece::General)
        return WazirAttacks<M, N>(s) | FerzAttacks<M, N>(s) |
               KnightAttacks<M, N>(s);
    else if constexpr (P == Piece::Archbishop)
        return BishopAttacks<M, N>(s, occ) | KnightAttacks<M, N>(s);
    else if constexpr (P == Piece::Chancellor)
        return RookAttacks<M, N>(s, occ) | KnightAttacks<M, N>(s);
    else if constexpr (P == Piece::Amazon)
        return BishopAttacks<M, N>(s, occ) | RookAttacks<M, N>(s, occ) |
               KnightAttacks<M, N>(s);
    else
        static_assert(unhandledPiece<P>, "unhandled piece");
}

} // namespace Tilted::Attacks
