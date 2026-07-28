export module Move;

import std;
import Consts;

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

    // Bit index to algebraic. Bit 0 is a8, so the rank counts down from the top.
    // Digits are emitted by hand: std::to_string isn't constexpr, and boards up to
    // MAX_RANKS need two of them.
    static constexpr std::string algebraic(Square s) {
        constexpr std::size_t innerCols = std::bit_ceil(Ruleset<V>::dims.cols);
        const std::size_t rank = Ruleset<V>::dims.ranks - s / innerCols;
        std::string out(1, static_cast<char>('a' + s % innerCols));
        if (rank >= 10)
            out += static_cast<char>('0' + rank / 10);
        out += static_cast<char>('0' + rank % 10);
        return out;
    }

    Square from() const;
    Square to() const;
    int moving() const;
    int ending() const;

    bool castling() const
        requires(Ruleset<V>::Castling);

    bool enPassant() const
        requires(Ruleset<V>::EnPassant);
    bool doublePush() const
        requires(Ruleset<V>::EnPassant);

    bool drop() const
        requires(Ruleset<V>::Pocket);

    Square duck() const
        requires(V == Variant::Duck);

    std::string moveUCIstr(bool flip = false) const;
};

} // namespace Tilted
