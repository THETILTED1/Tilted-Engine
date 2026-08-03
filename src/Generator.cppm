export module Generator;

import std;
export import Position;

export namespace Tilted {

template <Variant V> class MoveList {
  public:
    // Assumption
    static constexpr std::size_t capacity = 4 * Bits<V>::noSquare();

    void push(const Move<V> &m) { moves[filled++] = m; }
    void clear() { filled = 0; }

    std::size_t size() const { return filled; }
    bool empty() const { return filled == 0; }

    Move<V> &operator[](std::size_t i) { return moves[i]; }
    const Move<V> &operator[](std::size_t i) const { return moves[i]; }

  private:
    std::array<Move<V>, capacity> moves;
    std::size_t filled = 0;
};

template <Variant V>
    requires(Ruleset<V>::Supported)
class Generator {};

} // namespace Tilted
