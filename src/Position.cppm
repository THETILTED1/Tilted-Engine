export module Position;

import std;
export import Variants;
export import Attacks;
export import Move;
export import Util;

export namespace Tilted {

template <Variant V> struct Castles {
    std::array<std::uint8_t, MAX_HISTORY_LEN>
        castleRights; // last 4 bits q k Q K

    std::array<Square, 2> kingFrom, kingRookFrom, queenRookFrom;

    static constexpr Square whiteBack =
        (Bits<V>::ranks() - 1) * Bits<V>::innerCols();

    static constexpr std::array<Square, 2> kingRookTo{
        Bits<V>::cols() - 3, whiteBack + Bits<V>::cols() - 3};
    static constexpr std::array<Square, 2> queenRookTo{3, whiteBack + 3};
    static constexpr std::array<Square, 2> kingKingTo{
        Bits<V>::cols() - 2, whiteBack + Bits<V>::cols() - 2};
    static constexpr std::array<Square, 2> kingQueenTo{2, whiteBack + 2};

    std::array<Bits<V>, 2> kingSafeMask;
    std::array<Bits<V>, 2> queenSafeMask;

    std::array<Bits<V>, 2> kingOccMask;
    std::array<Bits<V>, 2> queenOccMask;

    std::array<std::uint8_t, Bits<V>::noSquare()> rightsChange;

    [[no_unique_address]] Util::Conditional<V == Variant::Chess, bool> isFRC;

    std::uint8_t rights(const Color &c) {
        return std::uint8_t(0b11 << (2 * (White - c)));
    }

    // A king or rook on noSquare() drops the rights it would have held.
    void arrangeCastling(const std::array<Square, 2> &kings,
                         const std::array<Square, 2> &kingRooks,
                         const std::array<Square, 2> &queenRooks);

    std::array<std::string, 16> castleStrings = {
        "-", "K",  "Q",  "KQ",  "k",  "Kk",  "Qk",  "KQk",
        "q", "Kq", "Qq", "KQq", "kq", "Kkq", "Qkq", "KQkq"};
};

template <Variant V>
    requires(Ruleset<V>::Supported)
class Position {
  public:
    Position() = default;

    std::array<Bits<V>, Ruleset<V>::types> pieces{};
    std::array<Bits<V>, 2> sides{};
    Color toMove;

    std::array<Hash, MAX_HISTORY_LEN> hashes;
    std::array<int, MAX_HISTORY_LEN> halfMoves;
    std::array<Move<V>, MAX_HISTORY_LEN> plays;

    int clock;

    [[no_unique_address]] Util::Conditional<Ruleset<V>::Checky != 0,
                                            std::array<std::size_t, 2>> checks;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Nonrectangle, Bits<V>>
        wall;
    [[no_unique_address]] Util::Conditional<
        Ruleset<V>::Pocket, Util::Table<std::size_t, 2, Ruleset<V>::types>>
        pockets;
    [[no_unique_address]] Util::Conditional<
        Ruleset<V>::Petrified || V == Variant::Duck, Bits<V>> bricks;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Points,
                                            std::array<std::size_t, 2>> points;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Hill, Bits<V>> hill;
    [[no_unique_address]] Util::Conditional<
        Ruleset<V>::EnPassant, std::array<Square, MAX_HISTORY_LEN>> enPassant;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Castling, Castles<V>>
        castles;

    int pieceAt(const Square &) const;

    Bits<V> isAttacked(const Square &, const Color &) const;

    Bits<V> those(const Color &c, const int &type) const {
        return pieces[type] & sides[c];
    }
    Bits<V> any(const int &type) const { return pieces[type]; }

    // Union over whichever of `ps` this variant actually has.
    Bits<V> anyOf(std::span<const Piece> ps) const {
        Bits<V> result{};
        for (Piece p : ps)
            if (const int x = PieceIndex<V>(p); x >= 0)
                result |= pieces[x];
        return result;
    }
    Bits<V> side(const Color &c) const { return sides[c]; }
    Bits<V> occupied() const { return sides[Black] | sides[White]; }

    int sinceReset() const { return halfMoves[clock]; }
    Hash thisHash() const { return hashes[clock]; }
    Move<V> lastPlayed() const { return plays[clock]; }

    std::string moveUCI(const Move<V> &) const;

    void readFen(std::string);
    std::string makeFen() const;

    void empty();
    void setStartPos();

    void print();

    void beginZobrist();
    int repetitions(int ply = 0) const;

    void forget();

    void makeMove(const Move<V> &);
    void unmakeMove();

    void passMove();
    void unpassMove();

    Bits<V> isChecked(const Color &) const
        requires(Ruleset<V>::Royal >= 0);

    bool insufficient() const
        requires(Ruleset<V>::Insufficient != 0);

    int thisPassant() const
        requires(Ruleset<V>::EnPassant)
    {
        return enPassant[clock];
    }

    bool onlyPawns() const
        requires(PieceIndex<V>(Piece::Pawn) >= 0);
};

} // namespace Tilted
