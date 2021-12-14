// C++ code to demonstrate the working of
// push_heap() and pop_heap()
#include <bits/stdc++.h>

#include "container_printer.h"

using namespace std;
int main() {

  // Initializing a vector
  vector<int> v1 = {20, 30, 40, 25, 15};

  print_container("vector:", v1);
  is_heap(v1.begin(), v1.end()) ? cout << "The container is a heap."
                                : cout << "The container is not a heap.";
  cout << endl;

  // Converting vector into a heap
  // using make_heap()
  make_heap(v1.begin(), v1.end());
  print_container("make_heap:", v1);

  // Displaying the maximum element of heap
  // using front()
  cout << "The maximum element of heap is : ";
  cout << v1.front() << endl;

  // using push_back() to enter element
  // in vector
  v1.push_back(50);
  print_container("push_back(50):", v1);

  // using push_heap() to reorder elements
  push_heap(v1.begin(), v1.end());
  print_container("push_heap:", v1);

  // Displaying the maximum element of heap
  // using front()
  cout << "The maximum element of heap after push is : ";
  cout << v1.front() << endl;

  // using pop_heap() to delete maximum element
  pop_heap(v1.begin(), v1.end());
  print_container("pop_heap:", v1);
  v1.pop_back();
  print_container("pop_back:", v1);

  // Displaying the maximum element of heap
  // using front()
  cout << "The maximum element of heap after pop is : ";
  cout << v1.front() << endl;

  return 0;
}
