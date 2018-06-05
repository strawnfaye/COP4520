Team 17
Faye Strawn
Jenny Belvin
Thomas Boswell

Our implementation of Lock-Free Concurrent Resizable Tries

To compile:
g++ -std=c++11 atomic.cpp -o run

To run:
./run

To test:
modify main() function of atomic.cpp to use std::threads to insert, lookup, or remove a given integer to the trie. We use a debugger to walk through and ensure correctness, but print statements could be used as well.