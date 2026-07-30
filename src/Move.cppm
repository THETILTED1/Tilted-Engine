export module Move;

import std;
export import Consts;
import Bitboard;

export namespace Tilted {

template <Variant V> class Move {
  private:
    static constexpr std::size_t pieceWidth =
        std::bit_width(Ruleset<V>::types - 1);
    static constexpr std::size_t squareWidth =
        std::bit_width(Bits<V>::noSquare() - 1);

    static constexpr std::size_t fromOffset = 0;
    static constexpr std::size_t toOffset = squareWidth;
    static constexpr std::size_t movingOffset = 2 * squareWidth;
    static constexpr std::size_t endingOffset = movingOffset + pieceWidth;
    static constexpr std::size_t victimOffset = endingOffset + pieceWidth;

    static constexpr std::size_t flagOffset = victimOffset + pieceWidth;

    static constexpr std::size_t flagCount = 1 + Ruleset<V>::Castling +
                                             Ruleset<V>::EnPassant * 2 +
                                             Ruleset<V>::Pocket;

    static constexpr std::size_t capturingBit = flagOffset;
    static constexpr std::size_t castlingBit = capturingBit + 1;
    static constexpr std::size_t enPassantBit =
        castlingBit + Ruleset<V>::Castling;
    static constexpr std::size_t doublePushBit = enPassantBit + 1;
    static constexpr std::size_t dropBit =
        enPassantBit + 2 * Ruleset<V>::EnPassant;

    static constexpr std::size_t duckOffset = dropBit + Ruleset<V>::Pocket;

    static_assert(duckOffset == flagOffset + flagCount);

    static_assert(flagOffset + flagCount + squareWidth * (V == Variant::Duck) <=
                  8 * sizeof(std::uint32_t));

    constexpr std::uint32_t substring(std::size_t offset,
                                      std::size_t width) const {
        return (data >> offset) & ((std::uint32_t(1) << width) - 1);
    }

  public:
    std::uint32_t data = 0;

    Move() = default;
    constexpr explicit Move(std::uint32_t bits) : data(bits) {}

    static constexpr Move null() { return {}; }
    static constexpr Move invalid() { return Move{~std::uint32_t(0)}; }

    static constexpr std::string algebraic(Square s) {
        const std::size_t rank = Bits<V>::ranks() - s / Bits<V>::innerCols();
        std::string out(1, static_cast<char>('a' + s % Bits<V>::innerCols()));
        if (rank >= 10)
            out += static_cast<char>('0' + rank / 10);
        out += static_cast<char>('0' + rank % 10);
        return out;
    }

    constexpr Square from() const { return substring(fromOffset, squareWidth); }
    constexpr Square to() const { return substring(toOffset, squareWidth); }
    constexpr int moving() const { return substring(movingOffset, pieceWidth); }
    constexpr int ending() const { return substring(endingOffset, pieceWidth); }
    constexpr int victim() const { return substring(victimOffset, pieceWidth); }

    constexpr bool capturing() const { return substring(capturingBit, 1); }

    constexpr bool castling() const
        requires(Ruleset<V>::Castling)
    {
        return substring(castlingBit, 1);
    }

    constexpr bool enPassant() const
        requires(Ruleset<V>::EnPassant)
    {
        return substring(enPassantBit, 1);
    }
    constexpr bool doublePush() const
        requires(Ruleset<V>::EnPassant)
    {
        return substring(doublePushBit, 1);
    }

    constexpr bool drop() const
        requires(Ruleset<V>::Pocket)
    {
        return substring(dropBit, 1);
    }

    constexpr Square duck() const
        requires(V == Variant::Duck)
    {
        return substring(duckOffset, squareWidth);
    }

    constexpr std::string moveUCIstr() const {
        return algebraic(from()) + algebraic(to());
    }
};

} // namespace Tilted
