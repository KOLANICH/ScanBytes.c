#pragma once

#include <cstdint>
#include <vector>
#include <compare>
#include <memory>
#include <thread>

template<typename ValueT>
struct ValueBlock{
	using value_type = ValueT;
	using VecT = std::vector<ValueT>;

	uint16_t id;
	VecT vec;

	ValueBlock(uint16_t id, size_t allocSize);
	std::strong_ordering operator<=>(const ValueBlock &b) const;
};

template<typename ValueT> struct ThreadAllocator;

template<typename ValueT>
struct MTOAOA{
	using TAllocT = ThreadAllocator<ValueT>;
	using StorageT = std::vector<std::unique_ptr<ValueBlock<ValueT>>>;
	using NBT = ValueBlock<ValueT>;

	static size_t page_size;

	StorageT chunks;
	std::mutex lock;

	MTOAOA();

	TAllocT getForThread(uint16_t id);

	NBT* reserveBlock(uint16_t id, size_t allocSize);
};

template<typename ValueT>
struct ThreadAllocator{
	using NAllocT = MTOAOA<ValueT>;

	uint16_t id;
	size_t allocSize;
	size_t consumed;
	NAllocT &parent;
	typename NAllocT::NBT *mySpan;

	ThreadAllocator(uint16_t id, NAllocT &parent);
	~ThreadAllocator();

	void finalize();

	void reserveBlock();

	void reserveBlock(size_t allocSize);

	void append(uint64_t num);
};

