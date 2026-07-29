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

    pockets = {};
    castles = {};

    if constexpr (Ruleset<V>::EnPassant)
        enPassant.fill(Bits<V>::noSquare());
    points = {};

    hashes = {};
    halfMoves = {};
    plays = {};
    clock = 0;

    bricks = {};
    hill = {};
    wall = {};

    isFRC = {};
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::setStartPos() {
    // TODO
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

    if constexpr (Ruleset<V>::Pocket)
        for (std::size_t t = 0; t < Ruleset<V>::types; ++t) {
            h ^= Zobrist::pocket(Black, t, pockets[Black][t]);
            h ^= Zobrist::pocket(White, t, pockets[White][t]);
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
