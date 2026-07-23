export module Tilted.Move;

import std;
import Tilted.Consts;

export namespace Tilted {

template <Variant V> class Move {
  public:
    std::uint32_t data = 0;

    static constexpr std::size_t pieceWidth =
        std::bit_width(Ruleset<V>::types - 1);
    static constexpr std::size_t squareWidth = std::bit_width(
        Ruleset<V>::dims.ranks * std::bit_ceil(Ruleset<V>::dims.cols) - 1);

    static constexpr std::size_t fromOffset = 0;
    static constexpr std::size_t toOffset = squareWidth;
    static constexpr std::size_t movingOffset = 2 * squareWidth;
    static constexpr std::size_t endingOffset = movingOffset + pieceWidth;

    static constexpr std::size_t flagOffset = endingOffset + pieceWidth;

    Move() = default;

    Square from() const;
    Square to() const;
    int moving() const;
    int ending() const;

    bool castling() const
        requires(Ruleset<V>::castling);

    bool enPassant() const
        requires(Ruleset<V>::enPassant);
    bool doublePush() const
        requires(Ruleset<V>::enPassant);

    bool drop() const
        requires(Ruleset<V>::pocket);

    Square duck() const
        requires(V == Variant::Duck);
};

} // namespace Tilted
