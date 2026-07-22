// Bitboard<M,N> tests, mostly typed over `Boards`. Squares are internal bit
// indices (r*innerCols + f). GoogleTest isn't a module -- std via includes.
#include <algorithm>
#include <bit>
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

import Tilted.Bitboard;

using Tilted::Bitboard;
using Tilted::Square;

// Geometry helpers derived from a Bitboard type, so typed tests can name
// squares by (rank, file) without reaching into the class internals.
template <class BB> struct Geo {
    static constexpr std::size_t M = BB::ranks();
    static constexpr std::size_t N = BB::cols();
    static constexpr std::size_t IC = std::bit_ceil(N);
    static constexpr std::size_t bitSpan = IC * M; // one past the last bit
    static constexpr Square bitOf(std::size_t r, std::size_t f) {
        return r * IC + f;
    }
    static BB one(std::size_t r, std::size_t f) {
        return BB::squareToBitboard(bitOf(r, f));
    }
};

template <class BB> class BitboardTest : public ::testing::Test {};

using Boards = ::testing::Types<
    // primary sizes
    Bitboard<4, 4>, Bitboard<6, 6>, Bitboard<8, 8>, Bitboard<8, 10>,
    Bitboard<14, 14>,
    // odd sizes
    Bitboard<3, 5>, Bitboard<5, 9>, Bitboard<11, 7>, Bitboard<13, 13>>;

TYPED_TEST_SUITE(BitboardTest, Boards);

TYPED_TEST(BitboardTest, EmptyState) {
    using G = Geo<TypeParam>;
    TypeParam e;
    EXPECT_TRUE(e.empty());
    EXPECT_EQ(e.count(), 0u);
    // The empty-scan sentinel is one past the last bit, never a real square.
    EXPECT_EQ(e.leastSquare(), G::bitSpan);
    EXPECT_EQ(e.mostSquare(), G::bitSpan);
}

TYPED_TEST(BitboardTest, SetGetRoundtrip) {
    using G = Geo<TypeParam>;
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f) {
            const Square s = G::bitOf(r, f);
            TypeParam b = G::one(r, f);
            EXPECT_EQ(b.count(), 1u) << "r=" << r << " f=" << f;
            EXPECT_TRUE(b.test(s));
            EXPECT_EQ(b.leastSquare(), s);
            EXPECT_EQ(b.mostSquare(), s);
        }
}

TYPED_TEST(BitboardTest, ToggleTwiceClears) {
    using G = Geo<TypeParam>;
    TypeParam b;
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f) {
            const Square s = G::bitOf(r, f);
            b.toggle(s);
            EXPECT_TRUE(b.test(s));
            b.toggle(s);
            EXPECT_FALSE(b.test(s));
        }
    EXPECT_TRUE(b.empty());
}

// ~empty must set exactly the M*N on-board squares and leave padding clear --
// proves boardMask is addressed by bit, not by dense square.
TYPED_TEST(BitboardTest, FullBoardComplement) {
    using G = Geo<TypeParam>;
    const TypeParam full = ~TypeParam{};
    EXPECT_EQ(full.count(), G::M * G::N);
    for (std::size_t r = 0; r < G::M; ++r) {
        for (std::size_t f = 0; f < G::N; ++f)
            EXPECT_TRUE(full.test(G::bitOf(r, f)))
                << "missing r=" << r << " f=" << f;
        for (std::size_t f = G::N; f < G::IC; ++f) // padding columns
            EXPECT_FALSE(full.test(G::bitOf(r, f)))
                << "padding set r=" << r << " f=" << f;
    }
    EXPECT_TRUE((full & ~full).empty());
    EXPECT_EQ((TypeParam{} | full), full);
}

TYPED_TEST(BitboardTest, Complement) {
    using G = Geo<TypeParam>;
    const TypeParam x = G::one(0, 0) | G::one(G::M - 1, G::N - 1);
    EXPECT_EQ(~(~x), x); // involution on a clean board
    EXPECT_TRUE((x & ~x).empty());
    EXPECT_EQ((x | ~x), ~TypeParam{});
}

TYPED_TEST(BitboardTest, Bitwise) {
    using G = Geo<TypeParam>;
    const TypeParam a = G::one(0, 0) | G::one(G::M - 1, G::N - 1);
    const TypeParam b = G::one(0, 0);
    EXPECT_EQ((a & b), b);
    EXPECT_EQ((a | b), a);
    EXPECT_EQ((a ^ b), G::one(G::M - 1, G::N - 1));
}

// Core geometry: each single-square board steps to its neighbor or off the
// edge -- exercises the raw shifts + wrap-guard masks across all sizes.
TYPED_TEST(BitboardTest, Directions) {
    using G = Geo<TypeParam>;
    const TypeParam none;
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f) {
            const TypeParam b = G::one(r, f);
            EXPECT_EQ(b.north(), r > 0 ? G::one(r - 1, f) : none)
                << "north r=" << r << " f=" << f;
            EXPECT_EQ(b.south(), r + 1 < G::M ? G::one(r + 1, f) : none)
                << "south r=" << r << " f=" << f;
            EXPECT_EQ(b.east(), f + 1 < G::N ? G::one(r, f + 1) : none)
                << "east r=" << r << " f=" << f;
            EXPECT_EQ(b.west(), f > 0 ? G::one(r, f - 1) : none)
                << "west r=" << r << " f=" << f;

            const bool ne = r > 0 && f + 1 < G::N;
            const bool nw = r > 0 && f > 0;
            const bool se = r + 1 < G::M && f + 1 < G::N;
            const bool sw = r + 1 < G::M && f > 0;
            EXPECT_EQ(b.northEast(), ne ? G::one(r - 1, f + 1) : none)
                << "NE r=" << r << " f=" << f;
            EXPECT_EQ(b.northWest(), nw ? G::one(r - 1, f - 1) : none)
                << "NW r=" << r << " f=" << f;
            EXPECT_EQ(b.southEast(), se ? G::one(r + 1, f + 1) : none)
                << "SE r=" << r << " f=" << f;
            EXPECT_EQ(b.southWest(), sw ? G::one(r + 1, f - 1) : none)
                << "SW r=" << r << " f=" << f;
        }
}

TYPED_TEST(BitboardTest, RankMirror) {
    using G = Geo<TypeParam>;
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f) {
            const TypeParam b = G::one(r, f);
            EXPECT_EQ(b.rankMirror(), G::one(G::M - 1 - r, f))
                << "r=" << r << " f=" << f;
            EXPECT_EQ(b.rankMirror().rankMirror(), b); // involution
        }
}

TYPED_TEST(BitboardTest, FlipRank) {
    using G = Geo<TypeParam>;
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f)
            EXPECT_EQ(TypeParam::flipRank(G::bitOf(r, f)),
                      G::bitOf(G::M - 1 - r, f))
                << "r=" << r << " f=" << f;
}

TYPED_TEST(BitboardTest, PopLeastAscending) {
    using G = Geo<TypeParam>;
    std::vector<Square> squares = {
        G::bitOf(0, 0),
        G::bitOf(0, G::N - 1),
        G::bitOf(G::M / 2, G::N / 2),
        G::bitOf(G::M - 1, 0),
        G::bitOf(G::M - 1, G::N - 1),
    };
    std::sort(squares.begin(), squares.end());
    squares.erase(std::unique(squares.begin(), squares.end()), squares.end());

    TypeParam b;
    for (Square s : squares)
        b.toggle(s);
    EXPECT_EQ(b.count(), squares.size());

    for (Square expected : squares) {
        EXPECT_EQ(b.leastSquare(), expected);
        EXPECT_EQ(b.popLeastSquare(), expected);
    }
    EXPECT_TRUE(b.empty());
    EXPECT_EQ(b.popLeastSquare(), G::bitSpan);
}

// A couple of concrete cases that pin the exact internal layout numbers, so a
// future change to the addressing scheme trips here with obvious values.
TEST(BitboardConcrete, Layout8x8) {
    using BB = Bitboard<8, 8>;
    EXPECT_TRUE(BB::squareToBitboard(0).test(0)); // bit 0 == rank0,file0
    EXPECT_EQ(BB::squareToBitboard(0).south().leastSquare(), 8u);
    EXPECT_EQ(BB::squareToBitboard(63).mostSquare(), 63u);
    EXPECT_EQ((~BB{}).count(), 64u);
}

// 3x5: innerCols=8, stored in two 16-bit words. Rank 2 (bit 16) lives in the
// second word, so north/south here cross a word boundary.
TEST(BitboardConcrete, CrossWord3x5) {
    using BB = Bitboard<3, 5>;
    EXPECT_EQ(BB::squareToBitboard(16).north().leastSquare(), 8u);
    EXPECT_EQ(BB::squareToBitboard(8).south().leastSquare(), 16u);
    EXPECT_EQ((~BB{}).count(), 15u);
    EXPECT_FALSE((~BB{}).test(5)); // padding column
}
