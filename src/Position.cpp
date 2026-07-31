// Implementation unit for Position: out-of-line member definitions plus
// the explicit instantiations, so function bodies stay out of the BMI and
// editing them doesn't force importers to rebuild.
module Position;

import std;
import Zobrist;

namespace Tilted {

template <Variant V>
void Castles<V>::arrangeCastling(const std::array<Square, 2> &kings,
                                 const std::array<Square, 2> &kingRooks,
                                 const std::array<Square, 2> &queenRooks) {
    kingFrom = kings;
    kingRookFrom = kingRooks;
    queenRookFrom = queenRooks;

    kingSafeMask = {};
    queenSafeMask = {};
    kingOccMask = {};
    queenOccMask = {};
    rightsChange.fill(0);
    castleRights[0] = 0;

    // between() is open, and both ends of a castling run are part of it.
    const auto span = [](Square a, Square b) {
        return Bits<V>::between(a, b) | Bits<V>::squareToBitboard(a) |
               Bits<V>::squareToBitboard(b);
    };

    for (const Color c : {Black, White})
        for (const bool kingside : {true, false}) {
            const Square king = kingFrom[c];
            const Square rook = kingside ? kingRookFrom[c] : queenRookFrom[c];
            if (king == Bits<V>::noSquare() || rook == Bits<V>::noSquare())
                continue;

            const Bits<V> path =
                span(king, kingside ? kingKingTo[c] : kingQueenTo[c]);
            const Bits<V> clear =
                (path | span(rook, kingside ? kingRookTo[c] : queenRookTo[c])) &
                ~(Bits<V>::squareToBitboard(king) |
                  Bits<V>::squareToBitboard(rook));

            (kingside ? kingSafeMask : queenSafeMask)[c] = path;
            (kingside ? kingOccMask : queenOccMask)[c] = clear;

            const std::uint8_t bit = 1 << (2 * (White - c) + !kingside);
            rightsChange[king] |= bit;
            rightsChange[rook] |= bit;
            castleRights[0] |= bit;
        }
}

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
    const Bits<V> royals = pieces[Ruleset<V>::Royal];

    // Shatranj bares the king and 3check wins on checks, so KN and KB still win
    // there: only two bare kings draw.
    if constexpr (Ruleset<V>::oneOf({Variant::Chaturanga, Variant::ThreeCheck}))
        return (occupied() & ~royals).empty();

    constexpr auto minors = std::to_array({Piece::Knight, Piece::Bishop});
    const Bits<V> minor = anyOf(minors);

    return (occupied() & ~royals & ~minor).empty() && minor.count() <= 1;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
Bits<V> Position<V>::isAttacked(const Square &s, const Color &c) const {
    constexpr std::size_t M = Ruleset<V>::dims.ranks, N = Ruleset<V>::dims.cols;
    const Bits<V> occ = occupied(), army = sides[c];
    Bits<V> attackers{};

    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (
            [&] {
                constexpr auto shape = Attacks::Shapes[I];
                if constexpr (Ruleset<V>::has(shape))
                    attackers |= Attacks::PieceAttacks<shape[0]>(s, occ) &
                                 anyOf(shape) & army;
            }(),
            ...);
    }(std::make_index_sequence<Attacks::Shapes.size()>{});

    if constexpr (Ruleset<V>::has(Attacks::PawnLike))
        attackers |= Attacks::PawnAttacks<M, N>(static_cast<Color>(!c), s) &
                     anyOf(Attacks::PawnLike) & army;

    if constexpr (Ruleset<V>::has(Attacks::HorseLike))
        for (Bits<V> b = anyOf(Attacks::HorseLike) & army &
                         Attacks::KnightAttacks<M, N>(s);
             !b.empty();) {
            const Square from = b.popLeastSquare();
            if (Attacks::HorseAttacks(from, occ).test(s))
                attackers |= Bits<V>::squareToBitboard(from);
        }

    if constexpr (Ruleset<V>::has(Attacks::GrasshopperLike))
        for (Bits<V> b = anyOf(Attacks::GrasshopperLike) & army; !b.empty();) {
            const Square from = b.popLeastSquare();
            if (Attacks::GrasshopperAttacks(from, occ).test(s))
                attackers |= Bits<V>::squareToBitboard(from);
        }

    return attackers;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
Bits<V> Position<V>::isChecked(const Color &c) const
    requires(Ruleset<V>::Royal >= 0)
{
    const Bits<V> royal = those(c, Ruleset<V>::Royal);

    // Horde's White is a kingless pawn swarm, so its royal square is empty.
    if constexpr (V == Variant::Horde)
        if (royal.empty())
            return {};

    return isAttacked(royal.leastSquare(), static_cast<Color>(!c));
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

    if constexpr (V == Variant::Chess) {
        const bool frc = castles.isFRC;
        castles = {};
        castles.isFRC = frc;
    } else
        castles = {};

    // beginZobrist();
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

    if constexpr (Ruleset<V>::Castling) {
        static_assert(Ruleset<V>::Royal >= 0, "castling needs a royal king");

        std::array<Square, 2> kings, kingRooks, queenRooks;
        for (const Color c : {Black, White}) {
            const Bits<V> royal = those(c, Ruleset<V>::Royal);
            const Square back = c == White ? Castles<V>::whiteBack : 0;

            kings[c] =
                royal.empty() ? Bits<V>::noSquare() : royal.leastSquare();
            kingRooks[c] = royal.empty() ? Bits<V>::noSquare()
                                         : back + Bits<V>::cols() - 1;
            queenRooks[c] = royal.empty() ? Bits<V>::noSquare() : back;
        }

        castles.arrangeCastling(kings, kingRooks, queenRooks);
    }

    toMove = White;

    beginZobrist();
}

template <Variant V>
    requires(Ruleset<V>::Supported)
std::string Position<V>::moveUCI(const Move<V> &m) const {
    Square to = m.to();

    // 960 GUIs read a castle as king-takes-rook: on a shuffled rank the king's
    // own destination is ambiguous with a one-step king move.
    if constexpr (V == Variant::Chess)
        if (m.castling() && castles.isFRC) {
            const Color side = to >= Castles<V>::whiteBack ? White : Black;
            to = to == Castles<V>::kingKingTo[side]
                     ? castles.kingRookFrom[side]
                     : castles.queenRookFrom[side];
        }

    std::string uci = Move<V>::algebraic(m.from()) + Move<V>::algebraic(to);

    if (m.ending() != m.moving())
        uci += static_cast<char>(PieceChars[static_cast<std::size_t>(
                                     Ruleset<V>::pieces[m.ending()])] +
                                 ('a' - 'A'));

    return uci;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::print() {
    const std::array<std::string, 2> sideNames{"Black", "White"};
    printBoards(std::cout, sides, sideNames);

    std::array<std::string, Ruleset<V>::types> typeNames;
    for (std::size_t t = 0; t < Ruleset<V>::types; ++t)
        typeNames[t] =
            PieceNames[static_cast<std::size_t>(Ruleset<V>::pieces[t])];
    printBoards(std::cout, pieces, typeNames);

    const Move<V> last = lastPlayed();
    std::cout << "toMove     " << sideNames[toMove] << '\n'
              << "lastPlayed " << (last.data ? moveUCI(last) : "-") << '\n';

    if constexpr (Ruleset<V>::Castling)
        std::cout << "castling   "
                  << castles.castleStrings[castles.castleRights[clock]] << '\n';

    if constexpr (Ruleset<V>::EnPassant)
        std::cout << "enPassant  "
                  << (enPassant[clock] == Bits<V>::noSquare()
                          ? std::string("-")
                          : Move<V>::algebraic(enPassant[clock]))
                  << '\n';

    std::cout << "zobrist    " << std::hex << thisHash() << std::dec << '\n';
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
int Position<V>::repetitions(int ply) const {
    const int reach = std::min(sinceReset(), clock);

    int reps = 1; // this occurrence
    for (int back = 2; back <= reach && reps < 3; back += 2)
        reps += (hashes[clock - back] == thisHash()) * (1 + (back < ply));

    return reps;
}

// Applies `m` to the board while folding the identical change into a fresh
// hash, so key and position never drift. Every field it writes is indexed by
// the new clock, which is what lets unmakeMove restore state by rewinding.
template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::makeMove(const Move<V> &m) {
    const Square start = m.from(), end = m.to();
    const std::size_t moving = m.moving(), ending = m.ending();
    const Color enemy = static_cast<Color>(!toMove);

    // A rank step. Index grows toward White's home rank, so a mover's own side
    // is +stride: rear = end - stride + 2 * stride * toMove, no branch needed.
    const std::size_t stride = Bits<V>::innerCols();

    ++clock;

    plays[clock] = m;
    hashes[clock] = hashes[clock - 1] ^ Zobrist::turn();

    // Ask capturing(), never victim(): these type indices are dense, so 0 is a
    // real piece (the pawn, mostly) and cannot double as "captured nothing".
    if (m.capturing()) {
        // An en-passant victim stands beside the pawn, not under it -- one rank
        // behind where it lands. An ordinary capture takes no step.
        Square step = 0;
        if constexpr (Ruleset<V>::EnPassant)
            step = static_cast<Square>(m.enPassant()) * stride;

        const Square target = end - step + 2 * step * toMove;

        pieces[m.victim()].toggle(target);
        sides[enemy].toggle(target);
        hashes[clock] ^= Zobrist::piece<V>(enemy, m.victim(), target);
    }

    // moving and ending differ on a promotion, so the two ends of the move come
    // off and go onto separate piece boards.
    sides[toMove].toggle(start);
    sides[toMove].toggle(end);
    pieces[moving].toggle(start);
    pieces[ending].toggle(end);

    hashes[clock] ^= Zobrist::piece<V>(toMove, moving, start) ^
                     Zobrist::piece<V>(toMove, ending, end);

    if constexpr (Ruleset<V>::EnPassant) {
        // Whatever right the previous ply granted expires now, taken or not.
        if (const Square last = enPassant[clock - 1];
            last != Bits<V>::noSquare())
            hashes[clock] ^= Zobrist::enPassant(last % stride);

        // A double push grants the square it crossed, on the pawn's own file.
        enPassant[clock] = Bits<V>::noSquare();
        if (m.doublePush()) {
            enPassant[clock] = end - stride + 2 * stride * toMove;
            hashes[clock] ^= Zobrist::enPassant(end % stride);
        }
    }

    bool reset = m.capturing();
    if constexpr (PieceIndex<V>(Piece::Pawn) >= 0)
        reset |= moving == static_cast<std::size_t>(PieceIndex<V>(Piece::Pawn));
    halfMoves[clock] = reset ? 0 : halfMoves[clock - 1] + 1;

    if constexpr (Ruleset<V>::Castling) {
        static_assert(PieceIndex<V>(Piece::Rook) >= 0, "castling needs a rook");
        constexpr std::size_t rook = PieceIndex<V>(Piece::Rook);

        std::uint8_t &rights = castles.castleRights[clock];
        rights = castles.castleRights[clock - 1];

        if (const std::uint8_t change =
                castles.rightsChange[start] | castles.rightsChange[end];
            change & rights) {
            hashes[clock] ^= Zobrist::castling(rights);
            rights &= ~change;
            hashes[clock] ^= Zobrist::castling(rights);
        }

        if (m.castling()) {
            const bool kingside = end == Castles<V>::kingKingTo[toMove];
            const Square rookFrom = kingside ? castles.kingRookFrom[toMove]
                                             : castles.queenRookFrom[toMove];
            const Square rookTo = kingside ? Castles<V>::kingRookTo[toMove]
                                           : Castles<V>::queenRookTo[toMove];

            sides[toMove].toggle(rookFrom);
            sides[toMove].toggle(rookTo);
            pieces[rook].toggle(rookFrom);
            pieces[rook].toggle(rookTo);

            hashes[clock] ^= Zobrist::piece<V>(toMove, rook, rookFrom) ^
                             Zobrist::piece<V>(toMove, rook, rookTo);
        }
    }

    toMove = enemy;
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::unmakeMove() {
    const Move<V> m = lastPlayed();

    const Square start = m.from(), end = m.to();
    const std::size_t moving = m.moving(), ending = m.ending();
    const std::size_t stride = Bits<V>::innerCols();

    toMove = static_cast<Color>(!toMove);
    const Color enemy = static_cast<Color>(!toMove);

    sides[toMove].toggle(start);
    sides[toMove].toggle(end);
    pieces[moving].toggle(start);
    pieces[ending].toggle(end);

    if (m.capturing()) {
        Square step = 0;
        if constexpr (Ruleset<V>::EnPassant)
            step = static_cast<Square>(m.enPassant()) * stride;

        const Square target = end - step + 2 * step * toMove;

        pieces[m.victim()].toggle(target);
        sides[enemy].toggle(target);
    }

    if constexpr (Ruleset<V>::Castling)
        if (m.castling()) {
            constexpr std::size_t rook = PieceIndex<V>(Piece::Rook);

            const bool kingside = end == Castles<V>::kingKingTo[toMove];
            const Square rookFrom = kingside ? castles.kingRookFrom[toMove]
                                             : castles.queenRookFrom[toMove];
            const Square rookTo = kingside ? Castles<V>::kingRookTo[toMove]
                                           : Castles<V>::queenRookTo[toMove];

            sides[toMove].toggle(rookFrom);
            sides[toMove].toggle(rookTo);
            pieces[rook].toggle(rookFrom);
            pieces[rook].toggle(rookTo);
        }

    --clock;
}

// A null move: the turn changes hands and the board is untouched. Passing still
// forfeits the en-passant right, so that key leaves the hash with it.
template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::passMove() {
    ++clock;

    plays[clock] = Move<V>::null();
    hashes[clock] = hashes[clock - 1] ^ Zobrist::turn();
    halfMoves[clock] = halfMoves[clock - 1] + 1;

    if constexpr (Ruleset<V>::Castling)
        castles.castleRights[clock] = castles.castleRights[clock - 1];

    if constexpr (Ruleset<V>::EnPassant) {
        if (const Square last = enPassant[clock - 1];
            last != Bits<V>::noSquare())
            hashes[clock] ^= Zobrist::enPassant(last % Bits<V>::innerCols());
        enPassant[clock] = Bits<V>::noSquare();
    }

    toMove = static_cast<Color>(!toMove);
}

// Nothing to restore: every field but toMove is indexed by the clock.
template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::unpassMove() {
    toMove = static_cast<Color>(!toMove);
    --clock;
}

// The variants that castle: Castles bodies live here too, so other TUs need
// these symbols emitted.
template struct Castles<Variant::Chess>;
template struct Castles<Variant::Horde>;
template struct Castles<Variant::Paradigm>;
template struct Castles<Variant::XXL>;
template struct Castles<Variant::Gothic>;

template class Position<Variant::Chess>;
template class Position<Variant::Antichess>;
template class Position<Variant::Horde>;
template class Position<Variant::Chaturanga>;
template class Position<Variant::Paradigm>;
template class Position<Variant::XXL>;
template class Position<Variant::Gothic>;

} // namespace Tilted
