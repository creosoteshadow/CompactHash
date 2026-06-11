#pragma once
// file PHI.h

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

#include <cstdint>

// Successive 64-bit chunks of the binary fractional part of the golden ratio φ = (1 + √5)/2
// ≈ 0.6180339887... starting after the binary point.
// Sourced from high-precision hex digits at https://www.numberworld.org/constants.html
inline constexpr uint64_t PHI[50] = {
	0x9e3779b97f4a7c15ull, 0xf39cc0605cedc834ull, 0x1082276bf3a27251ull, 0xf86c6a11d0c18e95ull,
	0x2767f0b153d27b7full, 0x0347045b5bf1827full, 0x01886f0928403002ull, 0xc1d64ba40f335e36ull,
	0xf06ad7ae9717877eull, 0x85839d6effbd7dc6ull, 0x64d325d1c5371682ull, 0xcadd0cccfdffbbe1ull,
	0x626e33b8d04b4331ull, 0xbbf73c790d94f79dull, 0x471c4ab3ed3d82a5ull, 0xfec507705e4ae6e5ull,
	0xe73a9b91f3aa4db2ull, 0x87ae44f332e923a7ull, 0x3cb91648e428e975ull, 0xa3781eb01b49d867ull,
	0x4fa1508419e0eaa4ull, 0x038b352d9bad30f4ull, 0x485b71a8ef64452aull, 0x0dd40dc8cb8f9a2dull,
	0x4c514f1b229dcaa2ull, 0x22ac268e9666e4a8ull, 0x66769145f5f5880aull, 0x9d0acd3b9e8c682full,
	0x4f810320abeb9403ull, 0x4e70f21608c061abull, 0x1c1caef1ebdcefbcull, 0x72134ecf06ed82bfull,
	0xb7d8eb1a41901d65ull, 0xf5c8cab2accbc32eull, 0xab1fbe8284f2b44bull, 0xa2e834c5893a39eaull,
	0x7865443f489c37f8ull, 0x742acd895afd87b4ull, 0x67d22a40d098f30dull, 0xd2cafdeb3abb3a13ull,
	0x507b46b3d757fc04ull, 0x001906e1767d40c3ull, 0xa3792a26eeef2ab5ull, 0xbd6685b915b56294ull, 
	0x00faa684ecba752dull, 0xddcb5d18576d77b6ull, 0x52ac0d9999736866ull, 0x04128f0cd4274359ull, 
	0x6deb2d42c789d64bull, 0x92658610b5b95c71ull };

