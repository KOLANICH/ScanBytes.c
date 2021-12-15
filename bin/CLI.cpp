#include <iostream>
#include <set>

#include <mio/mmap.hpp>
#include <ScanBytes/ScanBytes.hpp>

#include <HydrArgs/HydrArgs.hpp>

#include <numeric>

typedef int (CmdFuncPtr) (ScanBytes::Backend b, std::vector<uint8_t> &charsToScanFor, ScanBytes::ScannableT &m);

int index(ScanBytes::Backend b, std::vector<uint8_t> &charsToScanFor, ScanBytes::ScannableT &m){
	auto res = ScanBytes::scan(m, charsToScanFor, b);
	ScanBytes::sortIndices(res);
	ScanBytes::dumpIndices(res);

	return EXIT_SUCCESS;
}

int benchmark(ScanBytes::Backend b, std::vector<uint8_t> &charsToScanFor, ScanBytes::ScannableT &m){
	if(b == ScanBytes::Backend::Auto){
		b = ScanBytes::getGenericBackend();
	}

	auto benchmarkAttempts = 10;
	auto res = ScanBytes::benchmark(b, m, charsToScanFor, benchmarkAttempts);

	double sum = 0., sumSq = 0.;
	for(auto el: res){
		auto elD = el.count();
		sum += elD;
		sumSq += elD * elD;
	}
	double mean = sum / benchmarkAttempts;
	double sqExp = sumSq / benchmarkAttempts;
	double var = sqExp - mean * mean;

	std::cerr << mean << " (std=" << std::sqrt(var) << ")" << " us" << std::endl;

	return EXIT_SUCCESS;
}


const char usage[] = "ScanBytes <i/s|v|b> <file>";
const char programName[] = "ScanBytes";
const char description[] = "ScanBytes allows you to scan a file for occurences of bytes and get a file with offsets.";

using namespace HydrArgs;
using namespace HydrArgs::Backend;

int main(int argc, const char ** argv){
	SArg<ArgType::string> commandArg{'c', "command", "Command to run, can be s (scan), v (verify) or bs (benchmark scan)", 1, "i|v|b", "", ""};
	SArg<ArgType::string> fileArg{'f', "file", "Scanned file", 1, "path to file", "", ""};

	SArg<ArgType::string> backendArg{'b', "backend", "The scanner implementation", 0, "Backend name", "", "Auto"};
	SArg<ArgType::string> alphabetArg{'a', "alphabet", "Chars to use as separators", 0, "alphabet", "", "\n"};

	std::vector<Arg*> dashedSpec{&backendArg, &alphabetArg};

	std::vector<Arg*> positionalSpec{&commandArg, &fileArg};

	std::unique_ptr<IArgsParser> ap{argsParserFactory(programName, description, usage, dashedSpec, positionalSpec)};

	auto status = (*ap)({argv[0], {&argv[1], static_cast<size_t>(argc - 1)}});

	if(status.parsingStatus){
		return status.parsingStatus.returnCode;
	}

	CmdFuncPtr *cmdPtr = nullptr;

	if(commandArg.value == "s"){
		cmdPtr = index;
	} else if (commandArg.value == "bs") {
		cmdPtr = benchmark;
	} else if (commandArg.value == "v") {
		std::cerr << "v not yet implemented" << std::endl;
		return EXIT_FAILURE;
	} else{
		std::cerr << "Invalid command " << commandArg.value << std::endl;
		ap->printHelp(std::cout, argv[0]);
		return EXIT_FAILURE;
	}

	auto b = ScanBytes::getBackendByName(backendArg.value);
	if(b == ScanBytes::Backend::Unknown){
		std::cerr << "Invalid backend name: " << backendArg.value << std::endl;
		ap->printHelp(std::cout, argv[0]);
		return EXIT_FAILURE;
	}

	std::vector<uint8_t> charsToScanFor;
	charsToScanFor.reserve(alphabetArg.value.size());
	{
		// deduplicating
		uint8_t charz[256 / 8];
		memset(charz, 0, sizeof(charz));
		for(uint8_t c: alphabetArg.value){
			uint8_t offs = c >> 3;
			uint8_t inOffs = c & 0x07;
			if(!(charz[inOffs] & (1 << inOffs))){
				charz[offs] |= (1 << inOffs);
				charsToScanFor.emplace_back(c);
			}
		}
	}

	mio::ummap_source m(fileArg.value);
	ScanBytes::ScannableT s{&m[0], m.size()};

	return cmdPtr(b, charsToScanFor, s);
}
