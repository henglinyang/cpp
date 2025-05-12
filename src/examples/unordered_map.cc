#include <unordered_map>

#include "container_printer.h"

int main() {
  std::unordered_map<std::string, int> mymap;

  mymap.insert({"zero", 0});
  mymap["two"] = 2;
  mymap["fifteen"] = 15;
  mymap["one"] = 1;
  print_map("map:", mymap);
  std::cout << "mymap[\"one\"]: " << mymap["one"]
            << ", count(\"one\"):" << mymap.count("test")
            << ", count(\"test\"):" << mymap.count("one") << std::endl;

  mymap.erase("test");
  print_map("erase(\"test\"):", mymap);

  mymap.erase("one");
  print_map("erase(\"one\"):", mymap);

  mymap.clear();
  print_map("clear:", mymap);

  mymap = {{"one", 1}, {"two", 2}, {"three", 3}};
  print_map("assign:", mymap);

  return 0;
}
