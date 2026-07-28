export module Tilted.Position;

import std;
import Tilted.Attacks; // transitively re-exports Bitboard + Consts
import Tilted.Move;
import Tilted.Util;
import Tilted.Zobrist;

export namespace Tilted {

template <Variant V> using Bits = Bitboard<Ruleset<V>::dims.ranks, Ruleset<V>::dims.cols>;

template <Variant V> struct Castles{
    std::array<std::uint8_t, MAX_HISTORY_LEN> castleRights; // last 4 bits q k Q K

    std::array<Square, 2> kingRookFrom, queenRookFrom;

    // Bit 0 is a8, so Black castles on rank 0 and its destinations are bare
    // files; White's are those files on the last rank. Indexed by Color.
    static constexpr Square whiteBack =
        (Bits<V>::ranks() - 1) * std::bit_ceil(Bits<V>::cols());

    // King-side lands the king 2nd from the right edge with the rook just inside
    // it, queen-side the 3rd and 4th from the left. Taken relative to width that
    // is 8x8's g1/f1 and c1/d1, and also Gothic's i1/h1 and XXL's m1/l1.
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

    std::array<std::uint8_t, Bits<V>::ranks() * std::bit_ceil(Bits<V>::cols())> rightsChange;

    std::uint8_t rights(const Color&);

    std::array<std::string, 16> castleStrings = 
    {"-", "K", "Q", "KQ",
    "k", "Kk", "Qk", "KQk",
    "q", "Kq", "Qq", "KQq",
    "kq", "Kkq", "Qkq", "KQkq"};
};

// Full game state for a variant. Board geometry, piece set, and which optional
// state even exists (castling, en passant, pockets, ...) all come from
// Ruleset<V>, so each variant instantiates a distinct, minimally-sized Position.
template <Variant V> 
    requires (Ruleset<V>::Supported)
class Position {
  public:
    Position() = default;

    std::array<Bits<V>, Ruleset<V>::types> pieces{};
    std::array<Bits<V>, 2> sides{};
    Color toMove;

    [[no_unique_address]] Util::Conditional<Ruleset<V>::Pocket,
        Util::Table<std::size_t, 2, Ruleset<V>::types>> pockets;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Castling, Castles<V>> castles;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::EnPassant,
        std::array<Square, MAX_HISTORY_LEN>> enPassant;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Points,
        std::array<std::size_t, 2>> points;

    std::array<Hash, MAX_HISTORY_LEN> hashes;
    std::array<int, MAX_HISTORY_LEN> halfMoves;
    std::array<Move<V>, MAX_HISTORY_LEN> plays;

    std::size_t clock;

    // Duck has no Ruleset flag of its own; Move<V>::duck() gates the same way.
    [[no_unique_address]] Util::Conditional<V == Variant::Duck, Bits<V>> duck;
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Hill, Bits<V>> hill;
    // Bricks are both the squares cut out of a non-rectangular board and the ones
    // a petrified capture leaves behind, so either rule needs the mask.
    [[no_unique_address]] Util::Conditional<Ruleset<V>::Nonrectangle || Ruleset<V>::Petrified,
        Bits<V>> bricks;

    // Position();
    // Position(const Position&);
    // equals operator

    int pieceAt(const Square&) const;

    bool insufficient() const
        requires(Ruleset<V>::Insufficient);

    Bits<V> isAttacked(const Square&, const Color&) const;
    Bits<V> isChecked(const Color&) const;

    Bits<V> those(const Color&, const int&);
    Bits<V> any(const int&);
    Bits<V> side(const Color&);
    Bits<V> occupied();

    // Sliding-piece groups only mean something where the variant fields them, so
    // these gate on the piece being present in Ruleset<V>'s mapping.
    Bits<V> rooks()
        requires(PieceIndex<V>(Piece::Rook) >= 0);
    Bits<V> bishops()
        requires(PieceIndex<V>(Piece::Bishop) >= 0);

    bool onlyPawns() const
        requires(PieceIndex<V>(Piece::Pawn) >= 0);


    int sinceReset() const{ return halfMoves[clock]; }
    int thisPassant() const
        requires(Ruleset<V>::EnPassant)
    { return enPassant[clock]; }
    Hash thisHash() const{ return hashes[clock]; }
    Move<V> lastPlayed() const{ return plays[clock]; }

    // void readFen(std::string);
    // std::string makeFen() const;

    void empty();
    void setStartPos();

    void print();

    void beginZobrist();
    // void showZobrist() const;
    int repetitions(int) const;

    // void forget();

    void makeMove(const Move<V>&);
    void unmakeMove();

    void passMove();
    void unpassMove();

    // Shuffled back ranks are a standard-chess option only.
    [[no_unique_address]] Util::Conditional<V == Variant::Chess, bool> isFRC;

};

} // namespace Tilted
