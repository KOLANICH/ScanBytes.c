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
#include <functional>

#include <ScanBytes/ScanBytes.hpp>
#include <ScanBytes/MultiThreadedOrderedAppendOnlyAllocator.hpp>
#include "CharDetector.hpp"

namespace ScanBytes{

	template <Backend typeValue> struct GetBackendFromEnum{};
	template<> struct GetBackendFromEnum<Backend::JIT> {using type = JittedCharDetector;};
	template<> struct GetBackendFromEnum<Backend::Fallback> {using type = FallbackCharDetector;};
	template<> struct GetBackendFromEnum<Backend::LF> {using type = LineBreaksDetector;};
	template<> struct GetBackendFromEnum<Backend::CSV> {using type = CSVDetector;};
	template<> struct GetBackendFromEnum<Backend::TSV> {using type = TSVDetector;};


	const char * backendNames[] = {
		"Unknown",
		"Auto",
		"JIT",
		"Fallback",
		"LF",
		"CSV",
		"TSV",
		"Space",
		"Punct",
	};

	std::unordered_map<std::string, Backend> backendsByNames{
		{backendNames[static_cast<uint8_t>(Backend::Auto)], Backend::Auto},
		{backendNames[static_cast<uint8_t>(Backend::JIT)], Backend::JIT},
		{backendNames[static_cast<uint8_t>(Backend::Fallback)], Backend::Fallback},
		{backendNames[static_cast<uint8_t>(Backend::LF)], Backend::LF},
		{backendNames[static_cast<uint8_t>(Backend::CSV)], Backend::CSV},
		{backendNames[static_cast<uint8_t>(Backend::TSV)], Backend::TSV},
		{backendNames[static_cast<uint8_t>(Backend::Space)], Backend::Space},
		{backendNames[static_cast<uint8_t>(Backend::Punct)], Backend::Punct},
	};

	Backend getBackendByName(std::string& name){
		auto it = backendsByNames.find(name);
		if(it == end(backendsByNames)){
			return Backend::Unknown;
		}
		return it->second;
	}

	template<typename DetectorT>
	void singleSearchingThreadFunction(uint8_t id, NumbersAllocator *nall, DetectorT *d, uint8_t *m, size_t start, size_t stop){
		auto t = nall->getForThread(id);

		for(size_t i=start; i<stop; ++i){
			if((*d)(m[i])){
				t.append(i);
			}
		}
	}

	template<typename DetectorT>
	NBST scan(ScannableT m, DetectorT &d){
		/*
		The following threading solutions have been tried (the numbers are times of processing of 2 fifferent large files of different sizes):
		1. No multithreading, no parallelization. 0.475390 1.92215
		2. Single dedicated std::thread, no parallelization 0.378107 1.50243e+06
		2. Threading using tbb:parallel_for 0.182083 0.657477
		3. OpenMP 0.151552 0.648832
		4. std::thread, the current impl: 0.131101 0.488398

		surprisingly, std::thread from the stdlib is the fastest way of all of them.

		Note: the times have increased to 1s when I have introduced the abstractions to allow selection of chars in runtime. Seems like they are not fully optimized out.
		*/
		NumbersAllocator nall;

		auto s = m.size();
		uint16_t procsCount = std::thread::hardware_concurrency();
		uint16_t lastProc = procsCount - 1;

		size_t shareSize = s / procsCount;

		std::vector<std::thread> threadList;

		for(uint8_t i = 0; i < lastProc; ++i){
			size_t start = shareSize * i;
			size_t stop = start + shareSize;
			threadList.emplace_back(std::thread(singleSearchingThreadFunction<DetectorT>, i, &nall, &d, &m[0], start, stop));
		}
		threadList.emplace_back(std::thread(singleSearchingThreadFunction<DetectorT>, lastProc, &nall, &d, &m[0], shareSize * lastProc, s));
		std::for_each(threadList.begin(),threadList.end(), std::mem_fn(&std::thread::join));

		return std::move(nall.chunks);
	}

	template<Backend backendEnum>
	NBST scanWithFreshDetector(ScannableT m, std::vector<uint8_t> charsToScanFor){
		using DetectorT = typename GetBackendFromEnum<backendEnum>::type;
		DetectorT d(charsToScanFor);
		return std::move(scan(m, d));
	}

	template<Backend backendEnum>
	BenchmarkResultT benchmarkWithDetector(ScannableT m, std::vector<uint8_t> &charsToScanFor, uint8_t benchmarkAttempts){
		using DetectorT = typename GetBackendFromEnum<backendEnum>::type;
		DetectorT d(charsToScanFor);

		BenchmarkResultT res;
		res.reserve(benchmarkAttempts);

		NBST resSt;
		for(decltype(benchmarkAttempts) i=0;i<benchmarkAttempts;++i){
			auto t1 = std::chrono::high_resolution_clock::now();
			resSt = scan(m, d);
			auto t2 = std::chrono::high_resolution_clock::now();

			resSt.clear();
			res.emplace_back(t2 - t1);
		}

		return res;
	}

	Backend detectProperBackendInternal(std::vector<uint8_t> &charsToScanFor, size_t s){
		if(s == 1){
			if(charsToScanFor[0] == '\n'){
				return Backend::LF;
			}
		} else if(s == 2){
			if((charsToScanFor[0] == '\n' && charsToScanFor[1] == ',') || (charsToScanFor[1] == '\n' && charsToScanFor[0] == ',')){
				return Backend::CSV;
			} else if((charsToScanFor[0] == '\n' && charsToScanFor[1] == '\t') || (charsToScanFor[1] == '\n' && charsToScanFor[0] == '\t')){
				return Backend::TSV;
			}
		}
		return getGenericBackend();
	}

	Backend detectProperBackend(std::vector<uint8_t> &charsToScanFor){
		auto s = charsToScanFor.size();
		if(s){
			return detectProperBackendInternal(charsToScanFor, s);
		}
		throw std::logic_error("Set of the chars must be not empty");
	}

	Backend getGenericBackend(){
		#ifdef SCANBYTES_JIT_SUPPORTED
		return Backend::JIT;
		#else
		return Backend::Fallback;
		#endif
	}

	NBST scanWithBackend(ScannableT m, std::vector<uint8_t> charsToScanFor, Backend b = Backend::Auto){
		switch(b){
			#ifdef SCANBYTES_JIT_SUPPORTED
			case Backend::JIT:
				return scanWithFreshDetector<Backend::JIT>(m, charsToScanFor);
			break;
			#endif
			case Backend::Fallback:
				return scanWithFreshDetector<Backend::Fallback>(m, charsToScanFor);
			break;
			case Backend::LF:
				return scanWithFreshDetector<Backend::LF>(m, charsToScanFor);
			break;
			case Backend::CSV:
				return scanWithFreshDetector<Backend::CSV>(m, charsToScanFor);
			break;
			case Backend::TSV:
				return scanWithFreshDetector<Backend::TSV>(m, charsToScanFor);
			break;
			default:
				throw std::logic_error("Unknown backend");
		}
	}

	NBST scan(ScannableT m, std::vector<uint8_t> charsToScanFor, Backend b){
		auto s = charsToScanFor.size();
		if(s){
			if(b == Backend::Auto){
				b = detectProperBackendInternal(charsToScanFor, s);
			}
			return scanWithBackend(m, charsToScanFor, b);
		}
		throw std::logic_error("Set of the chars must be not empty");
	}

	BenchmarkResultT benchmark(Backend b, ScannableT m, std::vector<uint8_t> charsToScanFor, uint8_t benchmarkAttempts){
		switch(b){
			case Backend::JIT:
				return benchmarkWithDetector<Backend::JIT>(m, charsToScanFor, benchmarkAttempts);
			break;
			case Backend::Fallback:
				return benchmarkWithDetector<Backend::Fallback>(m, charsToScanFor, benchmarkAttempts);
			break;
			case Backend::LF:
				return benchmarkWithDetector<Backend::LF>(m, charsToScanFor, benchmarkAttempts);
			break;
			case Backend::CSV:
				return benchmarkWithDetector<Backend::CSV>(m, charsToScanFor, benchmarkAttempts);
			break;
			case Backend::TSV:
				return benchmarkWithDetector<Backend::TSV>(m, charsToScanFor, benchmarkAttempts);
			break;
			default:
				throw std::logic_error("Unknown backend");
		}
	}

	void sortIndices(NBST &chunks){
		std::sort(begin(chunks), end(chunks), [](auto &a, auto &b) -> bool {
			return (*a) <= (*b);
		});
	}

	void dumpIndices(NBST &chunks){
		for(auto &chunk: chunks){
			auto &offsets = chunk->vec;
			auto s = offsets.size();
			if(s){
				std::cout.write(reinterpret_cast<char *>(&offsets[0]), s * sizeof(offsets[0]));
			}
		}
	}
};
