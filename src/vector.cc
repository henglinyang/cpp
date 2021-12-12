#include <iostream>
#include <vector>
using namespace std;

void print(vector<int> coll) {
  for (unsigned i = 0; i < coll.size(); ++i) {
    cout << coll[i] << ' ';
  }
  cout << endl;
}

int main() {
  vector<int> coll; // interger vector container

  for (int i = 0; i < 6; ++i) {
    coll.push_back(i);
  }

  print(coll);
  return 0;
}
