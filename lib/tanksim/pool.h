#pragma once
// Fixed-capacity pool: no heap allocation, ever (DESIGN §10). When full, add() drops the item.

namespace tank {

template <class T, int N>
struct Pool {
  static constexpr int kCapacity = N;
  T items[N]{};
  int count = 0;

  bool add(const T& v) {
    if (count >= N) return false;
    items[count++] = v;
    return true;
  }
  void removeAt(int i) {
    for (int k = i + 1; k < count; ++k) items[k - 1] = items[k];
    --count;
  }
  // Keeps order; O(n).
  template <class Pred>
  void removeIf(Pred pred) {
    int w = 0;
    for (int r = 0; r < count; ++r)
      if (!pred(items[r])) items[w++] = items[r];
    count = w;
  }
  T* begin() { return items; }
  T* end() { return items + count; }
  const T* begin() const { return items; }
  const T* end() const { return items + count; }
};

}  // namespace tank
