// Implementation unit for Position: out-of-line member definitions plus
// the explicit instantiations, so function bodies stay out of the BMI and
// editing them doesn't force importers to rebuild.
module Position;

import std;
import Zobrist;

namespace Tilted {

template <Variant V>
    requires(Ruleset<V>::Supported)
int Position<V>::pieceAt(const Square &s) const {
    for (std::size_t t = 0; t < Ruleset<V>::types; ++t)
        if (pieces[t].test(s))
            return static_cast<int>(t);
    return -1;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
bool Position<V>::insufficient() const
    requires(Ruleset<V>::Insufficient)
{
    return false; // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
Bits<V> Position<V>::isAttacked(const Square &s, const Color &c) const {
    return {}; // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
Bits<V> Position<V>::isChecked(const Color &c) const {
    return {}; // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
Bits<V> Position<V>::rooks() const {
    Bits<V> result{};
    for (Piece p :
         {Piece::Rook, Piece::Queen, Piece::Chancellor, Piece::Amazon})
        if (const int x = PieceIndex<V>(p); x >= 0)
            result |= pieces[x];
    return result;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
Bits<V> Position<V>::bishops() const {
    Bits<V> result{};
    for (Piece p : {Piece::Bishop, Piece::Queen, Piece::Dragon,
                    Piece::Archbishop, Piece::Amazon})
        if (const int x = PieceIndex<V>(p); x >= 0)
            result |= pieces[x];
    return result;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
bool Position<V>::onlyPawns() const
    requires(PieceIndex<V>(Piece::Pawn) >= 0)
{
    Bits<V> spared = pieces[PieceIndex<V>(Piece::Pawn)];
    if constexpr (Ruleset<V>::Royal >= 0)
        spared |= pieces[Ruleset<V>::Royal];
    return (occupied() & ~spared).empty();
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::empty() {
    pieces = {};
    sides = {};
    toMove = Black;

    hashes = {};
    halfMoves = {};
    plays = {};
    clock = 0;

    checks = {};
    wall = {};
    pockets = {};
    bricks = {};
    points = {};
    hill = {};
    if constexpr (Ruleset<V>::EnPassant)
        enPassant.fill(Bits<V>::noSquare());
    castles = {};
    isFRC = {};

    beginZobrist();
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::place(std::span<const Piece> back) {
    for (Square f = 0; f < back.size(); ++f)
        pieces[PieceIndex<V>(back[f])] |= Bits<V>::squareToBitboard(f);
    pieces[PieceIndex<V>(Piece::Pawn)] |=
        Bits<V>::rankMask(Bits<V>::innerCols());

    for (const Bits<V> &b : pieces)
        sides[Black] |= b;
}

// Reflects a board holding nothing but Black's men into both sides.
template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::mirror() {
    for (Bits<V> &b : pieces)
        b |= b.rankMirror();
    sides[White] = sides[Black].rankMirror();
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::setStartPos() {
    empty();

    using enum Piece;
    constexpr std::array standard{Rook, Knight, Bishop, Queen,
                                  King, Bishop, Knight, Rook};

    if constexpr (V == Variant::Chess || V == Variant::Antichess) {
        place(standard);
        mirror();

    } else if constexpr (V == Variant::Horde) {
        place(standard);

        // White is a pawn horde, not Black's reflection: the four ranks nearest
        // White, plus b5, c5, f5, g5.
        Bits<V> horde{};
        for (Square r = Bits<V>::ranks() - 4; r < Bits<V>::ranks(); ++r)
            horde |= Bits<V>::rankMask(r * Bits<V>::innerCols());
        for (Square f : {1, 2, 5, 6})
            horde |= Bits<V>::squareToBitboard(
                (Bits<V>::ranks() - 5) * Bits<V>::innerCols() + f);

        pieces[PieceIndex<V>(Pawn)] |= horde;
        sides[White] = horde;

    } else if constexpr (V == Variant::Chaturanga) {
        constexpr std::array back{Rook, Knight, Alfil,  King,
                                  Ferz, Alfil,  Knight, Rook};
        place(back);
        mirror();

    } else if constexpr (V == Variant::Paradigm) {
        constexpr std::array back{Rook, Knight, Dragon, Queen,
                                  King, Dragon, Knight, Rook};
        place(back);
        mirror();

    } else if constexpr (V == Variant::XXL) {
        constexpr std::array back{Rook,       Knight, Bishop, Archbishop, Camel,
                                  General,    Amazon, King,   General,    Camel,
                                  Chancellor, Bishop, Knight, Rook};
        place(back);
        mirror();

    } else if constexpr (V == Variant::Gothic) {
        constexpr std::array back{Rook, Knight,     Bishop, Queen,  Chancellor,
                                  King, Archbishop, Bishop, Knight, Rook};
        place(back);
        mirror();
    }

    if constexpr (Ruleset<V>::Castling) { // note Horde only has kq, not KQkq,
                                          // but white has no king
        castles.castleRights[0] = 0b1111;
        castles.kingRookFrom = {Bits<V>::cols() - 1,
                                Castles<V>::whiteBack + Bits<V>::cols() - 1};
        castles.queenRookFrom = {0, Castles<V>::whiteBack};
    }

    toMove = White;

    beginZobrist();
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::print() {
    // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::beginZobrist() {
    Hash h = 0;

    for (std::size_t t = 0; t < Ruleset<V>::types; ++t) {
        for (Bits<V> b = pieces[t] & sides[Black]; !b.empty();)
            h ^= Zobrist::piece<V>(Black, t, b.popLeastSquare());
        for (Bits<V> b = pieces[t] & sides[White]; !b.empty();)
            h ^= Zobrist::piece<V>(White, t, b.popLeastSquare());
    }

    h ^= Zobrist::turn() * toMove;

    if constexpr (Ruleset<V>::Castling)
        h ^= Zobrist::castling(castles.castleRights[0]);

    if constexpr (Ruleset<V>::EnPassant) {
        if (enPassant[0] != Bits<V>::noSquare())
            h ^= Zobrist::enPassant(enPassant[0] % Bits<V>::innerCols());
    }

    hashes[0] = h;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
int Position<V>::repetitions(int) const {
    return 0; // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::makeMove(const Move<V> &m) {
    // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::unmakeMove() {
    // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::passMove() {
    // TODO
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::unpassMove() {
    // TODO
}

template class Position<Variant::Chess>;
template class Position<Variant::Antichess>;
template class Position<Variant::Horde>;
template class Position<Variant::Chaturanga>;
template class Position<Variant::Paradigm>;
template class Position<Variant::XXL>;
template class Position<Variant::Gothic>;

} // namespace Tilted
