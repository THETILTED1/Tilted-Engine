// Position tests. Castling geometry is checked against an independent algebraic
// reference (g1/f1, c1/d1 and their wide-board equivalents) for every variant
// whose Ruleset enables castling.
#include <bit>
#include <cstddef>
#include <string>

#include <gtest/gtest.h>

import Consts;
import Bitboard;
import Move;
import Position;

using Tilted::Black;
using Tilted::Castles;
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
    constexpr std::size_t inner = std::bit_ceil(Tilted::Bits<V>::cols());

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
            EXPECT_EQ(s / inner, rank);
            EXPECT_LT(s % inner, Tilted::Bits<V>::cols());
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
    expectSupportedLinks<Variant::ThreeCheck>();
    expectSupportedLinks<Variant::Horde>();
    expectSupportedLinks<Variant::KingOfTheHill>();
    expectSupportedLinks<Variant::RacingKings>();
    expectSupportedLinks<Variant::Chaturanga>();
    expectSupportedLinks<Variant::Paradigm>();
    expectSupportedLinks<Variant::MiniForest>();
    expectSupportedLinks<Variant::Petrified>();
    expectSupportedLinks<Variant::Duck>();
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
