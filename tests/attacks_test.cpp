// Attack-generation tests: each piece's attack function is cross-checked
// against an independent brute-force ray/leg walk over empty, full, and
// Util::Random occupancies at every square, across single- and multi-word
// boards.
#include <bit>
#include <cstddef>

#include <gtest/gtest.h>

import Tilted.Bitboard;
import Tilted.Attacks;
import Tilted.Util;

using Tilted::Bitboard;
using Tilted::Square;
using Tilted::Util::Random;

#include "board_types.hpp"

template <class BB> class AttacksTest : public ::testing::Test {};

TYPED_TEST_SUITE(AttacksTest, Boards);

// A pseudo-random occupancy from the engine's splitmix64 device.
template <class BB> BB randomOccupancy(Random &rng) {
    using G = Geo<BB>;
    BB occ;
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f)
            if (rng() & 1)
                occ.toggle(G::bitOf(r, f));
    return occ;
}

// Compare an attack function against its brute-force reference over empty,
// full, and several random occupancies at every square. `attack(s, occ)`
// deduces the board size from occ; `reference(r, f, occ)` walks the geometry
// directly.
template <class BB, class Attack, class Reference>
void expectMatches(Attack attack, Reference reference) {
    using G = Geo<BB>;
    Random rng{0x9E3779B97F4A7C15ULL};
    const BB occs[] = {BB{}, ~BB{}, randomOccupancy<BB>(rng),
                       randomOccupancy<BB>(rng), randomOccupancy<BB>(rng)};
    for (const BB &occ : occs)
        for (std::size_t r = 0; r < G::M; ++r)
            for (std::size_t f = 0; f < G::N; ++f) {
                const Square s = G::bitOf(r, f);
                const BB got = attack(s, occ);
                EXPECT_EQ(got, reference(r, f, occ)) << "r=" << r << " f=" << f;
            }
}

// Brute-force diagonal ray walk from (r,f), first blocker included.
template <class BB> BB bishopRef(std::size_t r, std::size_t f, const BB &occ) {
    using G = Geo<BB>;
    constexpr int dirs[4][2] = {{-1, 1}, {-1, -1}, {1, 1}, {1, -1}};
    BB out;
    for (const auto &d : dirs) {
        int rr = static_cast<int>(r), ff = static_cast<int>(f);
        for (;;) {
            rr += d[0];
            ff += d[1];
            if (rr < 0 || rr >= static_cast<int>(G::M) || ff < 0 ||
                ff >= static_cast<int>(G::N))
                break;
            const Square sq = G::bitOf(static_cast<std::size_t>(rr),
                                       static_cast<std::size_t>(ff));
            out.toggle(sq);
            if (occ.test(sq))
                break;
        }
    }
    return out;
}

// Brute-force orthogonal ray walk from (r,f), first blocker included.
template <class BB> BB rookRef(std::size_t r, std::size_t f, const BB &occ) {
    using G = Geo<BB>;
    constexpr int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    BB out;
    for (const auto &d : dirs) {
        int rr = static_cast<int>(r), ff = static_cast<int>(f);
        for (;;) {
            rr += d[0];
            ff += d[1];
            if (rr < 0 || rr >= static_cast<int>(G::M) || ff < 0 ||
                ff >= static_cast<int>(G::N))
                break;
            const Square sq = G::bitOf(static_cast<std::size_t>(rr),
                                       static_cast<std::size_t>(ff));
            out.toggle(sq);
            if (occ.test(sq))
                break;
        }
    }
    return out;
}

// Brute-force Xiangqi horse: a knight move is legal only if the orthogonal leg
// square (one step toward the target's magnitude-2 axis) is empty.
template <class BB> BB horseRef(std::size_t r, std::size_t f, const BB &occ) {
    using G = Geo<BB>;
    constexpr int moves[8][2] = {{-2, -1}, {-2, 1}, {2, -1}, {2, 1},
                                 {-1, -2}, {-1, 2}, {1, -2}, {1, 2}};
    BB out;
    for (const auto &m : moves) {
        const int tr = static_cast<int>(r) + m[0];
        const int tf = static_cast<int>(f) + m[1];
        if (tr < 0 || tr >= static_cast<int>(G::M) || tf < 0 ||
            tf >= static_cast<int>(G::N))
            continue;
        int lr = static_cast<int>(r), lf = static_cast<int>(f);
        lr += (m[0] == 2) - (m[0] == -2);
        lf += (m[1] == 2) - (m[1] == -2);
        const Square leg = G::bitOf(static_cast<std::size_t>(lr),
                                    static_cast<std::size_t>(lf));
        if (occ.test(leg))
            continue; // hobbled
        out.toggle(G::bitOf(static_cast<std::size_t>(tr),
                            static_cast<std::size_t>(tf)));
    }
    return out;
}

// Brute-force grasshopper: along each of the 8 queen directions, find the first
// occupied square (the hurdle) and land on the square just beyond it.
template <class BB>
BB grasshopperRef(std::size_t r, std::size_t f, const BB &occ) {
    using G = Geo<BB>;
    constexpr int dirs[8][2] = {{-1, 0},  {1, 0},  {0, -1}, {0, 1},
                                {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    BB out;
    for (const auto &d : dirs) {
        int rr = static_cast<int>(r), ff = static_cast<int>(f);
        bool found = false;
        while (true) {
            rr += d[0];
            ff += d[1];
            if (rr < 0 || rr >= static_cast<int>(G::M) || ff < 0 ||
                ff >= static_cast<int>(G::N))
                break;
            if (occ.test(G::bitOf(static_cast<std::size_t>(rr),
                                  static_cast<std::size_t>(ff)))) {
                found = true;
                break;
            }
        }
        if (!found)
            continue; // no hurdle in this direction
        const int lr = rr + d[0], lf = ff + d[1];
        if (lr < 0 || lr >= static_cast<int>(G::M) || lf < 0 ||
            lf >= static_cast<int>(G::N))
            continue; // hurdle on the rim -- nowhere to land
        out.toggle(G::bitOf(static_cast<std::size_t>(lr),
                            static_cast<std::size_t>(lf)));
    }
    return out;
}

TYPED_TEST(AttacksTest, Bishop) {
    expectMatches<TypeParam>(
        [](Square s, const TypeParam &o) {
            return Tilted::Attacks::BishopAttacks(s, o);
        },
        bishopRef<TypeParam>);
}

TYPED_TEST(AttacksTest, Rook) {
    expectMatches<TypeParam>(
        [](Square s, const TypeParam &o) {
            return Tilted::Attacks::RookAttacks(s, o);
        },
        rookRef<TypeParam>);
}

TYPED_TEST(AttacksTest, Horse) {
    expectMatches<TypeParam>(
        [](Square s, const TypeParam &o) {
            return Tilted::Attacks::HorseAttacks(s, o);
        },
        horseRef<TypeParam>);
}

TYPED_TEST(AttacksTest, Grasshopper) {
    expectMatches<TypeParam>(
        [](Square s, const TypeParam &o) {
            return Tilted::Attacks::GrasshopperAttacks(s, o);
        },
        grasshopperRef<TypeParam>);
}

// PieceAttacks routes each piece to its primitive/composite; the if-constexpr
// chain instantiates only the requested branch, so a piece a variant never uses
// costs it nothing.
TYPED_TEST(AttacksTest, Dispatch) {
    using G = Geo<TypeParam>;
    namespace A = Tilted::Attacks;
    using Tilted::Piece;
    Random rng{0xD1B54A32D192ED03ULL};
    const TypeParam occ = randomOccupancy<TypeParam>(rng);
    for (std::size_t r = 0; r < G::M; ++r)
        for (std::size_t f = 0; f < G::N; ++f) {
            const Square s = G::bitOf(r, f);
            EXPECT_EQ((A::PieceAttacks<Piece::Knight>(s, occ)),
                      (A::KnightAttacks<G::M, G::N>(s)));
            EXPECT_EQ((A::PieceAttacks<Piece::Bishop>(s, occ)),
                      A::BishopAttacks(s, occ));
            EXPECT_EQ((A::PieceAttacks<Piece::Rook>(s, occ)),
                      A::RookAttacks(s, occ));
            EXPECT_EQ((A::PieceAttacks<Piece::Queen>(s, occ)),
                      (A::BishopAttacks(s, occ) | A::RookAttacks(s, occ)));
            EXPECT_EQ((A::PieceAttacks<Piece::Horse>(s, occ)),
                      A::HorseAttacks(s, occ));
            EXPECT_EQ((A::PieceAttacks<Piece::Amazon>(s, occ)),
                      (A::BishopAttacks(s, occ) | A::RookAttacks(s, occ) |
                       A::KnightAttacks<G::M, G::N>(s)));
            EXPECT_EQ((A::PieceAttacks<Piece::Grasshopper>(s, occ)),
                      A::GrasshopperAttacks(s, occ));
        }
}
