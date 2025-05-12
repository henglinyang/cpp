// C++ program to demonstrate working strchr()
#include <cstring>
#include <iostream>
using namespace std;

int main() {
  char str[] = "This is a string";
  char c = 's';
  char *rch = strrchr(str, c);
  char *lch = strchr(str, c);
  cout << "Last position of '" << c << "' in char[]: \"" << str << "\" is "
       << rch - str + 1 << ", and the first position is " << lch - str + 1
       << endl
       << endl;
  return 0;
}
