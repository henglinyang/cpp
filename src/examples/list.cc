#include <algorithm>
#include <iostream>
#include <list>

#include "container_printer.h"

int main() {
  struct less_a {
    bool operator()(const char &value) { return value < 'a'; }
  };

  std::list<char> coll; // list container for chars
  std::list<char> coll2;

  coll.push_back('a');
  coll.push_back('b');
  coll.push_back('c');
  print_container<std::list<char>>("coll", coll);
  std::cout << "coll, front: " << coll.front() << ", back: " << coll.back()
            << std::endl;

  coll.push_front('z');
  coll.push_front('y');
  print_container<std::list<char>>("push_front coll", coll);

  coll.pop_back();
  print_container<std::list<char>>("pop_back, coll", coll);

  coll.pop_front();
  print_container<std::list<char>>("pop_front, coll", coll);

  coll.remove('z');
  print_container<std::list<char>>("remove('z'), coll", coll);

  for (char c = '1'; c < '4'; ++c)
    coll2.push_back(c);
  print_container<std::list<char>>("coll2", coll2);
  // begin and end of coll2 are optional
  coll.splice(coll.end(), coll2, ++coll2.begin(), coll2.end());
  print_container<std::list<char>>("splice, coll", coll);
  print_container<std::list<char>>("splice, coll2", coll2);

  coll.remove_if(less_a());
  print_container<std::list<char>>("remove_if, coll", coll);

  auto pos = find(coll.begin(), coll.end(), 'b');
  coll.insert(pos, '0');
  print_container<std::list<char>>("find('b'), insert('0'), coll", coll);

  coll.swap(coll2);
  print_container<std::list<char>>("swap, coll", coll);
  print_container<std::list<char>>("swap, coll2", coll2);

  return 0;
}
