#pragma once

/*
* This file implements two hash functions:
* 
*		CompactHash	- an all-in-one hasher
*		CompactHash_streaming - a streaming-capable hash class
* 
* - Both are non-cryptographic
* - Both return identical results.
* - Both are based on the wyhash mixing function
* - Both pass SMHasher (rurban version) with no adverse findings. See file SMHasher_results.txt
* - CompactHash is approximately 5% faster than CompactHash_streaming
*/

#include <cstdint>
#include <intrin.h> // _umul128
#include <algorithm>    // std::min
#include <array>        // std::array
#include <assert.h>
#include <cstddef>		// size_t
#include <cstring>      // memcpy, memset
#include <cstdlib>
#include <iostream>
#include <stdexcept>    // std::runtime_error
#include <string>
#include <string_view>
#include <type_traits>   // for std::is_trivially_copyable_v
#include <vector>



/////////////////
// wymix
/////////////////
namespace ch {
#if _MSC_VER
	[[nodiscard]] inline uint64_t wymix(uint64_t a, uint64_t b) noexcept {
		uint64_t hi;
		uint64_t lo = _umul128(a, b, &hi);
		return (lo ^ hi) ^ (a ^ b);
	}
#else
	[[nodiscard]] inline uint64_t wymix(uint64_t a, uint64_t b) noexcept {
		unsigned __int128 product = (unsigned __int128)a * b;
		uint64_t lo = (uint64_t)product;
		uint64_t hi = (uint64_t)(product >> 64);
		return (lo ^ hi) ^ (a ^ b);
	}
#endif
}

/////////////////
// CompactHash
/////////////////
namespace ch {
	// Constants for CompactHash, all selected from the first 50 8-byte words of the fractional part of the 
	// golden ratio. It is expected that these constants are not particularly special, and that any other
	// selection of reasonable well distributed bits would function similarly. The real strength in these
	// hash functions is not the contants, it is the wymix function.
	// 
	// Source of phi digits:  https://www.numberworld.org/constants.html
	// --- Seed Mixing ---
	constexpr uint64_t SEED_MIX_0 = 0xddcb5d18576d77b6ull;
	constexpr uint64_t SEED_MIX_1 = 0x00faa684ecba752dull;

	// --- Counter Initialization ---
	constexpr uint64_t INITIAL_COUNTER_0 = 0xf39cc0605cedc834ull;
	constexpr uint64_t INITIAL_COUNTER_1 = 0x1082276bf3a27251ull;

	// --- Per-block Counter Increments (must be odd) ---
	constexpr uint64_t COUNTER_INCREMENT_0 = 0x9e3779b97f4a7c15ull;
	constexpr uint64_t COUNTER_INCREMENT_1 = 0xa2e834c5893a39ebull;

	// --- Finalization Mixing ---
	constexpr uint64_t FINAL_MIX_0 = 0x0347045b5bf1827full;
	constexpr uint64_t FINAL_MIX_1 = 0x01886f0928403002ull;

	/**
	 * Fast, non-cryptographic 128-bit hash function with good avalanche properties.
	 *
	 * - 100% C++23 compliant, ~30 LOC (not counting constants and wymix definition)
	 * - Borrows mixing function from wyhash/rapidhash, with PHI-derived constants.
	 * - Simple 2-lane architecture
	 * - Bulk throughput: ~9.8-10.0 GB/s @ 3 GHz on x64 (unrolled/aligned cases)
	 * - Small key latency: ~29-34 cycles (1–31 bytes)
	 * - Uses Microsoft _umul128 intrinsic or GCC/Clang __int128 for 64x64->128 bit multiplication.
	 * - Passes full rurban smhasher suite with zero collisions and worst-case avalanche bias < 0.85%
	 */
	[[nodiscard]] inline std::array<uint64_t, 2> CompactHash(const void* x, const size_t size, const uint64_t seed = 0) noexcept {
		assert(x != nullptr || size == 0);

		// Initial state and counter
		uint64_t state[2] = { SEED_MIX_0 ^ seed, SEED_MIX_1 + seed };
		uint64_t counter[2] = { INITIAL_COUNTER_0 ^ seed, INITIAL_COUNTER_1 + seed };

		const uint8_t* data = static_cast<const uint8_t*>(x);
		size_t remaining = size;

		// Main input loop: process 16 byte chunks
		while (remaining >= 16) {
			uint64_t word[2];
			memcpy(word, data, 16);
			state[0] = wymix(state[0], word[0] ^ (counter[0] += COUNTER_INCREMENT_0));
			state[1] = wymix(state[1], word[1] ^ (counter[1] += COUNTER_INCREMENT_1));
			remaining -= 16;
			data += 16;
		}

		// Tail handling
		uint64_t word[2] = { 0, 0 };
		if (remaining > 0)
			memcpy(word, data, remaining); // copy any remaining bytes into the word
		((unsigned char*)word)[remaining] = 0x80; // The 0x80 bit
		state[0] = wymix(state[0], word[0] ^ (counter[0] += COUNTER_INCREMENT_0)); // process
		state[1] = wymix(state[1], word[1] ^ (counter[1] += COUNTER_INCREMENT_1));

		// Inject length 
		uint64_t L = size * 0xbea225f9eb34556d; L ^= L >> 29;
		state[0] ^= L;

		// Final cross-lane mixing.
		state[0] = wymix(state[0], state[1] ^ (counter[0] += COUNTER_INCREMENT_0));
		state[1] = wymix(state[1], state[0] ^ (counter[1] += COUNTER_INCREMENT_1));

		return { state[0], state[1] };
	}
}

/////////////////
// CompactHash_streaming
/////////////////
namespace ch {
	class CompactHash_streaming
	{
		/*
		Class-based version of CompactHash. More versatile, and very slightly slower, than the original.
		Returns identical hash values.

		Performance:
			Bulk: 9.2-9.7 GB/s
			Small Key: 37-52 cycles/hash
		*/
		// Initial state and counter
		uint64_t state[2];
		uint64_t counter[2];
		uint64_t byte_counter;

		// input buffer
		uint8_t buffer[16];
		size_t pos;

	public:
		CompactHash_streaming(const uint64_t seed = 0) noexcept
		{
			state[0] = SEED_MIX_0 ^ seed;
			state[1] = SEED_MIX_1 + seed;
			counter[0] = INITIAL_COUNTER_0 ^ seed;
			counter[1] = INITIAL_COUNTER_1 + seed;
			byte_counter = 0;
			pos = 0;
		}

		inline CompactHash_streaming& insert(const void* v, size_t size) noexcept {
			assert(v != nullptr || size == 0);

			const uint8_t* data = reinterpret_cast<const uint8_t*>(v);
			byte_counter += size;

			// Handle case where buffer is not empty
			if (pos != 0) {
				size_t M = std::min(16 - pos, size);
				memcpy(buffer + pos, data, M);
				data += M;
				size -= M;

				if ((pos += M) == 16) {
					update_state(buffer);
					pos = 0;
				}
			}

			// Bulk processing - bypass buffer, insert directly out of data
			const uint8_t* d = data;
			const size_t NBlocks = size / 16;
			for (size_t i = 0; i < NBlocks; i++) {
				uint64_t x[2];
				memcpy(x, d + 16 * i, 16);
				state[0] = wymix(state[0], x[0] ^ (counter[0] += COUNTER_INCREMENT_0));
				state[1] = wymix(state[1], x[1] ^ (counter[1] += COUNTER_INCREMENT_1));
			}
			data += NBlocks * 16;
			size -= NBlocks * 16;
			pos = 0;

			// Copy any remaining bytes to the buffer
			if (size > 0) {
				memcpy(buffer, data, size);
				pos = size;
			}

			return *this;
		}

		inline std::array<uint64_t, 2> finalize() const noexcept {
			// Working copies (const method → no mutation of object state)
			uint64_t s0 = state[0];
			uint64_t s1 = state[1];
			uint64_t c0 = counter[0];
			uint64_t c1 = counter[1];

			// Build tail correctly: start from zero, copy only the valid bytes, pad with 0x80
			uint64_t tail[2] = { 0, 0 };
			if (pos > 0) {
				memcpy(tail, buffer, pos);           // copy only the bytes we have
			}
			((uint8_t*)tail)[pos] = 0x80;            // set the padding bit (overwrites byte pos)

			// Process tail block (matches one-shot logic)
			s0 = wymix(s0, tail[0] ^ (c0 += COUNTER_INCREMENT_0));
			s1 = wymix(s1, tail[1] ^ (c1 += COUNTER_INCREMENT_1));

			// Length mixing — same place and strength as original
			uint64_t L = byte_counter * 0xbea225f9eb34556dull;
			L ^= L >> 29;
			s0 ^= L;

			// Final cross-lane mix (extra counter steps match original)
			s0 = wymix(s0, s1 ^ (c0 += COUNTER_INCREMENT_0));
			s1 = wymix(s1, s0 ^ (c1 += COUNTER_INCREMENT_1));

			return { s0, s1 };
		}
	private:
		inline void update_state(const uint8_t* p) noexcept {
			uint64_t x[2];
			memcpy(x, p, 16);
			state[0] = wymix(state[0], x[0] ^ (counter[0] += COUNTER_INCREMENT_0));
			state[1] = wymix(state[1], x[1] ^ (counter[1] += COUNTER_INCREMENT_1));
		}
	}; // class CompactHash_streaming

	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, int8_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, int16_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, int32_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, int64_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, uint8_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, uint16_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, uint32_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, uint64_t value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, float value) { return ch.insert(&value, sizeof(value)); }
	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, double value) { return ch.insert(&value, sizeof(value)); }

	// ─────────────────────────────────────────────────────────────────────────────
	// std::array<T, N> — generic fallback (non-trivial or unsafe to bulk-copy)
	// ─────────────────────────────────────────────────────────────────────────────
	template <class T, std::size_t N>
	inline CompactHash_streaming&
		operator<<(CompactHash_streaming& ch, const std::array<T, N>& arr)
	{
		for (const T& x : arr) {
			ch << x;
		}
		return ch;
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// std::array<T, N> — bulk insert when T is trivially copyable
	// ─────────────────────────────────────────────────────────────────────────────
	template <class T, std::size_t N>
	inline std::enable_if_t<std::is_trivially_copyable_v<T>, CompactHash_streaming&>
		operator<<(CompactHash_streaming& ch, const std::array<T, N>& arr) noexcept
	{
		// arr.data() returns const T*, safe & contiguous when T is trivially copyable
		ch.insert(arr.data(), N * sizeof(T));
		return ch;
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// std::vector<T> — generic fallback (non-trivial elements or empty vec)
	// ─────────────────────────────────────────────────────────────────────────────
	template <class T>
	inline CompactHash_streaming&
		operator<<(CompactHash_streaming& ch, const std::vector<T>& vec)  // ← const &
	{
		for (const T& x : vec) {   // ← const & is safer / more idiomatic
			ch << x;
		}
		return ch;
	}

	// ─────────────────────────────────────────────────────────────────────────────
	// std::vector<T> — bulk insert when T is trivially copyable
	// ─────────────────────────────────────────────────────────────────────────────
	template <class T>
	inline std::enable_if_t<std::is_trivially_copyable_v<T>, CompactHash_streaming&>
		operator<<(CompactHash_streaming& ch, const std::vector<T>& vec) noexcept
	{
		if (!vec.empty()) {   // ← protects against size=0 (though insert handles it, nicer)
			ch.insert(vec.data(), vec.size() * sizeof(T));
		}
		return ch;
	}

	inline CompactHash_streaming& operator<<(CompactHash_streaming& ch, std::string str) {
		return ch.insert(str.data(), str.size());
	}

}// namespace ch

