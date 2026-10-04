# Optimizations

What are the optimizations, and why/how do they work??

## First, the naive implementation

Naive implementation iterates first over the first dimension of the first matrix, then over the second dimension of the second matrix, then over the shared dimension.
This ends up being pretty slow because we have poor access patterns around memory, and we aren't able to use the cache effectively, or cache values ourselves.

Reordering the loops can make our throughput, clock speed, and cache usage a lot more effective, or even worse! Running

```
./build/matmul_bench --all-implementations --all-metrics \
  --benchmark_filter='.*N:512.*
```

gives us a quick glance. Running in ikj order os by far the most efficient. It takes the fewest cycles, has the highest effective bytes/second, most flops/s, and uses the fewest instructions. Below is a comparison of some of the benchmarks for all of our loop-reordered implementations. Note the logarithmic scale.

![alt text](loop-reordering-benchmark-results.png)

ikj being the most efficient is pretty interesting to look at. The reason is that we can (1) cache one of our fetch operations, which is common for our re-orders. What makes it
faster than the other re-orders though is that it allows us to walk through memory very quickly and efficiently. These matrices are storied in contiguous memory, and this 
loop ordering lets us walk through pages of memory by moving just 1 address at a time both for fetching from the second matrix and for writing to the memory addresses for the
results matrix. This is as efficient as it gets!

There are lots of better ways that we can utilize the cache and take advantage of parallelism, which we'll explore in the upcoming implementations.