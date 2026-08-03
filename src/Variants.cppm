export module Variants;

import std;
export import Consts;

namespace Tilted {

// Unqualified enumerators for the declarations below. Deliberately outside the
// export block: importers keep needing Piece::.
using enum Piece;

} // namespace Tilted

export namespace Tilted {

struct Dims {
    std::size_t ranks, cols;
};

// -1 when the list lacks the piece, which is how a variant with no royal ends
// up with Royal = -1.
constexpr int indexOf(std::span<const Piece> ps, Piece p) {
    for (std::size_t i = 0; i < ps.size(); ++i)
        if (ps[i] == p)
            return static_cast<int>(i);
    return -1;
}

// Every type but the pawn and the royal: the orthodox promotion set, so no
// variant keeping it has to spell it out.
template <auto Pieces, Piece RoyalPiece> consteval auto promotable() {
    constexpr std::size_t n = [] {
        std::size_t k = 0;
        for (Piece p : Pieces)
            if (p != Pawn && p != RoyalPiece)
                ++k;
        return k;
    }();

    std::array<Piece, n> out{};
    std::size_t i = 0;
    for (Piece p : Pieces)
        if (p != Pawn && p != RoyalPiece)
            out[i++] = p;
    return out;
}

// Presets a declaration starts from and overrides piecemeal. Variants derive
// from this alone, never from each other, so none inherits another's rules.
template <auto Pieces, Piece RoyalPiece = King> struct Orthodox {
    // Off the parameters, so redeclaring `pieces` in a body would leave the
    // three below it stale.
    static constexpr auto pieces = Pieces;
    static constexpr std::size_t types = pieces.size();
    static constexpr int Royal = indexOf(pieces, RoyalPiece);
    static constexpr auto promotions = promotable<Pieces, RoyalPiece>();

    static_assert(types <= MAX_TYPES);

    static constexpr std::string_view Name = "";
    static constexpr std::string_view StartFen = ""; // supported variants only

    static constexpr Dims dims{8, 8};

    // The side left with no legal move: -1 loses, 0 draws, 1 wins.
    static constexpr int Stalemate = 0;

    static constexpr int Checky = 0; // checks that win; 0 disables

    // Insufficient-material draw: 0 none, 1 also draws a lone minor, 2 draws
    // only two bare royals, so a bared king loses and a check still wins.
    static constexpr int Insufficient = 0;

    static constexpr bool AutoPromote = false;
    static constexpr bool Nonrectangle = false;
    static constexpr bool Regicide = false;
    static constexpr bool Compulsory = false;
    static constexpr bool Pocket = false;
    static constexpr bool Petrified = false;
    static constexpr bool Points = false;
    static constexpr bool Hill = false;

    static constexpr bool EnPassant = true;
    static constexpr bool Castling = true;

    static constexpr bool Supported = false;
};

// Literals more than one variant happens to want. Shared values, not shared
// rules: a variant still states that it uses them.
inline constexpr auto ChessMen =
    std::to_array({Pawn, Knight, King, Bishop, Rook, Queen});
inline constexpr auto CapablancaMen = std::to_array(
    {Pawn, Knight, King, Bishop, Rook, Queen, Archbishop, Chancellor});
inline constexpr std::string_view StandardFen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// One specialization per variant, and every Variant needs one: Zobrist sizes
// its tables over the whole enum.
// Supported: Chess, Antichess, Horde, Chaturanga, Paradigm, XXL, Gothic.
template <Variant V> struct Ruleset;

template <> struct Ruleset<Variant::Chess> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "chess";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr int Insufficient = 1;
    static constexpr bool Supported = true;
};

template <> struct Ruleset<Variant::Atomic> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "atomic";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr int Insufficient = 1;
};

// No royal, so promotable() offers the king as an ordinary man.
template <>
struct Ruleset<Variant::Antichess> : Orthodox<ChessMen, Piece::None> {
    static constexpr std::string_view Name = "antichess";
    static constexpr std::string_view StartFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1";
    static constexpr int Stalemate = 1;
    static constexpr bool Compulsory = true;
    static constexpr bool Castling = false;
    static constexpr bool Supported = true;
};

template <> struct Ruleset<Variant::ThreeCheck> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "3check";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr int Checky = 3;
    static constexpr int Insufficient = 2;
};

// White is a kingless pawn swarm, so only Black holds castling rights.
template <> struct Ruleset<Variant::Horde> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "horde";
    static constexpr std::string_view StartFen =
        "rnbqkbnr/pppppppp/8/1PP2PP1/PPPPPPPP/PPPPPPPP/PPPPPPPP/PPPPPPPP w kq "
        "- 0 1";
    static constexpr bool Supported = true;
};

template <> struct Ruleset<Variant::KingOfTheHill> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "kingofthehill";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr bool Hill = true;
};

// Fields no pawn, so nothing promotes.
template <>
struct Ruleset<Variant::RacingKings>
    : Orthodox<std::to_array({Knight, King, Bishop, Rook, Queen})> {
    static constexpr std::string_view Name = "racingkings";
    static constexpr std::array<Piece, 0> promotions{};
    static constexpr bool Hill = true;
    static constexpr bool EnPassant = false;
    static constexpr bool Castling = false;
};

// Shatranj under its older name. PieceChars spells the alfil 'I' and the ferz
// 'F', so its FEN reads oddly.
template <>
struct Ruleset<Variant::Chaturanga>
    : Orthodox<std::to_array({Pawn, Knight, King, Rook, Alfil, Ferz})> {
    static constexpr std::string_view Name = "chaturanga";
    static constexpr std::string_view StartFen =
        "rnikfinr/pppppppp/8/8/8/8/PPPPPPPP/RNIKFINR w - - 0 1";
    static constexpr auto promotions = std::to_array({Ferz});
    static constexpr int Stalemate = -1;
    static constexpr int Insufficient = 2;
    static constexpr bool EnPassant = false;
    static constexpr bool Castling = false;
    static constexpr bool Supported = true;
};

template <>
struct Ruleset<Variant::Paradigm>
    : Orthodox<std::to_array({Pawn, Knight, King, Rook, Queen, Dragon})> {
    static constexpr std::string_view Name = "paradigm";
    static constexpr std::string_view StartFen =
        "rndqkdnr/pppppppp/8/8/8/8/PPPPPPPP/RNDQKDNR w KQkq - 0 1";
    static constexpr int Insufficient = 1;
    static constexpr bool Supported = true;
};

// The ferz is promotion-only: none stands in the start position, but the list
// must field one for a pawn to have something to become.
template <>
struct Ruleset<Variant::MiniForest>
    : Orthodox<std::to_array(
          {Pawn, King, Bishop, Alfil, Dabbaba, Camel, Grasshopper, Ferz})> {
    static constexpr std::string_view Name = "miniforest";
    static constexpr bool Nonrectangle = true;
    static constexpr bool Regicide = true;
    static constexpr bool Points = true;
    static constexpr bool EnPassant = false;
    static constexpr bool Castling = false;
};

template <>
struct Ruleset<Variant::XXL>
    : Orthodox<std::to_array({Pawn, Knight, King, Bishop, Rook, Queen, Camel,
                              General, Archbishop, Chancellor, Amazon})> {
    static constexpr std::string_view Name = "xxl";
    static constexpr std::string_view StartFen =
        "rnbhcmakmcebnr/pppppppppppppp/14/14/14/14/14/14/14/14/14/14/"
        "PPPPPPPPPPPPPP/RNBHCMAKMCEBNR w KQkq - 0 1";
    static constexpr auto promotions = std::to_array({Queen});
    static constexpr Dims dims{14, 14};
    static constexpr bool AutoPromote = true;
    static constexpr bool Supported = true;
};

template <> struct Ruleset<Variant::Gothic> : Orthodox<CapablancaMen> {
    static constexpr std::string_view Name = "gothic";
    static constexpr std::string_view StartFen =
        "rnbqekhbnr/pppppppppp/10/10/10/10/PPPPPPPPPP/RNBQEKHBNR w KQkq - 0 1";
    static constexpr Dims dims{8, 10};
    static constexpr bool Supported = true;
};

// Two King slots: indexOf finds the first, so Royal lands on it and the second
// is an ordinary capturable king.
template <>
struct Ruleset<Variant::BehindTheMirror>
    : Orthodox<std::to_array({Pawn, Knight, King, King, Bishop, Rook, Queen})> {
    static constexpr std::string_view Name = "behindthemirror";
    static constexpr Dims dims{14, 14};
    static constexpr int Checky = 5;
    static constexpr bool Nonrectangle = true;
    static constexpr bool Castling = false;
};

template <> struct Ruleset<Variant::Setup> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "setup";
    static constexpr bool Pocket = true;
    static constexpr bool Castling = false;
};

template <>
struct Ruleset<Variant::Tinyhouse>
    : Orthodox<std::to_array({Pawn, King, Ferz, Wazir})> {
    static constexpr std::string_view Name = "tinyhouse";
    static constexpr Dims dims{4, 4};
    static constexpr int Stalemate = 1;
    static constexpr bool Pocket = true;
    static constexpr bool EnPassant = false;
    static constexpr bool Castling = false;
};

template <> struct Ruleset<Variant::Crazyhouse> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "crazyhouse";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr bool Pocket = true;
};

template <> struct Ruleset<Variant::Seirawan> : Orthodox<CapablancaMen> {
    static constexpr std::string_view Name = "seirawan";
    static constexpr bool Pocket = true;
};

template <> struct Ruleset<Variant::Petrified> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "petrified";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr bool Petrified = true;
    static constexpr bool Points = true;
};

template <> struct Ruleset<Variant::Spell> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "spell";
};

// The grasshopper is royal here, so the variant fields no king at all.
template <>
struct Ruleset<Variant::Jungle>
    : Orthodox<std::to_array({Pawn, Knight, Camel, Grasshopper, Wildebeest,
                              Archbishop, Chancellor}),
               Grasshopper> {
    static constexpr std::string_view Name = "jungle";
    static constexpr auto promotions =
        std::to_array({Wildebeest, Archbishop, Chancellor});
    static constexpr Dims dims{8, 10};
    static constexpr bool Points = true;
    static constexpr bool Castling = false;
};

template <> struct Ruleset<Variant::Duck> : Orthodox<ChessMen> {
    static constexpr std::string_view Name = "duck";
    static constexpr std::string_view StartFen = StandardFen;
    static constexpr bool Regicide = true;
};

template <>
struct Ruleset<Variant::Clobber>
    : Orthodox<std::to_array({Pawn, King, Rook, Wazir})> {
    static constexpr std::string_view Name = "clobber";
    static constexpr Dims dims{6, 8};
    static constexpr bool Nonrectangle = true;
    static constexpr bool Compulsory = true;
    static constexpr bool Hill = true;
    static constexpr bool EnPassant = false;
    static constexpr bool Castling = false;
};

// Nonrectangle because the centre 2x2 is holed out of the 6x6 board.
template <>
struct Ruleset<Variant::Cloister>
    : Orthodox<std::to_array({Wazir}), Piece::None> {
    static constexpr std::string_view Name = "cloister";
    static constexpr std::array<Piece, 0> promotions{};
    static constexpr Dims dims{6, 6};
    static constexpr int Stalemate = -1;
    static constexpr bool Nonrectangle = true;
    static constexpr bool Pocket = true;
    static constexpr bool Petrified = true;
    static constexpr bool EnPassant = false;
    static constexpr bool Castling = false;
};

// Where a piece sits in its variant's list, which is the type index Position
// and Move address it by. -1 when the variant lacks the piece.
template <Variant V> inline constexpr int PieceIndex(Piece p) {
    return indexOf(Ruleset<V>::pieces, p);
}

// Whether the variant fields any of these pieces, so a caller can skip work no
// piece on its board could do.
template <Variant V> inline constexpr bool hasAny(std::span<const Piece> ps) {
    for (Piece p : ps)
        if (PieceIndex<V>(p) >= 0)
            return true;
    return false;
}

} // namespace Tilted
