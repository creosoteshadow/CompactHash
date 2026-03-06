CompactHash

CompactHash is a fast, non-cryptographic hash function written in modern C++.

It is designed for high throughput (targeting ~10 GB/s on modern hardware) while passing the full SMHasher test suite — a de-facto standard for evaluating non-cryptographic hashes.

Currently in early development — clean, minimal, and focused on correctness + speed.

Features

- Extremely fast mixing using protected wymix-style multiplication (inspired by rapidhash / wyhash)
- 128-bit output
- Stateless / seedable API
- Passes all SMHasher tests (see results below)
- Header-only implementation (CompactHash.h) for easy integration
- No external dependencies
- Modern C++ (C++11+ compatible, with optional C++17/20 improvements)

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

Inspired by rapidhash, wyhash, xxHash, and SMHasher-driven development.

Contributing
  - Contributions welcome — especially benchmarks, ports, or fixes!

License
  -MIT License

Happy hashing!

— Jim (creosoteshadow)
