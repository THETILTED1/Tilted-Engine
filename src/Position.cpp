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

    bool frc = false;
    if constexpr (V == Variant::Chess)
        frc = isFRC;

    // KQkq, or each rook's own file under Shredder. A right no rook was named
    // for keeps a placeholder, which no reachable mask can print.
    std::array<char, 4> parts{'Y', 'Z', 'y', 'z'};
    if (!frc)
        parts = {'K', 'Q', 'k', 'q'};

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

            const std::size_t slot = 2 * (White - c) + !kingside;
            const std::uint8_t bit = 1 << slot;

            rightsChange[king] |= bit;
            rightsChange[rook] |= bit;
            castleRights[0] |= bit;

            if (frc) {
                const char file =
                    static_cast<char>('A' + rook % Bits<V>::innerCols());
                parts[slot] =
                    c == White ? file : static_cast<char>(file + ('a' - 'A'));
            }
        }

    castleStrings[0] = "-";
    for (std::size_t mask = 1; mask < castleStrings.size(); ++mask) {
        castleStrings[mask].clear();
        for (std::size_t slot = 0; slot < parts.size(); ++slot)
            if (mask & (1u << slot))
                castleStrings[mask] += parts[slot];
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
    requires(Ruleset<V>::Insufficient != 0)
{
    const Bits<V> royals = pieces[Ruleset<V>::Royal];

    constexpr auto minors = std::to_array({Piece::Knight, Piece::Bishop});
    const Bits<V> minor = anyOf(minors);

    // Rule 2 spares nothing: a bared king loses at Shatranj and a check wins at
    // three-check, so KN and KB keep their winning chances there.
    constexpr std::size_t spare = Ruleset<V>::Insufficient == 1;

    return (occupied() & ~royals & ~minor).empty() && minor.count() <= spare;
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
                if constexpr (hasAny<V>(shape))
                    attackers |= Attacks::PieceAttacks<shape[0]>(s, occ) &
                                 anyOf(shape) & army;
            }(),
            ...);
    }(std::make_index_sequence<Attacks::Shapes.size()>{});

    if constexpr (hasAny<V>(Attacks::PawnLike))
        attackers |= Attacks::PawnAttacks<M, N>(static_cast<Color>(!c), s) &
                     anyOf(Attacks::PawnLike) & army;

    if constexpr (hasAny<V>(Attacks::HorseLike))
        for (Bits<V> b = anyOf(Attacks::HorseLike) & army &
                         Attacks::KnightAttacks<M, N>(s);
             !b.empty();) {
            const Square from = b.popLeastSquare();
            if (Attacks::HorseAttacks(from, occ).test(s))
                attackers |= Bits<V>::squareToBitboard(from);
        }

    if constexpr (hasAny<V>(Attacks::GrasshopperLike))
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

    // A side holding no royal cannot be checked -- Horde's White is a kingless
    // pawn swarm.
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
void Position<V>::setStartPos() {
    std::string fen(Ruleset<V>::StartFen);

    // Shredder names the rooks by file, and on the standard rank the outermost
    // pair is the corner pair, so KQkq spells HAha.
    if constexpr (V == Variant::Chess)
        if (castles.isFRC)
            fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w HAha - 0 1";

    readFen(fen);
}

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::readFen(std::string fen) {
    empty();

    std::istringstream segments(fen);
    std::string field;
    segments >> field;

    // FEN reads rank 8 first and so does this numbering: the walk runs forward.
    Square rank = 0, file = 0;
    for (std::size_t i = 0; i < field.size(); ++i) {
        const char c = field[i];

        if (c == '/') {
            ++rank;
            file = 0;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            // A run reaches two digits once a board is ten or more wide.
            std::size_t run = 0;
            while (i < field.size() &&
                   std::isdigit(static_cast<unsigned char>(field[i])))
                run = run * 10 + static_cast<std::size_t>(field[i++] - '0');
            --i;
            file += run;
            continue;
        }

        // PieceChars is ordered by the Piece enum, so its index is the piece
        // and the case is the color.
        const int type = PieceIndex<V>(static_cast<Piece>(
            PieceChars.find(static_cast<char>(std::toupper(c)))));
        const Color side =
            std::isupper(static_cast<unsigned char>(c)) ? White : Black;
        const Square s = rank * Bits<V>::innerCols() + file++;

        pieces[type].toggle(s);
        sides[side].toggle(s);
    }

    segments >> field;
    toMove = field[0] == 'w' ? White : Black;

    segments >> field;
    if constexpr (Ruleset<V>::Castling) {
        bool frc = false;
        if constexpr (V == Variant::Chess)
            frc = castles.isFRC;

        std::array<Square, 2> kings, kingRooks, queenRooks;
        kings.fill(Bits<V>::noSquare());
        kingRooks = queenRooks = kings;

        for (const Color c : {Black, White}) {
            const Bits<V> royal = those(c, Ruleset<V>::Royal);
            if (!royal.empty()) // Horde's White has no king to castle
                kings[c] = royal.leastSquare();
        }

        // Shredder-FEN names each rook's file and never spells KQkq, so the two
        // forms cannot collide.
        if (frc)
            for (const char letter : field) {
                const std::size_t file =
                    static_cast<std::size_t>(std::tolower(letter) - 'a');
                if (file >= Bits<V>::cols())
                    continue;

                const Color c = std::isupper(static_cast<unsigned char>(letter))
                                    ? White
                                    : Black;
                const Square s =
                    (c == White ? Castles<V>::whiteBack : 0) + file;
                (s > kings[c] ? kingRooks : queenRooks)[c] = s;
            }
        else
            for (const Color c : {Black, White}) {
                const Square back = c == White ? Castles<V>::whiteBack : 0;

                if (field.find(c == White ? 'K' : 'k') != std::string::npos)
                    kingRooks[c] = back + Bits<V>::cols() - 1;
                if (field.find(c == White ? 'Q' : 'q') != std::string::npos)
                    queenRooks[c] = back;
            }

        castles.arrangeCastling(kings, kingRooks, queenRooks);
    }

    segments >> field;
    if constexpr (Ruleset<V>::EnPassant) {
        enPassant[0] = Bits<V>::noSquare();
        if (field != "-") {
            const Square rank = Bits<V>::ranks() - std::stoul(field.substr(1));
            enPassant[0] = rank * Bits<V>::innerCols() + (field[0] - 'a');
        }
    }

    // Short FENs stop here; empty() already zeroed the clock.
    if (segments >> field)
        halfMoves[0] = std::stoi(field);

    beginZobrist();
}

template <Variant V>
    requires(Ruleset<V>::Supported)
std::string Position<V>::makeFen() const {
    std::string fen;

    for (Square r = 0; r < Bits<V>::ranks(); ++r) {
        std::size_t empties = 0;

        for (Square f = 0; f < Bits<V>::cols(); ++f) {
            const Square s = r * Bits<V>::innerCols() + f;
            const int type = pieceAt(s);

            if (type < 0) {
                ++empties;
                continue;
            }

            if (empties) { // a run reaches two digits past nine files
                fen += std::to_string(empties);
                empties = 0;
            }

            const char glyph =
                PieceChars[static_cast<std::size_t>(Ruleset<V>::pieces[type])];
            fen += sides[White].test(s)
                       ? glyph
                       : static_cast<char>(glyph + ('a' - 'A'));
        }

        if (empties)
            fen += std::to_string(empties);
        if (r + 1 < Bits<V>::ranks())
            fen += '/';
    }

    fen += toMove ? " w " : " b ";

    if constexpr (Ruleset<V>::Castling)
        fen += castles.castleStrings[castles.castleRights[clock]];
    else
        fen += '-';

    fen += ' ';

    if constexpr (Ruleset<V>::EnPassant)
        fen += enPassant[clock] == Bits<V>::noSquare()
                   ? std::string("-")
                   : Move<V>::algebraic(enPassant[clock]);
    else
        fen += '-';

    // The full move number is not tracked, so it reads as a placeholder.
    return fen + ' ' + std::to_string(sinceReset()) + " 1023";
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

template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::forget() {
    hashes[0] = thisHash();
    halfMoves[0] = sinceReset();
    plays[0] = Move<V>::null(); // nothing left to unmake, as after a readFen

    if constexpr (Ruleset<V>::EnPassant)
        enPassant[0] = enPassant[clock];
    if constexpr (Ruleset<V>::Castling)
        castles.castleRights[0] = castles.castleRights[clock];

    clock = 0;
}

// Every field this writes is indexed by the new clock, which is what lets
// unmakeMove restore state by rewinding it.
template <Variant V>
    requires(Ruleset<V>::Supported)
void Position<V>::makeMove(const Move<V> &m) {
    const Square start = m.from(), end = m.to();
    const std::size_t moving = m.moving(), ending = m.ending();
    const Color enemy = static_cast<Color>(!toMove);

    // Index grows toward White's home rank, so a mover's own side is +stride.
    const std::size_t stride = Bits<V>::innerCols();

    ++clock;

    plays[clock] = m;
    hashes[clock] = hashes[clock - 1] ^ Zobrist::turn();

    // Ask capturing(), never victim(): these type indices are dense, so 0 is a
    // real piece (the pawn, mostly) and cannot double as "captured nothing".
    if (m.capturing()) {
        // An en-passant victim stands beside the pawn, not under it.
        Square step = 0;
        if constexpr (Ruleset<V>::EnPassant)
            step = static_cast<Square>(m.enPassant()) * stride;

        const Square target = end - step + 2 * step * toMove;

        pieces[m.victim()].toggle(target);
        sides[enemy].toggle(target);
        hashes[clock] ^= Zobrist::piece<V>(enemy, m.victim(), target);
    }

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
