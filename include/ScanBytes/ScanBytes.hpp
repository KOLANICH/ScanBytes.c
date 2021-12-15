#pragma once

#include <vector>
#include <iostream>
#include <string>
#include <filesystem>

#include <stdlib.h>
#include <string.h>

#include <span>
#include <memory>
#include <thread>
#include <chrono>
#include <utility>
#include <span>

#include "MultiThreadedOrderedAppendOnlyAllocator.hpp"

namespace ScanBytes{

	typedef std::span<uint8_t> ScannableT;

	enum class Backend: uint8_t{
		Unknown = 0,
		Auto = 1,
		JIT = 2, // "\n" <> <> "," 2.04798 6.47731 CSV <> 8.27703
		Fallback = 3, // "\n" 4.12886 11.9535  "," 2.95809 13.2762 CSV <> 13.7314
		LF = 4, // "\n" <> <>
		CSV = 5, // "\n" <> <> "," <> <> CSV <> 5.98905
		TSV = 6,
		Space = 7,
		Punct = 8,
	};


	using BenchmarkResultT = std::vector<std::chrono::duration<double, std::micro>>;

	template<Backend backendEnum>
	BenchmarkResultT benchmarkWithDetector(ScannableT m, std::vector<uint8_t> &charsToScanFor, uint8_t benchmarkAttempts = 10);

	extern const char * backendNames[];
	Backend getBackendByName(std::string& name);

	Backend getGenericBackend();
	Backend detectProperBackend(std::vector<uint8_t> &charsToScanFor);
	Backend getBackendByName(std::string& n);

	using NumbersAllocator = MTOAOA<uint64_t>;
	using NBST = NumbersAllocator::StorageT;
	NBST scan(ScannableT m, std::vector<uint8_t> charsToScanFor, Backend b = Backend::Auto);

	BenchmarkResultT benchmark(Backend b, ScannableT m, std::vector<uint8_t> charsToScanFor, uint8_t benchmarkAttempts = 10);

	void sortIndices(NBST &chunks);

	void dumpIndices(NBST &chunks);
};
