export module Property;

import std;
export import Variants;
import Attacks;

export namespace Tilted {

// How a variant is searched rather than how it plays. One primary template, not
// one per variant: engine choices derived from the rules, bar a few exceptions.
template <Variant V> struct Property {
    // Legal generation needs attacks that reverse, which a hobbled leg and a
    // hurdle do not; a duck blocks the attack after the king steps into it.
    static constexpr bool LegalBulk =
        Ruleset<V>::Royal < 0 ||
        (!hasAny<V>(Attacks::HorseLike) &&
         !hasAny<V>(Attacks::GrasshopperLike) && V != Variant::Duck);
};

} // namespace Tilted
