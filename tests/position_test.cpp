// Position tests. Castling geometry is checked against an independent algebraic
// reference (g1/f1, c1/d1 and their wide-board equivalents) for every variant
// whose Ruleset enables castling.
#include <bit>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

import Position; // re-exports Attacks/Bitboard/Consts, Move, Util
import Zobrist;  // implementation-only for Position, so imported explicitly

namespace Zobrist = Tilted::Zobrist;

using Tilted::Bits;
using Tilted::Black;
using Tilted::Castles;
using Tilted::MAX_HISTORY_LEN;
using Tilted::Color;
using Tilted::Move;
using Tilted::Piece;
using Tilted::PieceIndex;
using Tilted::Position;
using Tilted::Ruleset;
using Tilted::Square;
using Tilted::Variant;
using Tilted::White;

// The four destinations for one color, named so expectations read as squares
// rather than bit indices.
template <Variant V> struct Dests {
    std::string kingKing, kingRook, kingQueen, queenRook;
};

template <Variant V> Dests<V> destsOf(Color c) {
    return {Move<V>::algebraic(Castles<V>::kingKingTo[c]),
            Move<V>::algebraic(Castles<V>::kingRookTo[c]),
            Move<V>::algebraic(Castles<V>::kingQueenTo[c]),
            Move<V>::algebraic(Castles<V>::queenRookTo[c])};
}

// Castling destinations for a variant, given the back rank each color castles on.
template <Variant V>
void expectDests(const Dests<V> &white, const Dests<V> &black) {
    static_assert(Ruleset<V>::Castling, "variant does not castle");
    const Dests<V> gotWhite = destsOf<V>(White), gotBlack = destsOf<V>(Black);

    EXPECT_EQ(gotWhite.kingKing, white.kingKing);
    EXPECT_EQ(gotWhite.kingRook, white.kingRook);
    EXPECT_EQ(gotWhite.kingQueen, white.kingQueen);
    EXPECT_EQ(gotWhite.queenRook, white.queenRook);

    EXPECT_EQ(gotBlack.kingKing, black.kingKing);
    EXPECT_EQ(gotBlack.kingRook, black.kingRook);
    EXPECT_EQ(gotBlack.kingQueen, black.kingQueen);
    EXPECT_EQ(gotBlack.queenRook, black.queenRook);
}

// Every 8x8 variant that castles shares standard chess geometry.
template <Variant V> void expectStandard() {
    expectDests<V>({"g1", "f1", "c1", "d1"}, {"g8", "f8", "c8", "d8"});
}

TEST(CastlesTest, StandardBoards) {
    expectStandard<Variant::Chess>();
    expectStandard<Variant::Atomic>();
    expectStandard<Variant::ThreeCheck>();
    expectStandard<Variant::Horde>();
    expectStandard<Variant::KingOfTheHill>();
    expectStandard<Variant::Paradigm>();
    expectStandard<Variant::Crazyhouse>();
    expectStandard<Variant::Seirawan>();
    expectStandard<Variant::Petrified>();
    expectStandard<Variant::Spell>();
    expectStandard<Variant::Duck>();
}

// Gothic is 10 files wide, so king-side shifts out to the i/h files.
TEST(CastlesTest, Gothic) {
    expectDests<Variant::Gothic>({"i1", "h1", "c1", "d1"},
                                 {"i8", "h8", "c8", "d8"});
}

// XXL is 14 wide: "3rd from left, 2nd from right" puts the king on c1 and m1.
TEST(CastlesTest, XXL) {
    expectDests<Variant::XXL>({"m1", "l1", "c1", "d1"},
                              {"m14", "l14", "c14", "d14"});
}

// Structural invariants that must hold whatever the board width.
template <Variant V> void expectWellFormed() {
    using C = Castles<V>;

    for (Color c : {Black, White}) {
        // A king and its rook cannot share a destination.
        EXPECT_NE(C::kingKingTo[c], C::kingRookTo[c]);
        EXPECT_NE(C::kingQueenTo[c], C::queenRookTo[c]);
        // The two sides must not collide either.
        EXPECT_NE(C::kingKingTo[c], C::kingQueenTo[c]);
        EXPECT_NE(C::kingRookTo[c], C::queenRookTo[c]);

        // Every destination sits on that color's back rank, inside the board.
        const std::size_t rank = c == White ? Tilted::Bits<V>::ranks() - 1 : 0;
        for (Square s : {C::kingKingTo[c], C::kingRookTo[c], C::kingQueenTo[c],
                         C::queenRookTo[c]}) {
            EXPECT_EQ(s / Bits<V>::innerCols(), rank);
            EXPECT_LT(s % Bits<V>::innerCols(), Bits<V>::cols());
        }
    }

    // Rooks land on the inner side of their king, so the king is always outside.
    EXPECT_GT(C::kingKingTo[White], C::kingRookTo[White]);
    EXPECT_LT(C::kingQueenTo[White], C::queenRookTo[White]);
}

TEST(CastlesTest, WellFormed) {
    expectWellFormed<Variant::Chess>();
    expectWellFormed<Variant::Gothic>();
    expectWellFormed<Variant::XXL>();
    expectWellFormed<Variant::Crazyhouse>();
    expectWellFormed<Variant::Duck>();
}

// rights() must agree with the q k Q K layout that castleStrings encodes: each
// color owns two adjacent bits, together covering the mask and nothing more.
TEST(CastlesTest, RightsMasks) {
    Castles<Variant::Chess> c{};

    EXPECT_EQ(c.rights(White), 0b0011);
    EXPECT_EQ(c.rights(Black), 0b1100);

    // The two colors partition the four rights bits.
    EXPECT_EQ(c.rights(White) & c.rights(Black), 0);
    EXPECT_EQ(c.rights(White) | c.rights(Black), 0b1111);

    // Cross-check against castleStrings: every index within a color's mask must
    // name only that color's letters, upper case for White and lower for Black.
    for (std::size_t i = 0; i < 16; ++i) {
        const std::string_view s = c.castleStrings[i];
        for (char ch : s) {
            if (ch == '-')
                continue;
            const bool upper = ch == 'K' || ch == 'Q';
            const std::uint8_t owner = upper ? c.rights(White) : c.rights(Black);
            EXPECT_NE(i & owner, 0u) << "index " << i << " spells '" << s << "'";
        }
    }

    // Masks are width-independent, so wider boards agree.
    EXPECT_EQ(Castles<Variant::XXL>{}.rights(White), 0b0011);
    EXPECT_EQ(Castles<Variant::Gothic>{}.rights(Black), 0b1100);
}

// rightsChange is indexed by internal bit index, so it must span the padded board.
TEST(CastlesTest, RightsChangeSpansBoard) {
    EXPECT_EQ(Castles<Variant::Chess>{}.rightsChange.size(), 8u * 8u);
    EXPECT_EQ(Castles<Variant::Gothic>{}.rightsChange.size(), 8u * 16u);
    EXPECT_EQ(Castles<Variant::XXL>{}.rightsChange.size(), 14u * 16u);
}

// Position is only instantiable for variants in Ruleset<V>::Supported. These also
// exercise the explicit instantiations: pieceAt is defined solely in the
// implementation unit, so this separate TU can only reach it via the emitted
// symbol -- a missing `template class Position<...>` shows up as a link failure.
TEST(PositionTest, PieceAtReportsTheDenseTypeIndex) {
    Position<Variant::Chess> p{};
    constexpr int PAWN = PieceIndex<Variant::Chess>(Piece::Pawn);
    constexpr int KING = PieceIndex<Variant::Chess>(Piece::King);

    EXPECT_EQ(p.pieceAt(48), -1) << "empty board reports no piece";

    p.pieces[PAWN].toggle(48);
    p.pieces[KING].toggle(60);
    EXPECT_EQ(p.pieceAt(48), PAWN);
    EXPECT_EQ(p.pieceAt(60), KING);
    EXPECT_EQ(p.pieceAt(0), -1) << "untouched square stays empty";
}

// One call per explicit instantiation, so a variant left out of the implementation
// unit fails to link rather than going unnoticed.
template <Variant V> void expectSupportedLinks() {
    Position<V> p{};
    EXPECT_EQ(p.pieceAt(0), -1);
    p.pieces[0].toggle(0);
    p.sides[White].toggle(0);
    EXPECT_EQ(p.pieceAt(0), 0);
    EXPECT_EQ(p.occupied().count(), 1u);
    EXPECT_EQ(p.side(White).count(), 1u);
    EXPECT_EQ(p.those(White, 0).count(), 1u);
    EXPECT_EQ(p.those(Black, 0).count(), 0u);
}

TEST(PositionTest, EverySupportedVariantInstantiatesAndLinks) {
    expectSupportedLinks<Variant::Chess>();
    expectSupportedLinks<Variant::Antichess>();
    expectSupportedLinks<Variant::Horde>();
    expectSupportedLinks<Variant::Chaturanga>();
    expectSupportedLinks<Variant::Paradigm>();
    expectSupportedLinks<Variant::XXL>();
    expectSupportedLinks<Variant::Gothic>();
}

// The type-vs-color split: any() ignores color, those() intersects, occupied()
// unions the two sides.
TEST(PositionTest, BoardAccessorsPartitionByColor) {
    Position<Variant::Chess> p{};
    constexpr int PAWN = PieceIndex<Variant::Chess>(Piece::Pawn);

    for (Square s : {48, 49}) { p.pieces[PAWN].toggle(s); p.sides[White].toggle(s); }
    p.pieces[PAWN].toggle(8);
    p.sides[Black].toggle(8);

    EXPECT_EQ(p.any(PAWN).count(), 3u);
    EXPECT_EQ(p.those(White, PAWN).count(), 2u);
    EXPECT_EQ(p.those(Black, PAWN).count(), 1u);
    EXPECT_EQ(p.occupied().count(), 3u);

    EXPECT_EQ(p.occupied(), p.side(White) | p.side(Black));
    EXPECT_EQ(p.those(White, PAWN) | p.those(Black, PAWN), p.any(PAWN));
    EXPECT_TRUE((p.those(White, PAWN) & p.those(Black, PAWN)).empty());

    // All four must work through a const Position -- isChecked/insufficient are const.
    const Position<Variant::Chess> &cp = p;
    EXPECT_EQ(cp.occupied().count(), 3u);
    EXPECT_EQ(cp.pieceAt(48), PAWN);
}




// onlyPawns(): true while nothing but pawns and the royal piece remain. Helper
// places one piece of `type` for `c` on `s` and keeps sides[] consistent.
template <Variant V>
void put(Position<V> &p, Color c, Piece type, Square s) {
    p.pieces[PieceIndex<V>(type)].toggle(s);
    p.sides[c].toggle(s);
}

TEST(PositionTest, OnlyPawnsCountsPawnsAndRoyals) {
    Position<Variant::Chess> p{};
    EXPECT_TRUE(p.onlyPawns()) << "an empty board has nothing but pawns";

    put(p, White, Piece::Pawn, 48);
    put(p, Black, Piece::Pawn, 8);
    EXPECT_TRUE(p.onlyPawns());

    // Kings are royal in Chess, so they don't break it.
    put(p, White, Piece::King, 60);
    put(p, Black, Piece::King, 4);
    EXPECT_TRUE(p.onlyPawns());

    // Any other piece does, and removing it restores the property.
    put(p, White, Piece::Knight, 57);
    EXPECT_FALSE(p.onlyPawns());
    put(p, White, Piece::Knight, 57);
    EXPECT_TRUE(p.onlyPawns());

    // Either color's non-pawn counts -- it is a whole-board predicate.
    put(p, Black, Piece::Rook, 0);
    EXPECT_FALSE(p.onlyPawns());
}

// Antichess has kings but Royal is -1, so a king is ordinary material there.
TEST(PositionTest, OnlyPawnsIgnoresNonRoyalKings) {
    static_assert(Ruleset<Variant::Antichess>::Royal < 0);
    static_assert(PieceIndex<Variant::Antichess>(Piece::King) >= 0);

    Position<Variant::Antichess> p{};
    put(p, White, Piece::Pawn, 48);
    EXPECT_TRUE(p.onlyPawns());

    put(p, White, Piece::King, 60);
    EXPECT_FALSE(p.onlyPawns()) << "no royal piece, so the king is just material";
}

// Padded boards: Gothic is 8x10 and XXL 14x14, both with innerCols() 16, so the
// complement in onlyPawns must not pick up padding bits.
TEST(PositionTest, OnlyPawnsOnPaddedBoards) {
    Position<Variant::Gothic> g{};
    EXPECT_TRUE(g.onlyPawns());
    put(g, White, Piece::Pawn, Castles<Variant::Gothic>::whiteBack + 1);
    EXPECT_TRUE(g.onlyPawns());
    put(g, White, Piece::Chancellor, Castles<Variant::Gothic>::whiteBack + 2);
    EXPECT_FALSE(g.onlyPawns());

    Position<Variant::XXL> x{};
    EXPECT_TRUE(x.onlyPawns());
    put(x, Black, Piece::Amazon, 5);
    EXPECT_FALSE(x.onlyPawns());
}

// Horde's pawn side has no king at all; the predicate is still whole-board.
TEST(PositionTest, OnlyPawnsHandlesKinglessSide) {
    Position<Variant::Horde> p{};
    for (Square s : {40, 41, 42})
        put(p, White, Piece::Pawn, s);
    put(p, Black, Piece::King, 4);
    EXPECT_TRUE(p.onlyPawns());
    put(p, Black, Piece::Queen, 3);
    EXPECT_FALSE(p.onlyPawns());
}

// Fill every member with something non-zero so empty() has work to do.
template <Variant V> void dirty(Position<V> &p) {
    p.toMove = White;
    p.clock = 7;
    for (std::size_t t = 0; t < Ruleset<V>::types; ++t)
        p.pieces[t].toggle(t);
    p.sides[Black].toggle(0);
    p.sides[White].toggle(1);
    p.hashes.fill(0xDEADBEEF);
    p.halfMoves.fill(9);
    if constexpr (Ruleset<V>::EnPassant)
        p.enPassant.fill(3);
    if constexpr (Ruleset<V>::Castling)
        p.castles.castleRights.fill(0b1111);
}

TEST(PositionTest, EmptyClearsEveryMember) {
    Position<Variant::Chess> p{};
    dirty(p);
    p.empty();

    for (const auto &b : p.pieces)
        EXPECT_TRUE(b.empty());
    EXPECT_TRUE(p.sides[Black].empty());
    EXPECT_TRUE(p.sides[White].empty());
    EXPECT_TRUE(p.occupied().empty());
    EXPECT_EQ(p.toMove, Black);
    EXPECT_EQ(p.clock, 0u);
    // empty() ends with beginZobrist(), and Chess castles, so the cleared
    // rights still contribute their key.
    EXPECT_EQ(p.hashes[0], Zobrist::castling(0));
    EXPECT_EQ(p.hashes[MAX_HISTORY_LEN - 1], 0u);
    EXPECT_EQ(p.halfMoves[0], 0);
    EXPECT_EQ(p.castles.castleRights[0], 0);
}

// The one member that must NOT be zeroed: square 0 is playable, so a zeroed
// enPassant would read as a real target on a8.
TEST(PositionTest, EmptyUsesNoSquareForEnPassant) {
    Position<Variant::Chess> p{};
    dirty(p);
    p.empty();

    EXPECT_EQ(Bits<Variant::Chess>::noSquare(), 64u);
    EXPECT_EQ(p.enPassant[0], Bits<Variant::Chess>::noSquare());
    EXPECT_EQ(p.enPassant[MAX_HISTORY_LEN - 1], Bits<Variant::Chess>::noSquare());
    EXPECT_NE(p.enPassant[0], 0u) << "zero is square a8, not 'no target'";

    // Padded boards: noSquare() is past the padded stride, not the file count.
    EXPECT_EQ(Bits<Variant::Gothic>::noSquare(), 8u * 16u);
    EXPECT_EQ(Bits<Variant::XXL>::noSquare(), 14u * 16u);
}

// empty() then beginZobrist() must agree on the sentinel: with no pieces and
// Black to move, the only surviving term is castling, so an en-passant key
// leaking in would show up as an exact mismatch.
TEST(PositionTest, EmptyThenBeginZobristHasNoEnPassantTerm) {
    Position<Variant::Antichess> a{}; // en passant, no castling
    dirty(a);
    a.empty();
    a.beginZobrist();
    EXPECT_EQ(a.hashes[0], 0u) << "no pieces, Black to move, no castling rights";

    Position<Variant::Chaturanga> c{}; // neither
    dirty(c);
    c.empty();
    c.beginZobrist();
    EXPECT_EQ(c.hashes[0], 0u);

    Position<Variant::Chess> p{}; // both
    dirty(p);
    p.empty();
    p.beginZobrist();
    EXPECT_EQ(p.hashes[0], Zobrist::castling(0)) << "castling(0) and nothing else";
}

TEST(PositionTest, EmptyIsIdempotentAcrossVariants) {
    Position<Variant::Gothic> a{}, b{};
    dirty(a);
    a.empty();
    a.empty();
    b.empty();
    a.beginZobrist();
    b.beginZobrist();
    EXPECT_EQ(a.hashes[0], b.hashes[0]);
    EXPECT_TRUE(a.occupied().empty());

    Position<Variant::XXL> x{};
    dirty(x);
    x.empty();
    EXPECT_TRUE(x.occupied().empty());
    EXPECT_EQ(x.enPassant[0], Bits<Variant::XXL>::noSquare());
}
