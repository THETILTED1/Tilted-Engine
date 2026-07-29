export module Move;

import std;
export import Consts;
import Bitboard;

export namespace Tilted {

template <Variant V> class Move {
  public:
    std::uint32_t data = 0;

    static constexpr std::size_t pieceWidth =
        std::bit_width(Ruleset<V>::types - 1);
    static constexpr std::size_t squareWidth =
        std::bit_width(Bits<V>::noSquare() - 1);

    static constexpr std::size_t fromOffset = 0;
    static constexpr std::size_t toOffset = squareWidth;
    static constexpr std::size_t movingOffset = 2 * squareWidth;
    static constexpr std::size_t endingOffset = movingOffset + pieceWidth;

    static constexpr std::size_t flagOffset = endingOffset + pieceWidth;

    Move() = default;
    constexpr explicit Move(std::uint32_t bits) : data(bits) {}

    static constexpr Move null() { return {}; }
    static constexpr Move invalid() { return Move{~std::uint32_t(0)}; }

    static constexpr std::string algebraic(Square s) {
        const std::size_t rank = Bits<V>::ranks() - s / Bits<V>::innerCols();
        std::string out(1,
                        static_cast<char>('a' + s % Bits<V>::innerCols()));
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
