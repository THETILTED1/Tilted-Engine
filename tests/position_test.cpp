// Position tests. Castling geometry is checked against an independent algebraic
// reference (g1/f1, c1/d1 and their wide-board equivalents) for every variant
// whose Ruleset enables castling.
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>

#include <gtest/gtest.h>

import Position; // re-exports Attacks/Bitboard/Consts, Move, Util
import Zobrist;  // implementation-only for Position, so imported explicitly

namespace Zobrist = Tilted::Zobrist;

using Tilted::Bits;
using Tilted::Black;
using Tilted::Castles;
using Tilted::Hash;
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




template <Variant V> bool onlyPawnsIn(const std::string &fen) {
    Position<V> p{};
    p.readFen(fen);
    return p.onlyPawns();
}

TEST(PositionTest, OnlyPawnsCountsPawnsAndRoyals) {
    constexpr Variant Chess = Variant::Chess;
    EXPECT_TRUE(onlyPawnsIn<Chess>("8/8/8/8/8/8/8/8 w - - 0 1")) << "empty";
    EXPECT_TRUE(onlyPawnsIn<Chess>("8/p7/8/8/8/8/P7/8 w - - 0 1"));
    EXPECT_TRUE(onlyPawnsIn<Chess>("4k3/p7/8/8/8/8/P7/4K3 w - - 0 1"))
        << "kings are royal in Chess";

    EXPECT_FALSE(onlyPawnsIn<Chess>("4k3/p7/8/8/8/8/P7/1N2K3 w - - 0 1"));
    EXPECT_FALSE(onlyPawnsIn<Chess>("r3k3/p7/8/8/8/8/P7/4K3 w - - 0 1"))
        << "either color's non-pawn counts";
}

// Antichess has kings but Royal is -1, so a king is ordinary material there.
TEST(PositionTest, OnlyPawnsIgnoresNonRoyalKings) {
    static_assert(Ruleset<Variant::Antichess>::Royal < 0);
    static_assert(PieceIndex<Variant::Antichess>(Piece::King) >= 0);

    constexpr Variant Anti = Variant::Antichess;
    EXPECT_TRUE(onlyPawnsIn<Anti>("8/8/8/8/8/8/P7/8 w - - 0 1"));
    EXPECT_FALSE(onlyPawnsIn<Anti>("8/8/8/8/8/8/P7/4K3 w - - 0 1"))
        << "no royal piece, so the king is just material";
}

// Padded boards: Gothic is 8x10 and XXL 14x14, both with innerCols() 16, so the
// complement in onlyPawns must not pick up padding bits.
TEST(PositionTest, OnlyPawnsOnPaddedBoards) {
    constexpr Variant Goth = Variant::Gothic;
    EXPECT_TRUE(onlyPawnsIn<Goth>("10/10/10/10/10/10/10/10 w - - 0 1"));
    EXPECT_TRUE(onlyPawnsIn<Goth>("10/10/10/10/10/10/10/1P8 w - - 0 1"));
    EXPECT_FALSE(onlyPawnsIn<Goth>("10/10/10/10/10/10/10/1PE7 w - - 0 1"));

    const std::string bare = "14/14/14/14/14/14/14/14/14/14/14/14/14";
    EXPECT_TRUE(onlyPawnsIn<Variant::XXL>("14/" + bare + " w - - 0 1"));
    EXPECT_FALSE(onlyPawnsIn<Variant::XXL>("5a8/" + bare + " w - - 0 1"));
}

// Horde's pawn side has no king at all; the predicate is still whole-board.
TEST(PositionTest, OnlyPawnsHandlesKinglessSide) {
    constexpr Variant Horde = Variant::Horde;
    EXPECT_TRUE(onlyPawnsIn<Horde>("4k3/8/8/8/8/PPP5/8/8 w - - 0 1"));
    EXPECT_FALSE(onlyPawnsIn<Horde>("3qk3/8/8/8/8/PPP5/8/8 w - - 0 1"));
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
    EXPECT_EQ(p.clock, 0);
    EXPECT_EQ(p.hashes[0], 0u);
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

// Move's bit layout is private, so Enc rebuilds it. MoveEncoderMatchesMoveItself
// pins it against Move: a wrong layout would quietly make every move fiction.
template <Variant V> struct Enc {
    static constexpr std::size_t sq = std::bit_width(Bits<V>::noSquare() - 1);
    static constexpr std::size_t pc = std::bit_width(Ruleset<V>::types - 1);
    static constexpr std::size_t movingBit = 2 * sq;
    static constexpr std::size_t endingBit = movingBit + pc;
    static constexpr std::size_t victimBit = endingBit + pc;
    static constexpr std::size_t capturingBit = victimBit + pc;
    static constexpr std::size_t castlingBit = capturingBit + 1;
    static constexpr std::size_t enPassantBit =
        castlingBit + Ruleset<V>::Castling;
    static constexpr std::size_t doublePushBit = enPassantBit + 1;

    struct Spec {
        Square from = 0, to = 0;
        int moving = 0, ending = -1, victim = 0;
        bool capturing = false, castling = false, enPassant = false,
             doublePush = false;
    };

    static Move<V> move(const Spec &s) {
        const auto flag = [](bool on, std::size_t at) {
            return std::uint32_t(on) << at;
        };
        return Move<V>{
            std::uint32_t(s.from) | (std::uint32_t(s.to) << sq) |
            (std::uint32_t(s.moving) << movingBit) |
            (std::uint32_t(s.ending < 0 ? s.moving : s.ending) << endingBit) |
            (std::uint32_t(s.victim) << victimBit) |
            flag(s.capturing, capturingBit) | flag(s.castling, castlingBit) |
            flag(s.enPassant, enPassantBit) | flag(s.doublePush, doublePushBit)};
    }

    // Algebraic (rank, file) as an internal square: a8 is 0, rank 1 last.
    static constexpr Square at(std::size_t rank, std::size_t file) {
        return (Bits<V>::ranks() - rank) * Bits<V>::innerCols() + file;
    }

    static constexpr int index(Piece p) { return PieceIndex<V>(p); }
};

using E = Enc<Variant::Chess>;

// Seeds a hash history by hand: ply i holds hs[i], the halfmove clock counts up
// from a reset at ply 0, and clock sits on the last ply.
template <Variant V>
void history(Position<V> &p, std::initializer_list<Hash> hs) {
    p.empty();
    int i = 0;
    for (Hash h : hs) {
        p.hashes[i] = h;
        p.halfMoves[i] = i;
        ++i;
    }
    p.clock = i - 1;
}

// beginZobrist() rebuilds a hash from the boards alone and works off ply 0,
// hence the copy down. Any key an incremental update forgot shows as a mismatch.
template <Variant V> Hash scratchHash(const Position<V> &p) {
    Position<V> fresh{};
    fresh.empty();
    fresh.pieces = p.pieces;
    fresh.sides = p.sides;
    fresh.toMove = p.toMove;
    if constexpr (Ruleset<V>::Castling)
        fresh.castles.castleRights[0] = p.castles.castleRights[p.clock];
    if constexpr (Ruleset<V>::EnPassant)
        fresh.enPassant[0] = p.enPassant[p.clock];
    fresh.beginZobrist();
    return fresh.hashes[0];
}

// Holds whatever a move did: the two sides partition the occupancy, no square
// carries two types, and the hash still agrees with the boards.
template <Variant V> void expectConsistent(const Position<V> &p) {
    Bits<V> union_{};
    for (const Bits<V> &b : p.pieces) {
        EXPECT_TRUE((union_ & b).empty()) << "a square holds two types";
        union_ |= b;
    }
    EXPECT_TRUE((p.sides[Black] & p.sides[White]).empty()) << "and two colors";
    EXPECT_EQ(union_, p.occupied()) << "sides and types disagree";
    EXPECT_EQ(p.thisHash(), scratchHash(p)) << "hash drifted from the boards";
}

template <Variant V> void expectRoundTrip(Position<V> &p, const Move<V> &m) {
    const auto pieces = p.pieces;
    const auto sides = p.sides;
    const Color toMove = p.toMove;
    const int clock = p.clock, halfMoves = p.sinceReset();
    const Hash hash = p.thisHash();
    int passant = 0;
    if constexpr (Ruleset<V>::EnPassant)
        passant = p.thisPassant();

    p.makeMove(m);
    expectConsistent(p);
    p.unmakeMove();

    EXPECT_EQ(p.pieces, pieces);
    EXPECT_EQ(p.sides, sides);
    EXPECT_EQ(p.toMove, toMove);
    EXPECT_EQ(p.clock, clock);
    EXPECT_EQ(p.sinceReset(), halfMoves);
    EXPECT_EQ(p.thisHash(), hash);
    if constexpr (Ruleset<V>::EnPassant)
        EXPECT_EQ(p.thisPassant(), passant);
}

template <Variant V> bool drawnWith(const std::string &fen) {
    Position<V> p{};
    p.readFen(fen);
    return p.insufficient();
}

TEST(PositionTest, ReadFenParsesTheBoardAndTheCounters) {
    Position<Variant::Chess> p{}, start{};
    start.setStartPos();
    p.readFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    EXPECT_EQ(p.pieces, start.pieces) << "the start position, read back";
    EXPECT_EQ(p.sides, start.sides);
    EXPECT_EQ(p.toMove, White);
    EXPECT_EQ(p.thisPassant(), Bits<Variant::Chess>::noSquare());
    EXPECT_EQ(p.sinceReset(), 0);
    EXPECT_EQ(p.castles.castleRights[0], 0b1111);
    EXPECT_EQ(p.castles.kingRookFrom, start.castles.kingRookFrom);
    EXPECT_EQ(p.castles.queenRookFrom, start.castles.queenRookFrom);
    EXPECT_EQ(p.thisHash(), start.thisHash()) << "rights folded in and all";

    p.readFen("4k3/8/8/8/4pP2/8/8/4K3 b Kq f3 7 40");
    EXPECT_EQ(p.castles.castleRights[0], 0b1001) << "K and q only";

    p.readFen("4k3/8/8/8/4pP2/8/8/4K3 b - f3 7 40");
    EXPECT_EQ(p.pieceAt(E::at(4, 5)), E::index(Piece::Pawn));
    EXPECT_TRUE(p.sides[White].test(E::at(4, 5)));
    EXPECT_TRUE(p.sides[Black].test(E::at(4, 4)));
    EXPECT_EQ(p.occupied().count(), 4u);
    EXPECT_EQ(p.toMove, Black);
    EXPECT_EQ(p.thisPassant(), E::at(3, 5)) << "f3";
    EXPECT_EQ(p.sinceReset(), 7);
    expectConsistent(p);
}

// Shredder spells the rooks' files, so a shuffled rank needs no corner guess.
TEST(PositionTest, ReadFenReadsShredderCastlingWhenFRC) {
    Position<Variant::Chess> p{};
    p.castles.isFRC = true;
    p.readFen("bqnrkrnb/pppppppp/8/8/8/8/PPPPPPPP/BQNRKRNB w DFdf - 0 1");

    EXPECT_EQ(p.castles.castleRights[0], 0b1111);
    EXPECT_EQ(p.castles.kingFrom[White], E::at(1, 4));
    EXPECT_EQ(p.castles.kingRookFrom[White], E::at(1, 5)) << "f1, not the corner";
    EXPECT_EQ(p.castles.queenRookFrom[White], E::at(1, 3)) << "d1";
    EXPECT_EQ(p.castles.kingRookFrom[Black], E::at(8, 5));
    EXPECT_EQ(p.castles.queenRookFrom[Black], E::at(8, 3));

    const auto sq = [](std::size_t rank, std::size_t file) {
        return Bits<Variant::Chess>::squareToBitboard(E::at(rank, file));
    };
    EXPECT_EQ(p.castles.kingOccMask[White], sq(1, 6)) << "only g1 must clear";
    EXPECT_EQ(p.castles.queenOccMask[White], sq(1, 2)) << "only c1 must clear";
    EXPECT_EQ(p.castles.kingSafeMask[White], sq(1, 4) | sq(1, 5) | sq(1, 6));
    EXPECT_EQ(p.castles.queenSafeMask[White], sq(1, 2) | sq(1, 3) | sq(1, 4));
    expectConsistent(p);
}

// Ten files on a sixteen-wide row: an empty rank is "10", padding never shows.
TEST(PositionTest, ReadFenHandlesWideBoards) {
    using G = Enc<Variant::Gothic>;
    Position<Variant::Gothic> p{}, start{};
    start.setStartPos();
    // PieceChars spells Chancellor 'E' and Archbishop 'H', not the 'C' and 'A'
    // that Capablanca FENs use elsewhere.
    p.readFen("rnbqekhbnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQEKHBNR w - - 0 1");

    EXPECT_EQ(p.pieces, start.pieces);
    EXPECT_EQ(p.sides, start.sides);
    EXPECT_EQ(p.pieceAt(G::at(8, 9)), G::index(Piece::Rook)) << "j8, the last file";
    expectConsistent(p);
}

TEST(PositionTest, MakeFenRoundTripsWithReadFen) {
    Position<Variant::Chess> p{};
    p.setStartPos();
    EXPECT_EQ(p.makeFen(),
              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1023");

    for (const std::string &fen :
         {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1023",
          "4k3/8/8/8/4pP2/8/8/4K3 b Kq f3 7 1023",
          "8/8/8/8/8/8/8/8 w - - 0 1023"}) {
        SCOPED_TRACE(fen);
        p.readFen(fen);
        EXPECT_EQ(p.makeFen(), fen);
    }

    // Ten files: an empty rank is "10", and the padding never shows.
    Position<Variant::Gothic> g{};
    g.setStartPos();
    EXPECT_EQ(g.makeFen(), "rnbqekhbnr/pppppppppp/10/10/10/10/PPPPPPPPPP/"
                           "RNBQEKHBNR w KQkq - 0 1023");

    // Chaturanga has neither castling nor en passant, so both fields are '-'.
    Position<Variant::Chaturanga> c{};
    c.setStartPos();
    EXPECT_EQ(c.makeFen(),
              "rnikfinr/pppppppp/8/8/8/8/PPPPPPPP/RNIKFINR w - - 0 1023");
}

// Shredder writes the rooks' files, kingside first, as readFen reads them back.
TEST(PositionTest, MakeFenWritesShredderCastlingWhenFRC) {
    Position<Variant::Chess> p{};
    p.castles.isFRC = true;

    const std::string fen =
        "bqnrkrnb/pppppppp/8/8/8/8/PPPPPPPP/BQNRKRNB w FDfd - 0 1023";
    p.readFen(fen);
    EXPECT_EQ(p.makeFen(), fen);

    p.setStartPos();
    EXPECT_EQ(p.makeFen(),
              "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w HAha - 0 1023")
        << "the standard rank, spelled the Shredder way";
}

// Forgetting must land exactly where reading the same position would.
TEST(PositionTest, ForgetLeavesWhatReadFenWould) {
    Position<Variant::Chess> played{}, loaded{};
    played.setStartPos();
    played.makeMove(E::move({.from = E::at(2, 4),
                             .to = E::at(4, 4),
                             .moving = E::index(Piece::Pawn),
                             .doublePush = true}));
    played.forget();

    loaded.readFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

    EXPECT_EQ(played.pieces, loaded.pieces);
    EXPECT_EQ(played.sides, loaded.sides);
    EXPECT_EQ(played.toMove, loaded.toMove);
    EXPECT_EQ(played.clock, 0);
    EXPECT_EQ(played.thisHash(), loaded.thisHash());
    EXPECT_EQ(played.sinceReset(), loaded.sinceReset());
    EXPECT_EQ(played.thisPassant(), loaded.thisPassant());
    EXPECT_EQ(played.castles.castleRights[0], loaded.castles.castleRights[0]);
    EXPECT_EQ(played.lastPlayed().data, loaded.lastPlayed().data);
    EXPECT_EQ(played.repetitions(), 1) << "the discarded plies cannot repeat";
}

TEST(PositionTest, MoveEncoderMatchesMoveItself) {
    const int knight = E::index(Piece::Knight), queen = E::index(Piece::Queen),
              rook = E::index(Piece::Rook);
    const Move<Variant::Chess> m = E::move({.from = E::at(2, 4),
                                            .to = E::at(4, 4),
                                            .moving = knight,
                                            .ending = queen,
                                            .victim = rook,
                                            .capturing = true,
                                            .castling = true,
                                            .enPassant = true,
                                            .doublePush = true});

    EXPECT_EQ(m.from(), E::at(2, 4));
    EXPECT_EQ(m.to(), E::at(4, 4));
    EXPECT_EQ(m.moving(), knight);
    EXPECT_EQ(m.ending(), queen);
    EXPECT_EQ(m.victim(), rook);
    EXPECT_TRUE(m.capturing());
    EXPECT_TRUE(m.castling());
    EXPECT_TRUE(m.enPassant());
    EXPECT_TRUE(m.doublePush());

    EXPECT_EQ(Move<Variant::Chess>::algebraic(E::at(2, 4)), "e2");
    EXPECT_EQ(Move<Variant::Chess>::algebraic(E::at(8, 0)), "a8");
    EXPECT_EQ(Move<Variant::Chess>::algebraic(E::at(1, 7)), "h1");
    EXPECT_EQ(Move<Variant::Gothic>::algebraic(Enc<Variant::Gothic>::at(2, 4)),
              "e2");
}

TEST(PositionTest, RepetitionsCountsToTheThreefold) {
    Position<Variant::Chess> p{};
    history(p, {0xA});
    EXPECT_EQ(p.repetitions(), 1) << "nothing to repeat yet";

    history(p, {0xA, 0xB, 0xC, 0xB, 0xA});
    EXPECT_EQ(p.repetitions(), 2) << "a twofold is not the draw";

    history(p, {0xA, 0xB, 0xA, 0xB, 0xA});
    EXPECT_EQ(p.repetitions(), 3);
}

TEST(PositionTest, RepetitionsBoundsTheWalk) {
    Position<Variant::Chess> twos{};
    history(twos, {0xA, 0xB, 0xC});
    twos.hashes[twos.clock - 1] = twos.thisHash();
    EXPECT_EQ(twos.repetitions(), 1) << "the other side's plies cannot match";

    Position<Variant::Chess> reset{};
    history(reset, {0xA, 0xB, 0xC, 0xB, 0xA});
    reset.halfMoves = {0, 0, 1, 2, 3};
    EXPECT_EQ(reset.sinceReset(), 3);
    EXPECT_EQ(reset.repetitions(), 1) << "an odd-ply reset walls off ply 0";

    Position<Variant::Chess> loaded{};
    history(loaded, {0xA, 0xB, 0xA});
    loaded.halfMoves[loaded.clock] = 40;
    EXPECT_EQ(loaded.repetitions(), 2) << "a mid-count FEN holds less history";
}

// Inside the tree one repeat already scores as a draw, so it counts twice --
// but the root itself is not inside the tree.
TEST(PositionTest, RepetitionsCountsInTreeRepeatsTwice) {
    Position<Variant::Chess> p{};
    history(p, {0xA, 0xB, 0xA});

    EXPECT_EQ(p.repetitions(0), 2) << "game rule: a plain twofold";
    EXPECT_EQ(p.repetitions(4), 3) << "repeat sits two plies past the root";
    EXPECT_EQ(p.repetitions(2), 2) << "repeat is the root, not past it";
}

TEST(PositionTest, PassMoveHandsOverTheTurn) {
    Position<Variant::Chess> p{};
    p.setStartPos();
    const auto pieces = p.pieces;
    const auto sides = p.sides;
    const Hash before = p.thisHash();

    p.passMove();

    EXPECT_EQ(p.toMove, Black);
    EXPECT_EQ(p.clock, 1);
    EXPECT_EQ(p.pieces, pieces) << "a pass touches no piece";
    EXPECT_EQ(p.sides, sides);
    EXPECT_EQ(p.thisHash(), before ^ Zobrist::turn()) << "only the turn flips";
    EXPECT_EQ(p.sinceReset(), 1) << "a pass is reversible, so the clock runs on";
    EXPECT_EQ(p.lastPlayed().data, 0u) << "the null move";
    EXPECT_EQ(p.castles.castleRights[1], p.castles.castleRights[0]);

    p.passMove();
    EXPECT_EQ(p.toMove, White);
    EXPECT_EQ(p.thisHash(), before) << "two passes restore the hash";
}

TEST(PositionTest, PassMoveForfeitsEnPassantWhereItExists) {
    Position<Variant::Chess> p{};
    p.setStartPos();
    const Square ep = E::at(6, 4);
    const Square file = ep % Bits<Variant::Chess>::innerCols();
    p.enPassant[p.clock] = ep;
    p.hashes[p.clock] ^= Zobrist::enPassant(file);
    const Hash before = p.thisHash();

    p.passMove();
    EXPECT_EQ(p.thisPassant(), Bits<Variant::Chess>::noSquare());
    EXPECT_EQ(p.thisHash(), before ^ Zobrist::turn() ^ Zobrist::enPassant(file));

    // Chaturanga has neither rule, so passing must compile without either field.
    Position<Variant::Chaturanga> c{};
    c.setStartPos();
    const Hash start = c.thisHash();
    c.passMove();
    EXPECT_EQ(c.thisHash(), start ^ Zobrist::turn());
    c.unpassMove();
    EXPECT_EQ(c.thisHash(), start);
    EXPECT_EQ(c.toMove, White);
}

TEST(PositionTest, UnpassMoveRestoresThePriorPly) {
    Position<Variant::Chess> p{};
    p.setStartPos();
    const Hash before = p.thisHash();
    const Square ep = p.thisPassant();
    const auto rights = p.castles.castleRights[0];

    p.passMove();
    p.unpassMove();

    EXPECT_EQ(p.toMove, White);
    EXPECT_EQ(p.clock, 0);
    EXPECT_EQ(p.thisHash(), before);
    EXPECT_EQ(p.thisPassant(), ep);
    EXPECT_EQ(p.castles.castleRights[0], rights);
}

TEST(PositionTest, MakeMoveMovesCapturesAndPromotes) {
    const int knight = E::index(Piece::Knight), rook = E::index(Piece::Rook),
              pawn = E::index(Piece::Pawn), queen = E::index(Piece::Queen);

    Position<Variant::Chess> quiet{};
    quiet.setStartPos();
    const Move<Variant::Chess> m =
        E::move({.from = E::at(1, 6), .to = E::at(3, 5), .moving = knight});
    quiet.makeMove(m);
    EXPECT_EQ(quiet.pieceAt(E::at(1, 6)), -1);
    EXPECT_EQ(quiet.pieceAt(E::at(3, 5)), knight);
    EXPECT_TRUE(quiet.sides[White].test(E::at(3, 5)));
    EXPECT_EQ(quiet.toMove, Black);
    EXPECT_EQ(quiet.clock, 1);
    EXPECT_EQ(quiet.lastPlayed().data, m.data);
    expectConsistent(quiet);

    Position<Variant::Chess> capture{};
    capture.readFen("n3k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    capture.makeMove(E::move({.from = E::at(1, 0),
                              .to = E::at(8, 0),
                              .moving = rook,
                              .victim = knight,
                              .capturing = true}));
    EXPECT_EQ(capture.pieceAt(E::at(8, 0)), rook) << "victim replaced";
    EXPECT_TRUE(capture.sides[White].test(E::at(8, 0)));
    EXPECT_FALSE(capture.sides[Black].test(E::at(8, 0)));
    expectConsistent(capture);

    // A promotion capture is the awkward one: moving, ending and victim all land
    // on the pawn board or the queen board rather than one each.
    Position<Variant::Chess> promote{};
    promote.readFen("1p2k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    promote.makeMove(E::move({.from = E::at(7, 0),
                              .to = E::at(8, 1),
                              .moving = pawn,
                              .ending = queen,
                              .victim = pawn,
                              .capturing = true}));
    EXPECT_TRUE(promote.pieces[pawn].empty()) << "both pawns left the board";
    EXPECT_EQ(promote.pieceAt(E::at(8, 1)), queen);
    expectConsistent(promote);
}

TEST(PositionTest, MakeMoveHandlesEnPassant) {
    const int pawn = E::index(Piece::Pawn), knight = E::index(Piece::Knight);

    Position<Variant::Chess> w{};
    w.readFen("4k3/8/8/3Pp3/8/8/8/4K3 w - e6 0 1");
    w.makeMove(E::move({.from = E::at(5, 3),
                        .to = E::at(6, 4),
                        .moving = pawn,
                        .victim = pawn,
                        .capturing = true,
                        .enPassant = true}));
    EXPECT_EQ(w.pieceAt(E::at(5, 4)), -1) << "the victim stood beside, not under";
    EXPECT_EQ(w.pieceAt(E::at(6, 4)), pawn);
    EXPECT_EQ(w.thisPassant(), Bits<Variant::Chess>::noSquare()) << "spent";
    expectConsistent(w);

    Position<Variant::Chess> b{};
    b.readFen("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
    b.makeMove(E::move({.from = E::at(4, 3),
                        .to = E::at(3, 4),
                        .moving = pawn,
                        .victim = pawn,
                        .capturing = true,
                        .enPassant = true}));
    EXPECT_EQ(b.pieceAt(E::at(4, 4)), -1) << "behind runs the other way";
    expectConsistent(b);

    Position<Variant::Chess> p{};
    p.setStartPos();
    p.makeMove(E::move({.from = E::at(2, 4),
                        .to = E::at(4, 4),
                        .moving = pawn,
                        .doublePush = true}));
    EXPECT_EQ(p.thisPassant(), E::at(3, 4)) << "e3, the square e2e4 crossed";
    expectConsistent(p);

    p.makeMove(E::move({.from = E::at(7, 4),
                        .to = E::at(5, 4),
                        .moving = pawn,
                        .doublePush = true}));
    EXPECT_EQ(p.thisPassant(), E::at(6, 4)) << "e6, Black steps the other way";

    p.makeMove(
        E::move({.from = E::at(1, 6), .to = E::at(3, 5), .moving = knight}));
    EXPECT_EQ(p.thisPassant(), Bits<Variant::Chess>::noSquare());
    expectConsistent(p);

    // Gothic's rank step is 16, not its ten files.
    using G = Enc<Variant::Gothic>;
    Position<Variant::Gothic> g{};
    g.setStartPos();
    g.makeMove(G::move({.from = G::at(2, 4),
                        .to = G::at(4, 4),
                        .moving = G::index(Piece::Pawn),
                        .doublePush = true}));
    EXPECT_EQ(g.thisPassant(), G::at(3, 4));
    expectConsistent(g);
}

TEST(PositionTest, MakeMoveHandlesCastling) {
    const int king = E::index(Piece::King), rook = E::index(Piece::Rook);
    const Square e1 = E::at(1, 4), f1 = E::at(1, 5), g1 = E::at(1, 6),
                 h1 = E::at(1, 7);

    Position<Variant::Chess> p{};
    p.readFen("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

    p.makeMove(
        E::move({.from = e1, .to = g1, .moving = king, .castling = true}));

    EXPECT_EQ(p.pieceAt(g1), king);
    EXPECT_EQ(p.pieceAt(f1), rook) << "the rook rode along";
    EXPECT_EQ(p.pieceAt(h1), -1);
    EXPECT_EQ(p.castles.castleRights[1], 0b1100) << "White's bits are spent";
    expectConsistent(p);
}

TEST(PositionTest, MakeMoveTracksTheHalfmoveClock) {
    const int knight = E::index(Piece::Knight), pawn = E::index(Piece::Pawn),
              rook = E::index(Piece::Rook);

    Position<Variant::Chess> p{};
    p.setStartPos();
    p.makeMove(
        E::move({.from = E::at(1, 6), .to = E::at(3, 5), .moving = knight}));
    p.makeMove(
        E::move({.from = E::at(8, 6), .to = E::at(6, 5), .moving = knight}));
    EXPECT_EQ(p.sinceReset(), 2) << "quiet piece moves accumulate";

    p.makeMove(
        E::move({.from = E::at(2, 3), .to = E::at(3, 3), .moving = pawn}));
    EXPECT_EQ(p.sinceReset(), 0) << "a pawn move is irreversible";

    Position<Variant::Chess> q{};
    q.readFen("n3k3/8/8/8/8/8/8/R3K3 w - - 9 1");
    q.makeMove(E::move({.from = E::at(1, 0),
                        .to = E::at(8, 0),
                        .moving = rook,
                        .victim = knight,
                        .capturing = true}));
    EXPECT_EQ(q.sinceReset(), 0) << "so is a capture";
}

// A round trip alone would pass on a pair of no-ops, so the forward direction is
// pinned above; this covers every move kind reaching unmakeMove.
TEST(PositionTest, UnmakeMoveReversesEveryMoveKind) {
    const int pawn = E::index(Piece::Pawn), knight = E::index(Piece::Knight),
              rook = E::index(Piece::Rook), queen = E::index(Piece::Queen),
              king = E::index(Piece::King);

    struct Case {
        const char *fen;
        Move<Variant::Chess> move;
    };

    const std::string start =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    const std::array cases{
        Case{start.c_str(), E::move({.from = E::at(1, 6),
                                     .to = E::at(3, 5),
                                     .moving = knight})},
        Case{start.c_str(), E::move({.from = E::at(2, 4),
                                     .to = E::at(4, 4),
                                     .moving = pawn,
                                     .doublePush = true})},
        Case{"n3k3/8/8/8/8/8/8/R3K3 w - - 9 1",
             E::move({.from = E::at(1, 0), .to = E::at(8, 0), .moving = rook,
                      .victim = knight, .capturing = true})},
        Case{"4k3/P7/8/8/8/8/8/4K3 w - - 0 1",
             E::move({.from = E::at(7, 0), .to = E::at(8, 0), .moving = pawn,
                      .ending = queen})},
        Case{"1p2k3/P7/8/8/8/8/8/4K3 w - - 0 1",
             E::move({.from = E::at(7, 0), .to = E::at(8, 1), .moving = pawn,
                      .ending = queen, .victim = pawn, .capturing = true})},
        Case{"4k3/8/8/3Pp3/8/8/8/4K3 w - e6 0 1",
             E::move({.from = E::at(5, 3), .to = E::at(6, 4), .moving = pawn,
                      .victim = pawn, .capturing = true, .enPassant = true})},
        Case{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
             E::move({.from = E::at(1, 4), .to = E::at(1, 6), .moving = king,
                      .castling = true})}};

    for (const Case &c : cases) {
        SCOPED_TRACE(c.fen);
        Position<Variant::Chess> p{};
        p.readFen(c.fen);
        expectRoundTrip(p, c.move);
    }
}

TEST(PositionTest, UnmakeMoveRewindsASequence) {
    const int knight = E::index(Piece::Knight), pawn = E::index(Piece::Pawn);
    Position<Variant::Chess> p{}, start{};
    p.setStartPos();
    start.setStartPos();

    const std::array line{
        E::move({.from = E::at(1, 6), .to = E::at(3, 5), .moving = knight}),
        E::move({.from = E::at(8, 6), .to = E::at(6, 5), .moving = knight}),
        E::move({.from = E::at(2, 4),
                 .to = E::at(4, 4),
                 .moving = pawn,
                 .doublePush = true}),
        E::move({.from = E::at(7, 3),
                 .to = E::at(5, 3),
                 .moving = pawn,
                 .doublePush = true})};

    for (const Move<Variant::Chess> &m : line)
        p.makeMove(m);
    EXPECT_EQ(p.clock, 4);
    EXPECT_NE(p.thisHash(), start.thisHash());

    for (std::size_t i = 0; i < line.size(); ++i)
        p.unmakeMove();

    EXPECT_EQ(p.pieces, start.pieces);
    EXPECT_EQ(p.sides, start.sides);
    EXPECT_EQ(p.toMove, start.toMove);
    EXPECT_EQ(p.clock, 0);
    EXPECT_EQ(p.thisHash(), start.thisHash());
    EXPECT_EQ(p.thisPassant(), start.thisPassant());
    EXPECT_EQ(p.castles.castleRights[0], start.castles.castleRights[0]);
}

TEST(PositionTest, InsufficientCountsMinors) {
    constexpr Variant Chess = Variant::Chess;
    EXPECT_TRUE(drawnWith<Chess>("4k3/8/8/8/8/8/8/4K3 w - - 0 1")) << "bare kings";
    EXPECT_TRUE(drawnWith<Chess>("4k3/8/8/8/8/8/8/4KN2 w - - 0 1")) << "KN v K";
    EXPECT_TRUE(drawnWith<Chess>("3bk3/8/8/8/8/8/8/4K3 w - - 0 1"))
        << "either side may hold it";
    EXPECT_FALSE(drawnWith<Chess>("3bk3/8/8/8/8/8/8/4KN2 w - - 0 1"))
        << "KN v KB is two minors";

    for (const std::string &rank : {"5P2/4K3", "8/4KR2", "8/4KQ2"}) {
        const std::string fen = "4k3/8/8/8/8/8/" + rank + " w - - 0 1";
        EXPECT_FALSE(drawnWith<Chess>(fen)) << fen << " can mate";
    }
}

// Shatranj bares the king, so a lone minor still wins there. Paradigm has
// knights but no bishops, and its Dragon is no minor.
TEST(PositionTest, InsufficientFollowsTheVariantsRules) {
    EXPECT_TRUE(drawnWith<Variant::Chaturanga>("4k3/8/8/8/8/8/8/4K3 w - - 0 1"));
    for (const char glyph : {'N', 'I', 'F'}) {
        const std::string fen =
            std::string("4k3/8/8/8/8/8/8/4K") + glyph + "2 w - - 0 1";
        EXPECT_FALSE(drawnWith<Variant::Chaturanga>(fen)) << fen << " still wins";
    }

    EXPECT_TRUE(drawnWith<Variant::Paradigm>("4k3/8/8/8/8/8/8/4KN2 w - - 0 1"));
    EXPECT_FALSE(drawnWith<Variant::Paradigm>("4k3/8/8/8/8/8/8/4KD2 w - - 0 1"));
}

TEST(PositionTest, MoveUCIWritesSquaresPromotionsAndCastles) {
    Position<Variant::Chess> p{};
    p.setStartPos();
    const int pawn = E::index(Piece::Pawn), queen = E::index(Piece::Queen),
              king = E::index(Piece::King);

    EXPECT_EQ(p.moveUCI(E::move({.from = E::at(2, 4),
                                 .to = E::at(4, 4),
                                 .moving = pawn,
                                 .doublePush = true})),
              "e2e4");
    EXPECT_EQ(p.moveUCI(E::move({.from = E::at(7, 0),
                                 .to = E::at(8, 0),
                                 .moving = pawn,
                                 .ending = queen})),
              "a7a8q");
    EXPECT_EQ(p.moveUCI(E::move({.from = E::at(1, 4),
                                 .to = E::at(1, 6),
                                 .moving = king,
                                 .castling = true})),
              "e1g1");
    EXPECT_EQ(p.moveUCI(E::move({.from = E::at(8, 4),
                                 .to = E::at(8, 2),
                                 .moving = king,
                                 .castling = true})),
              "e8c8");
}

// 960 GUIs want king-takes-rook, and a PV holds moves for both sides, so the
// formatter can consult neither the notation default nor toMove.
TEST(PositionTest, MoveUCIUsesKingTakesRookInFRC) {
    Position<Variant::Chess> p{};
    p.setStartPos();
    p.castles.isFRC = true;
    const int king = E::index(Piece::King);
    const auto castle = [&](std::size_t rank, std::size_t file) {
        return E::move({.from = E::at(rank, 4),
                        .to = E::at(rank, file),
                        .moving = king,
                        .castling = true});
    };

    EXPECT_EQ(p.moveUCI(castle(1, 6)), "e1h1");
    EXPECT_EQ(p.moveUCI(castle(1, 2)), "e1a1");
    EXPECT_EQ(p.moveUCI(castle(8, 6)), "e8h8");
    EXPECT_EQ(p.moveUCI(castle(8, 2)), "e8a8");

    p.makeMove(E::move({.from = E::at(1, 6),
                        .to = E::at(3, 5),
                        .moving = E::index(Piece::Knight)}));
    EXPECT_EQ(p.toMove, Black);
    EXPECT_EQ(p.moveUCI(castle(8, 6)), "e8h8") << "unchanged a ply later";

    // A king on the b-file is why the plain form is ambiguous: b1c1 would read
    // as an ordinary step.
    p.castles.arrangeCastling({E::at(8, 4), E::at(1, 1)},
                              {E::at(8, 7), E::at(1, 7)},
                              {E::at(8, 0), E::at(1, 0)});
    EXPECT_EQ(p.moveUCI(E::move({.from = E::at(1, 1),
                                 .to = E::at(1, 6),
                                 .moving = king,
                                 .castling = true})),
              "b1h1");
    EXPECT_EQ(p.moveUCI(E::move({.from = E::at(1, 1),
                                 .to = E::at(1, 2),
                                 .moving = king,
                                 .castling = true})),
              "b1a1");
}
