#include <deque>
#include <iostream>
using namespace std;

int main() {
  deque<int> coll; // deque container for integers

  for (int i = 0; i < 3; ++i) {
    coll.push_front(i);
  }
  for (int i = 0; i < 3; ++i) {
    coll.push_back(i + 100);
  }

  for (unsigned i = 0; i < coll.size(); ++i) {
    cout << coll[i] << ' ';
  }
  cout << endl;

  return 0;
}
