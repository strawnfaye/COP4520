# COP4520

A C++ implementation of Lock-Free Resizeable Concurrent Tries, based on the research paper by Aleksandar Prokopec, Phil Bagwell, and Martin Odersky (1). The data structure presented is a modification of Phil Bagwell's structure, Hash Array Mapped Tries.

The paper describes an implementation of a non- blocking concurrent hash trie based on single-word compare-and-swap instructions in a shared memory system. Insert, lookup, and remove operations can be run independently and modify different parts of the hash trie. Remove operations make sure unnecessary memory is freed and that the trie is kept compact.

This implementation is in-progress, and a paper describing its current state can be found in the attached files.



References:

(1) http://chara.epfl.ch/~prokopec/lcpc_ctries.pdf

(2) https://infoscience.epfl.ch/record/64398/files/idealhashtrees.pdf
