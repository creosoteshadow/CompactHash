CompactHash

CompactHash.h 
- implements two hash functions:
  - CompactHash	- an all-in-one hasher
  - CompactHash_streaming - a streaming-capable hash class
- Both are non-cryptographic
- Use wymix-style multiplication (inspired by rapidhash / wyhash)
- 128-bit output. Both return identical results.
- Both are based on the wyhash mixing function
- Passes all SMHasher tests (see results below)
- CompactHash is approximately 5% faster than CompactHash_streaming
- 100% C++, header-only
- No external dependencies

CompactHash and CompactHash_streaming are fast, non-cryptographic hash functions written in modern C++.

They designed for high throughput (targeting ~10 GB/s on modern hardware) while passing the full SMHasher test suite — a de-facto standard for evaluating non-cryptographic hashes.

Goals: clean, minimal, and focused on correctness + speed. 

Status: Works with MSWindows/MSVC, not tested on other systems. Passes SMHasher/rurban.

Performance / Quality
  - Preliminary benchmarks show throughput around 10 GB/s on typical desktop CPUs (exact numbers depend on hardware, compiler flags, and workload).
  - Full results (all tests passing) are committed in SMHasher_results.txt.
      - No collisions or failures observed in the standard suite.
      - Sanity checks
      - Key & seed sensitivity
      - Avalanche (bit independence & correlation)
      - Cyclic tests
      - Sparse & combo tests
      - Differential & long-stream tests
      - Zeroes & ones patterns

Usage
  - See main.cpp for a simple demonstration.

Why another hash?
  - CompactHash aims for an ultra-simple, high-quality sweet spot:
    - Better statistical quality than many "fast" hashes
    - Competitive speed with protected mixing (avoids some known weaknesses in unprotected wyhash-style)
    - Minimal code footprint
  - CompactHash DOES NOT claim the fastest speed possible -- see rapid_hash for that.
  - CompactHash DOES have straightforward / readable code.

Inspired by rapidhash, wyhash, xxHash, and SMHasher-driven development.

Contributing
  - Contributions welcome — especially benchmarks, ports, or fixes!

License
  -MIT License

Happy hashing!

— Jim (creosoteshadow)
