export module Tilted.Zobrist;

import std;
import Tilted.Consts;
import Tilted.Util;

namespace Tilted {

inline constexpr Hash ZOBRIST_SEED = 0x9E3779B97F4A7C15;

// One random key per (color, dense piece index, square), densely filled from a
// single splitmix stream. Sized to the largest board and piece count, so this
// one table serves every variant: keys need only be distinct within a game
// (never across board sizes), and any board's square index is < MAX_SQUARES.
export constexpr Hash Zobrist(Color color, std::size_t piece, Square square) {
    static constexpr auto keys = [] {
        constexpr std::size_t MAX_SQUARES = MAX_RANKS * std::bit_ceil(MAX_FILES);
        Util::Table<Hash, 2, MAX_TYPES, MAX_SQUARES> table{};
        Util::Random rng{ZOBRIST_SEED};
        for (auto &c : table)
            for (auto &p : c)
                for (auto &sq : p)
                    sq = rng();
        return table;
    }();
    return keys[color][piece][square];
}

} // namespace Tilted
