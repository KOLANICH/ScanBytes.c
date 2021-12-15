#include <ScanBytes/MultiThreadedOrderedAppendOnlyAllocator.hpp>
//#include "mio.hpp"

template<typename ValueT>
ValueBlock<ValueT>::ValueBlock(uint16_t id, size_t allocSize): id(id), vec(allocSize){
	vec.resize(0);
}

template<typename ValueT>
std::strong_ordering ValueBlock<ValueT>::operator<=>(const ValueBlock &b) const{
	auto cmp = id <=> b.id;
	if (cmp != std::strong_ordering::equal){
		return cmp;
	} else {
		return vec[0] <=> b.vec[0];
	}
}

template<typename ValueT>
size_t MTOAOA<ValueT>::page_size = 4 * 1024;

template<typename ValueT>
ThreadAllocator<ValueT>::ThreadAllocator(uint16_t id, MTOAOA<ValueT> &parent): id(id), parent(parent) {
	allocSize = parent.page_size;
	reserveBlock();
}

template<typename ValueT>
ThreadAllocator<ValueT>::~ThreadAllocator(){
	finalize();
}

template<typename ValueT>
void ThreadAllocator<ValueT>::finalize(){
}

template<typename ValueT>
void ThreadAllocator<ValueT>::reserveBlock(){
	reserveBlock(allocSize);
}

template<typename ValueT>
void ThreadAllocator<ValueT>::reserveBlock(size_t allocSize){
	mySpan = parent.reserveBlock(id, allocSize);
	consumed = allocSize;
}

template<typename ValueT>
void ThreadAllocator<ValueT>::append(uint64_t num){
	auto & v = (*mySpan).vec;
	v.emplace_back(num);
	if(v.size() == v.capacity()){
		reserveBlock();
	}
}

template<typename ValueT>
MTOAOA<ValueT>::MTOAOA(){}

template<typename ValueT>
typename MTOAOA<ValueT>::TAllocT MTOAOA<ValueT>::getForThread(uint16_t id){
	return ThreadAllocator<ValueT>(id, *this);
}

template<typename ValueT>
typename MTOAOA<ValueT>::NBT *MTOAOA<ValueT>::reserveBlock(uint16_t id, size_t allocSize){
	lock.lock();
	auto res = &*chunks.emplace_back(std::make_unique<NBT>(id, allocSize));
	lock.unlock();
	return res;
}

template struct ValueBlock<uint64_t>;
template struct MTOAOA<uint64_t>;
template struct ThreadAllocator<uint64_t>;
