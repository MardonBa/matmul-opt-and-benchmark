# Work Log

## 10/1

I'm re-learning a lot of C++ things here as I go on, but baseline matmul is done!

## 10/2

Getting robust benchmarking written and setting it up so that I can choose which benchmarks to run and on which
matmul implementations is pretty difficult. I may use Codex to wire this up and read through it so I understand it,
but benchmarking isn't the primary purpose of this project so I'm don't want to get too hung up on it beyond
understanding how it works.

I just implemented the loop-reordered matmuls, I think I'll have some python data vis to see growth rate per function
and per matrix size.

## 10/4

Gonna work on cache-aware tiling next!!

I've just learned about what it even is, super interesting. I think it's pretty neat to see the things I've learned in CS 2110 and in the paper
"What every programmer should know about memory" (title paraphrased), lots of the things I didn't understand from that paper are making more sense now.