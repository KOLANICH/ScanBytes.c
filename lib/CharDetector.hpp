#pragma once
#include <cstdint>
#include <utility>
#include <vector>

#include "jit.hpp"

struct FallbackCharDetector{
	uint8_t charz[256 / 8];

	inline FallbackCharDetector(std::vector<uint8_t> &v){
		memset(charz, 0, sizeof(charz));
		for(auto c: v){
			add(c);
		}
	}

	inline std::pair<uint8_t, uint8_t> decode(uint8_t c){
		uint8_t offs = c >> 3;
		uint8_t inOffs = c & 0x07;
		return {offs, inOffs};
	}

	inline void add(uint8_t c){
		auto p = decode(c);
		charz[p.first] |= (1 << p.second);
	}

	inline bool operator()(uint8_t c){
		auto p = decode(c);
		return charz[p.first] & (1 << p.second);
	}
};

template <uint8_t targetChar>
struct SingleCharDetector{
	inline SingleCharDetector(std::vector<uint8_t> &v){};

	inline bool operator()(uint8_t c) { return c == targetChar;};
};

template <uint8_t mostFrequent, uint8_t leastFrequent>
struct TwoCharsDetector{
	inline TwoCharsDetector(std::vector<uint8_t> &v){};

	inline bool operator()(uint8_t c) { return c == mostFrequent || c == leastFrequent;};
};

struct PunctDetector{
	inline PunctDetector(std::vector<uint8_t> &v){};

	inline bool operator()(uint8_t c) {
		return (
			(33 <= c && c < 48) ||
			(58<= c && c < 65) ||
			(91<= c && c < 97) ||
			(123<= c && c < 127)
		);
	};
};

using LineBreaksDetector = SingleCharDetector<'\n'>;
using SpacesDetector = SingleCharDetector<' '>;
using CSVDetector = TwoCharsDetector<',', '\n'>;
using TSVDetector = TwoCharsDetector<'\t', '\n'>;

