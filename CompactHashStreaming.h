#pragma once

/*
MIT License

Copyright (c) 2026 Jim Staley

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files(the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and /or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


/*
 * CompactHashStreaming - Fast 128-bit non-cryptographic hash function
 *
 * Summary: High-speed, high-quality hash. 128-bit output.
 * Passes SMHasher (rurban version).
 *
 * Performance: ~10 GB/s on modern x64 CPUs (single-threaded).
 * Quality: Passes SMHasher with no findings.
 *
 * Design emphasizes:
 * - Counter-based lanes to defeat repeated sequence attacks
 * - Strong mixing via wymix + _umul128
 * - Unified tail handling with 0x80 padding + length injection
 *
 * Constants: Successive 64-bit chunks of the binary fractional part
 * of the golden ratio φ = (1 + √5)/2 ≈ 0.6180339887...
 * Sourced from https://www.numberworld.org/constants.html
 *
 * Security
 * Not cryptographic.
 *		Not designed for adversarial environments.
 *		Not tested in an adversarial environment.
 *		Not reviewed by cryptographic experts.
 * Do not use in applications where security is important.
 */

#include "PHI.h" // array of constants derived from the golden ratio

#include <array>
#include <cstdint>
#include <iostream> // std::cerr
#include <stdlib.h> // std::exit
#include <string.h> // memcpy



[[noreturn]] inline void throw_compact_hash_error() {
	std::cerr << "Fatal error: Null Pointer in CompactHash with non-zero length.\n";
	std::exit(EXIT_FAILURE);
}



class CompactHashStreaming {
	static constexpr uint64_t SEED_MIX_0 = PHI[45];
	static constexpr uint64_t SEED_MIX_1 = PHI[44];
	static constexpr uint64_t INITIAL_COUNTER_0 = PHI[1];
	static constexpr uint64_t INITIAL_COUNTER_1 = PHI[2];
	static constexpr uint64_t COUNTER_INCREMENT_0 = PHI[0]; // odd
	static constexpr uint64_t COUNTER_INCREMENT_1 = PHI[3]; // odd

	uint64_t state[2] = { 0 };
	uint64_t counter[2] = { 0 };
	uint8_t buffer[16] = { 0 };
	size_t buffer_index = 0; // Changed to size_t to match std::min
	size_t TotalBytes = 0;

public:
	CompactHashStreaming(uint64_t seed = 0)
	{
		seed += PHI[30];
		state[0] = wymix(seed, SEED_MIX_0 ^ seed);
		state[1] = wymix(seed, SEED_MIX_1 ^ seed);
		counter[0] = INITIAL_COUNTER_0 ^ seed;
		counter[1] = INITIAL_COUNTER_1 + seed;
	}

	void insert(const void* x, size_t size)
	{
		if (x == nullptr && size > 0) [[unlikely]]
			throw_compact_hash_error();

		TotalBytes += size;
		const uint8_t* data = static_cast<const uint8_t*>(x);
		size_t remaining = size;

		// 1. Handle any bytes currently in the buffer
		if (buffer_index > 0) {
			size_t to_copy = std::min(size_t(16) - buffer_index, remaining);
			memcpy(buffer + buffer_index, data, to_copy);
			buffer_index += to_copy;

			if (buffer_index == 16) {
				inject(buffer);
				buffer_index = 0;
				memset(buffer, 0, 16); // not strictly required -- defensive programming
			}
			data += to_copy;
			remaining -= to_copy;
		}

		// 2. Main loop - 16 bytes per iteration. 
		while (remaining >= 16)
		{
			// If we are inside this loop, the buffer is now empty. So, it is safe to bypass the buffer.
			inject(data);
			data += 16;
			remaining -= 16;
		}

		// 3. Put any remaining bytes in the buffer
		if (remaining > 0) {
			memcpy(buffer + buffer_index, data, remaining);
			buffer_index += remaining; // Update the index
		}
	}

	[[nodiscard]] std::array<uint64_t, 2> hash128() const {
		// Copy state and counters so finalization 
		// doesn't permanently destroy the internal state.
		CompactHashStreaming temp(*this);
		return temp.finalize();
	}


private:
	static inline uint64_t wymix(uint64_t a, uint64_t b) noexcept {
		uint64_t lo, hi;
#ifdef _MSC_VER
		lo = _umul128(a, b, &hi);
#else
		__uint128_t r = (__uint128_t)a * b;
		lo = (uint64_t)r;
		hi = (uint64_t)(r >> 64);
#endif
		/*
		* wyhash's optional protection against multi-collision attacks uses (lo^hi) ^ (a^b). Since we are sometimes
		* using this in the form wymix(x, x^constant) (see, for instance, in the constructor), this would
		* reduce to (lo^hi) ^ constant, making the output independent of 'a' entirely
		* when a == b. Keeping '^ b' instead ensures 'b' always influences the output
		* regardless of the multiply result.
		*/
		return (lo ^ hi) ^ b;
	}


	// Inject 16 bytes into the state
	void inject(const uint8_t* input)
	{
		uint64_t word[2];
		memcpy(word, input, 16);

		// Architectural Note: A previous version used '=' assignment here. 
		// However, if a word precisely matched the incremented counter, 
		// wymix would output 0 and completely erase all accumulated historical entropy.
		// Switching to '+=' acts as a feed-forward mechanism, ensuring a 
		// zero-sink block merely acts as a no-op instead of a state wipe.
		state[0] += wymix(state[0], word[0] ^ (counter[0] += COUNTER_INCREMENT_0));
		state[1] += wymix(state[1], word[1] ^ (counter[1] += COUNTER_INCREMENT_1));
	}

	std::array<uint64_t, 2> finalize() {
		// 1. Create a clean, 16-byte stack buffer using uint8_t to match byte-indexing
		uint8_t tail[16] = { 0 };

		// 2. Copy whatever remaining bytes we have (guaranteed to be 0 to 15 bytes)
		if (buffer_index > 0) {
			memcpy(tail, buffer, buffer_index);
		}

		// 3. Append the domain separation bit right after the data.
		// If buffer_index is 0, it goes to tail[0]. If 15, it goes to tail[15].
		tail[buffer_index] = 0x80;

		// 4. Inject the final block safely
		inject(tail);

		// 5. Mix length into lane 0
		const uint64_t nbits = TotalBytes * 8ULL;
		state[0] = wymix(state[0], nbits ^ (counter[0] += COUNTER_INCREMENT_0));

		// 6. Finalization - cross mixing between the two lanes
		const uint64_t u0 = wymix(state[0], state[1] ^ (counter[0] += COUNTER_INCREMENT_0));
		const uint64_t u1 = wymix(state[1], state[0] ^ (counter[1] += COUNTER_INCREMENT_1));

		return { u0, u1 };
	}
};

// SMHasher test harness
void CompactHashStreaming_test(const void* key, int len, uint32_t seed, void* out) {
	// SMHasher passes a 32-bit seed, but your implementation expects 64-bit.
	// Upcasting is fine.
	CompactHashStreaming hasher(static_cast<uint64_t>(seed));

	hasher.insert(key, static_cast<size_t>(len));

	std::array<uint64_t, 2> result = hasher.hash128();

	// Copy the 128-bit result into SMHasher's output pointer
	memcpy(out, result.data(), 16);
}
