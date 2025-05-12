#include <iostream>

template <class T>
void print_container(const std::string &&prefix, const T coll) {
  std::cout << prefix << '(' << coll.size() << '/' << coll.max_size() << "):";
  for (typename T::const_iterator it = coll.begin(); it != coll.end(); ++it)
    std::cout << ' ' << *it;
  std::cout << std::endl;
}

template <class T> void print_map(const std::string &&prefix, const T coll) {
  std::cout << prefix << '(' << coll.size() << '/' << coll.max_size() << "):";
  for (auto &it : coll)
    std::cout << ' ' << it.first << "->" << it.second;
  std::cout << std::endl;
}
