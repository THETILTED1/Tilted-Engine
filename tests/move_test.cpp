// Move tests. The bit schema is private, so every word here is built from an
// independent hand reference -- field widths re-derived from the variant's
// board and piece count, with flags packed in the order capturing, castling,
// enPassant, doublePush, drop, then the duck square.
#include <cstddef>
#include <cstdint>
#include <string>

#include <gtest/gtest.h>

import Move;
import Bitboard; // to anchor the hand-derived widths against the real geometry

using Tilted::Bits;
using Tilted::Move;
using Tilted::Piece;
using Tilted::PieceIndex;
using Tilted::Ruleset;
using Tilted::Square;
using Tilted::Variant;

// Chess: 64 squares -> 6-bit squares, 6 piece types -> 3-bit pieces. Both
// static_asserts fail if the geometry moves out from under the layout below.
namespace chess {
static_assert(Bits<Variant::Chess>::noSquare() == 64);
static_assert(Ruleset<Variant::Chess>::types == 6);

constexpr std::size_t sq = 6, pc = 3;
constexpr std::size_t fromBit = 0, toBit = sq, movingBit = 2 * sq;
constexpr std::size_t endingBit = movingBit + pc, victimBit = endingBit + pc;
constexpr std::size_t capturingBit = victimBit + pc;    // 21
constexpr std::size_t castlingBit = capturingBit + 1;   // 22
constexpr std::size_t enPassantBit = castlingBit + 1;   // 23
constexpr std::size_t doublePushBit = enPassantBit + 1; // 24

constexpr int PAWN = PieceIndex<Variant::Chess>(Piece::Pawn);
constexpr int ROOK = PieceIndex<Variant::Chess>(Piece::Rook);
constexpr int QUEEN = PieceIndex<Variant::Chess>(Piece::Queen);
} // namespace chess

// e7e5: mover and ending piece agree, nothing captured, double push set.
TEST(MoveTest, DecodesAQuietDoublePush) {
    using namespace chess;
    const Move<Variant::Chess> m{(12u << fromBit) | (28u << toBit) |
                                 (std::uint32_t(PAWN) << movingBit) |
                                 (std::uint32_t(PAWN) << endingBit) |
                                 (1u << doublePushBit)};

    EXPECT_EQ(Move<Variant::Chess>::algebraic(m.from()), "e7");
    EXPECT_EQ(Move<Variant::Chess>::algebraic(m.to()), "e5");
    EXPECT_EQ(m.moving(), PAWN);
    EXPECT_EQ(m.ending(), PAWN);
    EXPECT_FALSE(m.capturing());
    EXPECT_TRUE(m.doublePush());
    EXPECT_FALSE(m.enPassant());
    EXPECT_FALSE(m.castling());
}

// axb8=Q taking a rook: ending differs from moving, and victim carries the
// captured type so unmake needs nothing else.
TEST(MoveTest, DecodesAPromotionCapture) {
    using namespace chess;
    const Move<Variant::Chess> m{
        (8u << fromBit) | (1u << toBit) | (std::uint32_t(PAWN) << movingBit) |
        (std::uint32_t(QUEEN) << endingBit) |
        (std::uint32_t(ROOK) << victimBit) | (1u << capturingBit)};

    EXPECT_EQ(Move<Variant::Chess>::algebraic(m.from()), "a7");
    EXPECT_EQ(Move<Variant::Chess>::algebraic(m.to()), "b8");
    EXPECT_EQ(m.moving(), PAWN);
    EXPECT_EQ(m.ending(), QUEEN);
    EXPECT_EQ(m.victim(), ROOK);
    EXPECT_TRUE(m.capturing());
}

// Antichess drops castling, so every flag above capturing slides down one bit:
// the word that means "castling" in Chess means "en passant" here.
TEST(MoveTest, FlagsPackDownWhenTheRulesetOmitsOne) {
    static_assert(!Ruleset<Variant::Antichess>::Castling);
    static_assert(Ruleset<Variant::Antichess>::EnPassant);

    const Move<Variant::Antichess> a{1u | (1u << chess::castlingBit)};
    EXPECT_TRUE(a.enPassant()) << "castling's bit is en passant without it";
    EXPECT_FALSE(a.doublePush());
    EXPECT_FALSE(a.capturing());

    const Move<Variant::Chess> c{1u | (1u << chess::castlingBit)};
    EXPECT_TRUE(c.castling());
    EXPECT_FALSE(c.enPassant());
}

// XXL is the widest layout: 224 padded squares and 11 types fill all 32 bits,
// so the top flag lands on bit 31 and must survive the shift.
TEST(MoveTest, XXLFillsAllThirtyTwoBits) {
    static_assert(Bits<Variant::XXL>::noSquare() == 224);
    static_assert(Ruleset<Variant::XXL>::types == 11);

    constexpr std::size_t sq = 8, pc = 4;
    constexpr std::size_t victimBit = 2 * sq + 2 * pc; // 24
    constexpr std::size_t capturingBit = victimBit + pc;
    constexpr std::size_t doublePushBit = capturingBit + 3; // 31
    constexpr int AMAZON = PieceIndex<Variant::XXL>(Piece::Amazon);

    const Move<Variant::XXL> m{221u | (13u << sq) |
                               (std::uint32_t(AMAZON) << (2 * sq)) |
                               (1u << capturingBit) | (1u << doublePushBit)};

    EXPECT_EQ(m.from(), 221u) << "the largest real XXL square";
    EXPECT_EQ(m.to(), 13u);
    EXPECT_EQ(m.moving(), AMAZON);
    EXPECT_TRUE(m.capturing());
    EXPECT_TRUE(m.doublePush());
    EXPECT_FALSE(m.enPassant());
}

// The duck square is a whole square field stacked above the flags, not a flag.
TEST(MoveTest, DuckSquareSitsAboveTheFlags) {
    const Move<Variant::Duck> m{3u | (40u << 25)};
    EXPECT_EQ(m.from(), 3u);
    EXPECT_EQ(m.duck(), 40u);
}

// Cloister has exactly one piece type, so bit_width(types - 1) is 0 and all
// three piece fields are zero-width -- they must read 0, not garbage.
TEST(MoveTest, CloisterHasZeroWidthPieceFields) {
    static_assert(Ruleset<Variant::Cloister>::types == 1);

    const Move<Variant::Cloister> m{5u | (9u << 6) | (1u << 12) | (1u << 13)};
    EXPECT_EQ(m.from(), 5u);
    EXPECT_EQ(m.to(), 9u);
    EXPECT_EQ(m.moving(), 0);
    EXPECT_EQ(m.ending(), 0);
    EXPECT_EQ(m.victim(), 0);
    EXPECT_TRUE(m.capturing());
    EXPECT_TRUE(m.drop()) << "drop follows capturing directly here";
}

// null() is from == to, which no real move is; invalid() is all ones, which no
// real move reaches because its piece index is out of range.
TEST(MoveTest, SentinelsAreUnreachableAsRealMoves) {
    const auto none = Move<Variant::Chess>::null();
    EXPECT_EQ(none.data, 0u);
    EXPECT_EQ(none.from(), none.to());

    const auto bad = Move<Variant::Chess>::invalid();
    EXPECT_EQ(bad.data, ~std::uint32_t(0));
    EXPECT_EQ(bad.moving(), 7) << "7 is not a legal Chess type index";
    EXPECT_GE(bad.moving(), static_cast<int>(Ruleset<Variant::Chess>::types));
}

// Accessors are constexpr, so a Move is usable in a constant expression.
TEST(MoveTest, DecodesAtCompileTime) {
    static_assert(Move<Variant::Chess>{28u << chess::toBit}.to() == 28);
    static_assert(Move<Variant::Chess>{0}.from() == 0);
    SUCCEED();
}

// The requires-clauses are part of the interface: a variant without a rule must
// not offer its accessor at all.
template <class M> concept HasCastling = requires(const M m) { m.castling(); };
template <class M> concept HasDrop = requires(const M m) { m.drop(); };
template <class M> concept HasDuck = requires(const M m) { m.duck(); };

TEST(MoveTest, AccessorsExistOnlyWhereTheRuleDoes) {
    static_assert(HasCastling<Move<Variant::Chess>>);
    static_assert(!HasCastling<Move<Variant::Antichess>>);

    static_assert(HasDrop<Move<Variant::Crazyhouse>>);
    static_assert(!HasDrop<Move<Variant::Chess>>);

    static_assert(HasDuck<Move<Variant::Duck>>);
    static_assert(!HasDuck<Move<Variant::Chess>>);
    SUCCEED();
}

// algebraic() widens past one digit on tall boards and past 'h' on wide ones.
TEST(MoveTest, AlgebraicNamesSquaresOnEveryBoard) {
    EXPECT_EQ(Move<Variant::Chess>::algebraic(0), "a8");
    EXPECT_EQ(Move<Variant::Chess>::algebraic(56), "a1");
    EXPECT_EQ(Move<Variant::Chess>::algebraic(63), "h1");

    // Gothic is 10 files wide, so files run past 'h' to 'j'.
    EXPECT_EQ(Move<Variant::Gothic>::algebraic(9), "j8");

    // XXL is 14 ranks tall, so the top rank needs two digits.
    EXPECT_EQ(Move<Variant::XXL>::algebraic(0), "a14");
    EXPECT_EQ(Move<Variant::XXL>::algebraic(13), "n14");
}
