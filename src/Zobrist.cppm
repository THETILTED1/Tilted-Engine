export module Zobrist;

import std;
export import Consts;
import Bitboard;
import Util;

namespace Tilted::Zobrist {

constexpr Hash SEED = 0xD1B54A32D192ED03;

// Bits<V>::noSquare() is one past the last bit, so it is exactly the padded
// square count -- the row stride for a variant's slice of the piece table.
template <Variant V>
constexpr std::size_t footprint = Ruleset<V>::types * Bits<V>::noSquare();

constexpr auto footprints = []<std::size_t... V>(std::index_sequence<V...>) {
    return std::array{footprint<static_cast<Variant>(V)>...};
}(std::make_index_sequence<VARIANTS>{});

constexpr std::size_t MAX_KEYS = [] {
    std::size_t max = 0;
    for (std::size_t f : footprints)
        max = std::max(max, f);
    return max;
}();

constexpr auto pieceKeys = [] {
    std::array<std::array<Hash, MAX_KEYS>, 2> table{};
    Util::Random rng{SEED};
    for (auto &color : table)
        for (auto &k : color)
            k = rng();
    return table;
}();

constexpr auto castlingKeys = [] {
    std::array<Hash, 16> table{};
    Util::Random rng{SEED + 1};
    for (auto &k : table)
        k = rng();
    return table;
}();

constexpr auto enPassantKeys = [] {
    std::array<Hash, MAX_FILES> table{};
    Util::Random rng{SEED + 2};
    for (auto &k : table)
        k = rng();
    return table;
}();

constexpr auto pocketKeys = [] {
    Util::Table<Hash, 2, MAX_DROP_TYPES, MAX_IN_HAND + 1> table{};
    Util::Random rng{SEED + 3};
    for (auto &color : table)
        for (auto &type : color)
            for (auto &count : type)
                count = rng();
    return table;
}();

constexpr auto brickKeys = [] {
    std::array<Hash, MAX_RANKS * std::bit_ceil(MAX_FILES)> table{};
    Util::Random rng{SEED + 4};
    for (auto &k : table)
        k = rng();
    return table;
}();

constexpr Hash turnKey = 0xC1A5537E2D9B4E86;

export template <Variant V>
constexpr Hash piece(Color color, std::size_t type, Square square) {
    return pieceKeys[color][type * Bits<V>::noSquare() + square];
}

// Key for a castling-rights mask (0..15).
export constexpr Hash castling(std::size_t rights) {
    return castlingKeys[rights];
}

// Key for the file of an en-passant target square.
export constexpr Hash enPassant(std::size_t file) {
    return enPassantKeys[file];
}

// Key for holding `count` of droppable type `type` in `color`'s pocket.
export constexpr Hash pocket(Color color, std::size_t type, std::size_t count) {
    return pocketKeys[color][type][count];
}

export constexpr Hash brick(Square square) { return brickKeys[square]; }

// Key toggled when the side to move flips.
export constexpr Hash turn() { return turnKey; }

} // namespace Tilted::Zobrist
