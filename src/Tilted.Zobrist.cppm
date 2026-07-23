export module Tilted.Zobrist;

import std;
import Tilted.Consts;
import Tilted.Util;

namespace Tilted::Zobrist {

constexpr Hash SEED = 0xD1B54A32D192ED03;

// Padded square count and total key count of a variant, from its Ruleset.
template <Variant V>
constexpr std::size_t squares =
    Ruleset<V>::dims.ranks * std::bit_ceil(Ruleset<V>::dims.cols);

template <Variant V>
constexpr std::size_t footprint = Ruleset<V>::types * squares<V>;

// None sits just past the real variants, so its ordinal is exactly their count.
constexpr std::size_t VARIANTS = static_cast<std::size_t>(Variant::None);

// Each variant's footprint. This can't be a loop: footprint<V> selects the
// Ruleset<V> template, whose argument must be a constant, so the variants are
// reached through a compile-time pack instead of a runtime counter.
constexpr auto footprints = []<std::size_t... V>(std::index_sequence<V...>) {
    return std::array{footprint<static_cast<Variant>(V)>...};
}(std::make_index_sequence<VARIANTS>{});

// Flat piece table size: the largest footprint over every variant, not
// maxTypes*maxSquares, so nothing is wasted when the board and piece maxima
// live in different variants. Rows pack as [type * squares<V> + square] /
// color.
constexpr std::size_t MAX_KEYS = [] {
    std::size_t max = 0;
    for (std::size_t f : footprints)
        max = std::max(max, f);
    return max;
}();

// Each group has its own compile-time stream, seeds nudged apart so they stay
// disjoint; the turn key is a lone toggle, so just a fixed constant.
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

constexpr Hash turnKey = 0xC1A5537E2D9B4E86;

// Key for a piece `type` (dense PieceMapping index) of `color` on `square`.
// Templated on the variant so the row stride is a compile-time constant.
export template <Variant V>
constexpr Hash piece(Color color, std::size_t type, Square square) {
    return pieceKeys[color][type * squares<V> + square];
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

// Key toggled when the side to move flips.
export constexpr Hash turn() { return turnKey; }

} // namespace Tilted::Zobrist
