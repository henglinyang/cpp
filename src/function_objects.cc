#include <algorithm>
#include <iostream>
#include <list>

#include "container_printer.h"

template <int increment> void AddInt(int &element) { element += increment; }

class Multiply {
private:
  int multiplicant;

  Multiply() {}

public:
  Multiply(int v) : multiplicant(v) {}

  // function call
  void operator()(int &element) const { element *= multiplicant; }
};

int main() {
  std::list<int> coll;

  for (int i = 1; i < 5; ++i)
    coll.push_back(i);
  print_container("original list:", coll);
  for_each(coll.begin(), coll.end(), AddInt<10>);
  print_container("AddInt<10> list:", coll);

  for_each(coll.begin(), coll.end(), Multiply(2));
  print_container("Multiply(2) list:", coll);

  return 0;
}
