// Implementation unit for Position: out-of-line member definitions plus
// the explicit instantiations, so function bodies stay out of the BMI and editing
// them doesn't force importers to rebuild.
module Position;

import std;
import Attacks; // Bitboard members aren't re-exported by the interface

namespace Tilted {

// Dense PieceMapping index of whatever stands on `s`, or -1 when it is empty --
// matching PieceIndex's convention for an absent piece.
template <Variant V>
    requires(Ruleset<V>::Supported)
int Position<V>::pieceAt(const Square &s) const {
    for (std::size_t t = 0; t < Ruleset<V>::types; ++t)
        if (pieces[t].test(s))
            return static_cast<int>(t);
    return -1;
}

// Mirrors Ruleset<V>::Supported. Members whose constraints fail for a variant are
// skipped rather than errors, and a variant missing here is a link error naming
// it, not a silent gap.
template class Position<Variant::Chess>;
template class Position<Variant::Antichess>;
template class Position<Variant::ThreeCheck>;
template class Position<Variant::Horde>;
template class Position<Variant::KingOfTheHill>;
template class Position<Variant::RacingKings>;
template class Position<Variant::Chaturanga>;
template class Position<Variant::Paradigm>;
template class Position<Variant::MiniForest>;
template class Position<Variant::Petrified>;
template class Position<Variant::Duck>;

} // namespace Tilted
