#include <unordered_set>

#include "container_printer.h"

// In general, set<> is slower than unordered_set<>
int main() {
  auto equal = [](unsigned first, unsigned second) { return first == second; };

  std::unordered_set<unsigned> myset;

  for (unsigned i = 16; i > 10; i -= 2) {
    myset.insert(i);
  }
  print_container("original set:", myset);

  myset.insert(14);
  print_container("insert(14):", myset);

  myset.insert(15);
  print_container("insert(15):", myset);

  myset.erase(15);
  print_container("erase(15):", myset);

  std::cout << "count(15): " << myset.count(15)
            << ", count(12): " << myset.count(12) << std::endl;
}
