#pragma once

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>

struct PieceOfJit{
	const uint8_t *code;
	uint8_t size;
};

struct ModifiablePieceOfJit{
	PieceOfJit piece;
	uint8_t offset;
};

struct Architecture{
	PieceOfJit prolog;
	ModifiablePieceOfJit comparer;
	ModifiablePieceOfJit spacer;
	PieceOfJit epilog;
};


#if defined(__x86_64__) || defined(_M_X64)
	#define SCANBYTES_JIT_SUPPORTED 1

	/* some dummy functions pointers, otherwise we get linking errors */
	extern "C"{
		extern void jit_x86_64_exitPoint(void);
	};

	extern "C"{
		const uint8_t prolog[]{0xf3u, 0x0fu, 0x1eu, 0xfau     /*endbr64*/};
		const uint8_t comparer[]{0x40u, 0x80u, 0xffu, '\n',   /*cmpb    $0xd, %dil*/};
		const uint8_t spacer[]{0x74, 0x00,                    /*je +2 (jumps to next instruction, but we replace it with offset of epilog relative to next instruction) */};
		const uint8_t epilog[]{0xFF, 0xE0,                    /*jmp *(%rax) (rax must be populated with return address)  */};
	};

	constexpr Architecture ourArch {
		.prolog = {
			.code=prolog,
			.size=sizeof(prolog)
		},
		.comparer = {
			.piece={
				.code=comparer,
				.size=sizeof(comparer)
			},
			.offset = 3
		},
		.spacer = {
			.piece={
				.code=spacer,
				.size=sizeof(spacer)
			},
			.offset = 1
		},
		.epilog = {
			.code=epilog,
			.size=sizeof(epilog)
		}
	};
#else
#if defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	#undef SCANBYTES_JIT_SUPPORTED
#else
	#undef SCANBYTES_JIT_SUPPORTED
#endif
#endif


#ifdef SCANBYTES_JIT_SUPPORTED
	const size_t pageSize = 4 * 1024;

	#include <sys/mman.h>

	struct JittedCharDetector{
		uint8_t *funcJitCode;
		uint8_t *exitPointAddr;
		JittedCharDetector(std::vector<uint8_t> &v);
		~JittedCharDetector();

		inline bool operator()(uint8_t c){
			bool res = false;

			#if defined(__x86_64__) || defined(_M_X64)
				//https://stackoverflow.com/questions/49434489/relocation-r-x86-64-32-against-data-can-not-be-used-when-making-a-shared-obje?rq=1
				asm volatile(
					"movb %2, %%dil;\n"
					//"lea jit_x86_64_exitPoint(%%rip), %%rax;\n"
					"movq %3, %%rax;\n"
					"jmpq *%1;\n"
					"jit_x86_64_exitPoint:\n"
					: "=@cce" (res)
					: "r"(funcJitCode), "r" (c), "m" (exitPointAddr)
					: "%rax", "%rdi"
				);
			#endif
			//std::cout << "result = " << res << std::endl;
			return res;
		}
	};

	JittedCharDetector::JittedCharDetector(std::vector<uint8_t> &v){
		/*asm volatile(
			"lea jit_x86_64_exitPoint(%%rip), %%rax;\n"
			: "=%rax" (exitPointAddr)
			:
			: "%rax"
		);*/
		exitPointAddr = (uint8_t*) jit_x86_64_exitPoint;

		uint8_t bufferSize;
		uint8_t epilogOffset;
		{
			int16_t epilogOffset16 = ourArch.prolog.size + v.size() * (ourArch.comparer.piece.size + ourArch.spacer.piece.size) - ourArch.spacer.piece.size;
			int16_t bufferSize16 = epilogOffset16 + ourArch.epilog.size;
			if(bufferSize16 > std::numeric_limits<std::make_signed<decltype(bufferSize)>::type>::max()){
				throw std::logic_error("Buffer for JIT is larger than a maximum value of int8_t (which is maximum 1-byte positive immediate in x86).");
			}
			bufferSize = static_cast<int8_t>(bufferSize16);
			epilogOffset = static_cast<int8_t>(epilogOffset16);
		}

		static_assert (std::numeric_limits<decltype(bufferSize)>::max() < pageSize);

		funcJitCode = (uint8_t *) mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (funcJitCode == (void *)-1) {
			throw std::bad_alloc();
		}

		auto ptr = funcJitCode;
		memcpy(ptr, ourArch.prolog.code, ourArch.prolog.size);
		ptr += ourArch.prolog.size;

		uint8_t *epilogPtr = funcJitCode + epilogOffset;

		for(uint8_t i = 0; i<v.size() - 1; ++i){
			auto &el = v[i];
			memcpy(ptr, ourArch.comparer.piece.code, ourArch.comparer.piece.size);
			ptr[ourArch.comparer.offset] = el;
			ptr += ourArch.comparer.piece.size;

			memcpy(ptr, ourArch.spacer.piece.code, ourArch.spacer.piece.size);
			ptr[ourArch.spacer.offset] = (epilogPtr - ptr) - ourArch.spacer.piece.size;
			ptr += ourArch.spacer.piece.size;
		}

		{
			auto &el = v[v.size() - 1];
			memcpy(ptr, ourArch.comparer.piece.code, ourArch.comparer.piece.size);
			ptr[ourArch.comparer.offset] = el;
			ptr += ourArch.comparer.piece.size;
		}

		memcpy(ptr, ourArch.epilog.code, ourArch.epilog.size);
		ptr += ourArch.epilog.size;

		if (mprotect(funcJitCode, pageSize, PROT_READ | PROT_EXEC) == -1) {
			throw std::bad_alloc();
		}
	}

	JittedCharDetector::~JittedCharDetector(){
		munmap(funcJitCode, pageSize);
	}
#endif
