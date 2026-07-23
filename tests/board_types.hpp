#pragma once
// Shared typed-test scaffolding for the Bitboard/Attacks suites. Include it
// after `import Tilted.Bitboard;` and <gtest/gtest.h>, whose names (Bitboard,
// Square, testing::Types) it relies on.

// Geometry helper: name squares by (rank, file) without reaching into the class
// internals.
template <class BB> struct Geo {
    static constexpr std::size_t M = BB::ranks();
    static constexpr std::size_t N = BB::cols();
    static constexpr std::size_t IC = std::bit_ceil(N);
    static constexpr std::size_t bitSpan = IC * M; // one past the last bit
    static constexpr Tilted::Square bitOf(std::size_t r, std::size_t f) {
        return r * IC + f;
    }
    static BB one(std::size_t r, std::size_t f) {
        return BB::squareToBitboard(bitOf(r, f));
    }
};

// Board sizes exercised by every typed suite: primary and odd, single- and
// multi-word.
using Boards = ::testing::Types<
    Tilted::Bitboard<4, 4>, Tilted::Bitboard<6, 6>, Tilted::Bitboard<8, 8>,
    Tilted::Bitboard<8, 10>, Tilted::Bitboard<14, 14>, Tilted::Bitboard<3, 5>,
    Tilted::Bitboard<5, 9>, Tilted::Bitboard<11, 7>, Tilted::Bitboard<13, 13>>;
